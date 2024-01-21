#include "peer.h"
#include "../json.hpp"
#include "logger.h"
#include "util.h"
#include <fstream>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using json = nlohmann::json;

extern std::atomic<bool> isRunning;
extern Logger logger;

namespace peer {
/*
 * Constructor
 */
Peer::Peer(std::string &name, int port, std::vector<std::string> &files)
    : name(name), port(port), serverSocket(-1), files(files){};

/*
 * Destructor
 */
Peer::~Peer() {};

/*
 * This function is to start our peer. This will allow other peers to connect
 * to this peer and send commands that need to be processed by the application
 */
void Peer::startServer(int port) {
  int yes = 1;
  struct addrinfo hints, *ai, *p;
  std::string port_str = std::to_string(port);
  inFileTransferMode = false;

  // get address information
  memset(&hints, 0, sizeof(hints)); // zero out hints
  hints.ai_family = AF_UNSPEC;      // ipv4 or ipv6
  hints.ai_socktype = SOCK_STREAM;  // tcp
  hints.ai_flags = AI_PASSIVE;      // use the address the host is running on
  if (int rv = getaddrinfo(nullptr, port_str.c_str(), &hints, &ai); rv != 0) {
    logger.log("Unable to get address info", LogLevel::Error);
    return;
  }

  std::unique_ptr<struct addrinfo, decltype(&freeaddrinfo)> ai_ptr(
      ai, freeaddrinfo);

  for (p = ai; p != nullptr; p = p->ai_next) {
    serverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    if (serverSocket == -1) {
      continue;
    }

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
        -1) {
      logger.log("Unable to set socket options", LogLevel::Error);
      return;
    }

    if (bind(serverSocket, p->ai_addr, p->ai_addrlen) == -1) {
      close(serverSocket);
      continue;
    }
    break;
  }

  if (p == nullptr) {
    logger.log("Unable to find suitable socket for binding", LogLevel::Error);
    return;
  }

  if (listen(serverSocket, 10) == -1) {
    throw std::system_error(errno, std::generic_category(), "listen failed");
  }

  pfds.push_back({serverSocket, POLLIN, 0});
  logger.log("Server started, listening for activity...", LogLevel::Info);

  // contact the boostrap
  std::string bootstrap = "bootstrap";
  connectToPeerServer(bootstrap, 50000);
  requestSnapshot();

  listenForActivity();
}

/*
 * this function will listen for incomming connections on the server and process
 * them
 */
void Peer::listenForActivity() {
  char buf[256];
  char remoteIP[INET6_ADDRSTRLEN];

  while (true) {
    logger.log("Waiting for an event...", LogLevel::Info);

    int poll_count = poll(pfds.data(), pfds.size(), -1);

    if (poll_count == -1) {
      logger.log("Error while poling", LogLevel::Error);
      return;
    }

    for (size_t i = 0; i < pfds.size(); i++) {
      if (pfds[i].revents & POLLIN) {
        if (pfds[i].fd == serverSocket) {
          struct sockaddr_storage remoteaddr;
          socklen_t addrlen = sizeof(remoteaddr);
          int newfd =
              accept(serverSocket, (struct sockaddr *)&remoteaddr, &addrlen);

          if (newfd == -1) {
            logger.log("Error accepting connection", LogLevel::Error);
            continue;
          }

          add_to_pfds(newfd, pfds);
          logger.log("Got a new connection on socket " + std::to_string(newfd), LogLevel::Debug);
        } else {
          if (inFileTransferMode) {
            receiveFile(fileTransferOutputName, pfds[i].fd);
            inFileTransferMode = false;
          } else {
            int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);
            int sender_fd = pfds[i].fd;

            if (nbytes <= 0) {
              if (nbytes == 0) {
                logger.log("Peer closed", LogLevel::Info);
                handleSocketClose(sender_fd);

              } else {
                logger.log("Error recieveing information", LogLevel::Warning);
              }

              close(pfds[i].fd);
              del_from_pfds(i, pfds);
              --i; // Adjust the index since we removed an element
            } else {
              std::string command(buf, nbytes);
              logger.log("Received data " + command, LogLevel::Info);
              json commandJson = json::parse(command);

              if (commandJson["command"] == "responseSnapshot") {
                processSnapshot(commandJson);
              } else if (commandJson["command"] == "responseNotify") {
                processNotify(commandJson);
              } else if (commandJson["command"] == "responseListFiles") {
                processListFiles(commandJson);
              } else if (commandJson["command"] == "responsePeerWithFile") {
                processPeerWithFile(commandJson);
              } else if (commandJson["command"] == "requestFileContent") {
                processRequestFile(commandJson);
              } else {
                logger.log("Command not recognized", LogLevel::Warning);
              }
            }
          }
        }
      }
    }
  }
}

