#include "peer.h"
#include "../json.hpp"
#include "logger.h"
#include "util.h"
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
    : name(name), port(port), serverSocket(-1),
      files(files) {};

/*
 * Destructor
 */
Peer::~Peer() {
  // send close message to all connected servers
  std::cout << "the peer is being destoryed" << std::endl;
  for (auto server : connectedServers) {
    std::cout << server.first << std::endl;
    // sendMessage(server.first, "respClientClosed");
  }
};

/*
 * This function is to start our peer. This will allow other peers to connect
 * to this peer and send commands that need to be processed by the application
 */
void Peer::startServer(int port) {
  int yes = 1;
  struct addrinfo hints, *ai, *p;
  std::string port_str = std::to_string(port);

  // get address information
  memset(&hints, 0, sizeof(hints)); // zero out hints
  hints.ai_family = AF_UNSPEC;      // ipv4 or ipv6
  hints.ai_socktype = SOCK_STREAM;  // tcp
  hints.ai_flags = AI_PASSIVE;      // use the address the host is running on
  if (int rv = getaddrinfo(nullptr, port_str.c_str(), &hints, &ai); rv != 0) {
    throw std::runtime_error(gai_strerror(rv));
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
      throw std::system_error(errno, std::generic_category(),
                              "setsockopt failed");
    }

    if (bind(serverSocket, p->ai_addr, p->ai_addrlen) == -1) {
      close(serverSocket);
      continue;
    }
    break;
  }

  if (p == nullptr) {
    throw std::runtime_error("Failed to bind");
  }

  if (listen(serverSocket, 10) == -1) {
    throw std::system_error(errno, std::generic_category(), "listen failed");
  }

  pfds.push_back({serverSocket, POLLIN, 0});
  std::cout << "Server started. Listening for connections...\n";
  
  // contact the boostrap
  std::string bootstrap = "bootstrap";
  connectToPeerServer(bootstrap, 50000);
  requestSnapshot(); 

  listenForConnections();
}

/*
 * this function will listen for incomming connections on the server and process
 * them
 */