/*
 * This function is used to connect to other the servers of other peers within
 * the P2P network. After connecting, we save the connection to our internal
 * state so that if we would like to contact this peer in the future, we do not
 * have to reconnect to the server
 */
void Peer::connectToPeerServer(std::string &peerServerName, int port) {
  logger.log("In connectToPeerServer", LogLevel::Debug);
  std::string ip = "127.0.0.1";
  logger.log("Connecing to " + peerServerName + " on " + std::to_string(port), LogLevel::Info);

  // create the socket
  int peerClientFd = socket(AF_INET, SOCK_STREAM, 0);
  if (peerClientFd < 0) {
    logger.log("Unable to create the socket", LogLevel::Warning);
    return;
  }

  // set up the server that we want to connect to
  sockaddr_in sockaddr{};
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(port);
  inet_pton(AF_INET, ip.c_str(), &sockaddr.sin_addr);

  // connect to the peer server specified by ip:port
  if (connect(peerClientFd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) <
      0) {
    logger.log("Unable to connect to " + peerServerName + " on " + std::to_string(port), LogLevel::Warning);
    close(peerClientFd);
    return;
  }
  logger.log("Connection successful", LogLevel::Debug);

  // register the new connection
  nameToFd[peerServerName] = peerClientFd;
  fdToName[peerClientFd] = peerServerName;
}

/*
 * This funciton is for sending a message to another peer in the P2P network. If
 * we have not previoulsy connected to this peer and it is a new one, we will
 * make sure to connect to and register it before sending our message
 */
void Peer::sendMessage(const std::string &peerServerName,
                       const std::string &payload) {
  logger.log("In sendMessage with " + peerServerName + ":" + payload, LogLevel::Debug);
  auto it = nameToFd.find(peerServerName);
  if (it != nameToFd.end()) {
    logger.log("Sending message " + payload + " to " + peerServerName, LogLevel::Debug);
    send(it->second, payload.c_str(), payload.size(), 0);
  }
}

/**********************
 * Functions to communcate with other peers
 **********************/

/*
 * This fuction sends a new peer message to the bootstrap server. This is the
 * peers first contact with the bootstrap and it will send a snapshot of all the
 * current peers back to be processed
 */
void Peer::requestSnapshot() {
   logger.log("In requestSnapshot", LogLevel::Debug);
  // construct the boostrap message
  json requestJson;
  requestJson["command"] = "requestSnapshot";

  json data;
  data["name"] = name;
  data["port"] = port;
  data["files"] = files;

  requestJson["data"] = data;
  std::string requestMessage = requestJson.dump();

  sendMessage("bootstrap", requestMessage);
}

/**********************
 * Functions to process requests from other peers
 **********************/

/*
 * This function will process a snapshot from the boostrap server. A peer will
 * get a bootstreap response upon first connecting to the network. This will
 * contain information about other peers in the network for this peer to connet
 * to
 */
void Peer::processSnapshot(json responseJson) {
  logger.log("In processSnapshot", LogLevel::Debug);
  json peers = responseJson["data"];

  // go through all of the peers and connect to them
  for (auto &[name, port] : peers.items()) {
    std::string peerName(name);
    connectToPeerServer(peerName, port);
  }
}

/*
 * This function will process a notify message from the bootstrap server.
 * This means that a new peer has connected to the network and we want to
 * connect to it
 */
void Peer::processNotify(json responseJson) {
  logger.log("In processNotify", LogLevel::Debug);
  //  parse the peer data
  std::string name = responseJson["name"];
  int port = responseJson["port"];

  // connect to the peer
  connectToPeerServer(name, port);
}

/*
 * This function will process a message from the bootstrap sever
 * which contains a list of the files the network has access to
 */
void Peer::processListFiles(json command) {
  logger.log("In processListFiles", LogLevel::Debug);
  for (auto &[fileIdx, fileName] : command["files"].items()) {
    std::cout << fileName << " ";
  }
  std::cout << std::endl;
}

/*
 * This function will remove the peer from our list of active peers
 */
void Peer::handleSocketClose(int socketFd) {
  std::string peerName = fdToName[socketFd];
  logger.log("Peer: " + peerName + " on " + std::to_string(socketFd) + " closed", LogLevel::Debug);
  fdToName.erase(socketFd);
  nameToFd.erase(peerName);
}

/*
 * This function will process commands from the cli and
 * redirect it to the proper execution functions in the peer
 */