void Peer::listenForConnections() {
  char buf[256];
  char remoteIP[INET6_ADDRSTRLEN];

  while (true) {
    std::cout << "Waiting for an event...\n";

    int poll_count = poll(pfds.data(), pfds.size(), -1);

    if (poll_count == -1) {
      throw std::system_error(errno, std::generic_category(), "poll failed");
    }

    for (size_t i = 0; i < pfds.size(); i++) {
      if (pfds[i].revents & POLLIN) {
        if (pfds[i].fd == serverSocket) {
          struct sockaddr_storage remoteaddr;
          socklen_t addrlen = sizeof(remoteaddr);
          int newfd =
              accept(serverSocket, (struct sockaddr *)&remoteaddr, &addrlen);

          if (newfd == -1) {
            std::cerr << "Error in accept: " << strerror(errno) << '\n';
            continue;
          }

          add_to_pfds(newfd, pfds);

          std::cout << "New connection from "
                    << inet_ntop(remoteaddr.ss_family,
                                 get_in_addr((struct sockaddr *)&remoteaddr),
                                 remoteIP, INET6_ADDRSTRLEN)
                    << " on socket " << newfd << '\n';
        } else {
          int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);
          int sender_fd = pfds[i].fd;

          if (nbytes <= 0) {
            if (nbytes == 0) {
              std::cout << "Socket " << sender_fd << " closed\n";
              handleSocketClose(sender_fd);

            } else {
              std::cerr << "recv error: " << strerror(errno) << '\n';
            }

            close(pfds[i].fd);
            del_from_pfds(i, pfds);
            --i; // Adjust the index since we removed an element
          } else {
            std::cout << "Received data: " << std::string(buf, nbytes) << '\n';
            std::string command(buf, nbytes);
            json commandJson = json::parse(command);

            if (commandJson["command"] == "responseSnapshot") {
              processSnapshot(commandJson);
            } else if (commandJson["command"] == "responseNotify") {
              processNotify(commandJson);
            } else if (commandJson["command"] == "responseListFiles") {
              processListFiles(commandJson);
            }
            /*

            } else if (commandJson["command"] == "respPeerWithFile") {
              processPeerWithFile(commandJson);
            } else if (commandJson["command"] == "reqFile") {
              processRequestFile(commandJson);
            } else if (commandJson["command"] == "recvFile") {
              processRecieveFile(commandJson);
            }
*/
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
  std::string ip = "127.0.0.1";
  std::cout << "connecting to " << peerServerName << " on " << port << std::endl;

  // create the socket
  int peerClientFd = socket(AF_INET, SOCK_STREAM, 0);
  if (peerClientFd < 0) {
    return;
  }

  // set up the server that we want to connect to
  sockaddr_in sockaddr{};
  sockaddr.sin_family = AF_INET;
  sockaddr.sin_port = htons(port);
  inet_pton(AF_INET, ip.c_str(), &sockaddr.sin_addr);

  // connect to the peer server specified by ip:port
  if (connect(peerClientFd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
    close(peerClientFd);
    return;
  }

  // register the new connection
  nameToFd[peerServerName] = peerClientFd;
  fdToName[peerClientFd] = peerServerName;
}

/*
 * This funciton is for sending a message to another peer in the P2P network. If
 * we have not previoulsy connected to this peer and it is a new one, we will
 * make sure to connect to and register it before sending our message
 */
void Peer::sendMessage(const std::string &peerServerName, const std::string &payload) {
  auto it = nameToFd.find(peerServerName);
  if (it != nameToFd.end()) {
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
  // construct the boostrap message
  json requestJson;
  requestJson["command"] = "requestSnapshot";

  json data;
  data["name"] = name;
  data["port"] = port;
  data["ip"] = "127.0.0.1";
  data["files"] = files;

  requestJson["data"] = data;
  std::string requestMessage = requestJson.dump();

  sendMessage("bootstrap", requestMessage);
}

/**********************
* Functions to process requests from other peers
**********************/

/*
* This function will process a snapshot from the boostrap server. A peer will get a bootstreap
* response upon first connecting to the network. This will contain information about other
* peers in the network for this peer to connet to
*/
void Peer::processSnapshot(json responseJson) {
  std::cout << "in process" << std::endl;
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
  std::cout << "in process notify" << std::endl;
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
  for (auto &fileName : command["data"].items()) {
    std::cout << fileName << " ";
  }
  std::cout << std::endl;
}

/*
* This function will remove the peer from our list of active peers
*/
void Peer::handleSocketClose(int socketFd) {
  std::string peerName = fdToName[socketFd];
  fdToName.erase(socketFd);
  nameToFd.erase(peerName);
}

} // namespace peer
















/*
void Peer::sendFile(const std::string &filename, int socket) {
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open file for reading" << '\n';
    return;
  }

  char buffer[1024];
  while (!file.eof()) {
    file.read(buffer, sizeof(buffer));
    int bytesRead = file.gcount();
    send(socket, buffer, bytesRead, 0);
  }
}

void Peer::recieveFile(const std::string &outputFilename, int sockfd) {
  std::ofstream file(outputFilename, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Failed to open file for writing" << '\n';
  }

  char buffer[1024];
  int bytes_received;

  while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
    file.write(buffer, bytes_received);
  }
  file.close();
}




 * This function will process commands from the cli and
 * redirect it to the proper execution functions in the peer
void Peer::processCommand(std::string &command) {
  std::string payload;
  if (command == "listFiles") {
    // construct the listFiles message
    json requestJson;
    requestJson["command"] = "reqListFiles";
    requestJson["name"] = name;
    std::string requestMessage = requestJson.dump();

    // send the message to the bootstrap server
    sendMessage("bootstrap", requestMessage);
  } else if (command == "getfile") {
    std::string filename;
    std::cout << "What file would you to download: ";
    getline(std::cin, filename);
    payload = "getfile;filename";
    sendMessage("bootstrap", payload);
  }
}


*/