void Peer::processCommand(std::string &command) {
  logger.log("In processCommand", LogLevel::Debug);
  std::string payload;
  if (command == "listFiles") {
    logger.log("Received a listFiles command", LogLevel::Debug);
    json requestJson;
    requestJson["command"] = "requestListFiles";
    requestJson["name"] = name;
    std::string requestMessage = requestJson.dump();
    sendMessage("bootstrap", requestMessage);
  } else if (command == "getFile") {
    logger.log("Received a getFile command", LogLevel::Debug);
    std::string filename;
    std::cout << "What file would you to download: ";
    getline(std::cin, filename);
    json requestJson;
    requestJson["command"] = "requestPeerWithFile";
    requestJson["name"] = name;
    requestJson["file"] = filename;
    std::string requestMessage = requestJson.dump();
    sendMessage("bootstrap", requestMessage);
  }
}

/*
* This function is used to process a response from the bootstrap
* that will tell us which peer has the file that we are requesting.
* We then want to send a request to that peer signaling the initiation
* of file transfer. This is the first step of file tarnsfer
*/
void Peer::processPeerWithFile(json responseJson) {
  logger.log("In processPeerWithFile", LogLevel::Debug);
  std::string peer = responseJson["peer"];
  std::string file = responseJson["file"];
  logger.log("Peer " + peer + " has file " + file, LogLevel::Debug);
  inFileTransferMode = true;
  fileTransferOutputName = file;

  json requestJson;
  requestJson["command"] = "requestFileContent";
  requestJson["name"] = name;
  requestJson["file"] = file;
  std::string requestMessage = requestJson.dump();
  sendMessage(peer, requestMessage);
}

/*
* This function is used to send a file over to another peer. Once a 
* peer gets a request for the file content, it will extract the peer and the 
* filename and then send it over
*/
void Peer::processRequestFile(json responseJson) {
    logger.log("in processRequestFile", LogLevel::Debug);
    // send over the file
    std::string name = responseJson["name"];
    std::string requestedFile = responseJson["file"];

    sendFile(requestedFile, name);
}

/*
* This function is for sending a file to another peer. It will open and the file 
* and continuously send over the file untill it is all sent
*/
void Peer::sendFile(const std::string &filename, std::string &name) {

   std::string filePath = "files/" + filename;
    logger.log("In sendFile", LogLevel::Debug);
    logger.log("Sending " + filename + " to " + name, LogLevel::Debug);

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        logger.log("Unable to open file", LogLevel::Error);
        return;
    }

    char buffer[1024];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        int bytesRead = file.gcount();
        logger.log("Sending " + std::string(buffer) + " to " + name + " on " +  std::to_string(nameToFd[name]), LogLevel::Debug);
        send(nameToFd[name], buffer, bytesRead, 0);
    }

    // Send an end-of-file signal
    const std::string eofSignal = "<EOF>";
    send(nameToFd[name], eofSignal.c_str(), eofSignal.size(), 0);

    logger.log("Finished sending file", LogLevel::Debug);
}

/*
* This function is for receiving a file from another peer. It will be called when 
* we are in file transfer mode and will keep reading bytes from the network and
* writing it to the intended file
*/
void Peer::receiveFile(const std::string &outputFilename, int sockfd) {
  logger.log("In recvFile", LogLevel::Debug);
  logger.log("Recieving file " + outputFilename, LogLevel::Debug); 
  std::ofstream file(outputFilename, std::ios::binary);
  if (!file.is_open()) {
    logger.log("Failed to open file for writing", LogLevel::Error);
    return;
  }

  std::string buffer;
  const std::string eofSignal = "<EOF>";
  char tempBuffer[1025]; // Slightly larger buffer to accommodate null terminator

  while (true) {
      int bytes_received = recv(sockfd, tempBuffer, sizeof(tempBuffer) - 1, 0);
      if (bytes_received <= 0) break; // Check for end of transmission or errors
      logger.log("Received " + std::to_string(bytes_received) + " bytes", LogLevel::Debug);

      tempBuffer[bytes_received] = '\0'; // Null-terminate the string
      buffer.append(tempBuffer, bytes_received);

      // Check if the buffer ends with the EOF signal
      if (buffer.size() >= eofSignal.size() && buffer.substr(buffer.size() - eofSignal.size()) == eofSignal) {
          // Remove EOF signal from buffer and break
          buffer.erase(buffer.size() - eofSignal.size());
          break;
      }
  }

  if (!buffer.empty()) {
      file.write(buffer.c_str(), buffer.size());
  }

  file.close();
  logger.log("Finished receiving file", LogLevel::Debug);
}

} // namespace peer
