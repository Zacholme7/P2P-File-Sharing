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
Peer::Peer(std::string &name, std::string port, std::vector<std::string> &files)
    : name(name), port(port), serverSocket(-1),
      files(files){
          // establish connection to bootstrap server
          // std::string bootstrap = "bootstrap";
          // connectToPeerServer(50000, "127.0.0.1", bootstrap);
      };

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
void Peer::startServer(std::string &port) {
  int yes = 1;
  struct addrinfo hints, *ai, *p;

  // get address information
  memset(&hints, 0, sizeof(hints)); // zero out hints
  hints.ai_family = AF_UNSPEC;      // ipv4 or ipv6
  hints.ai_socktype = SOCK_STREAM;  // tcp
  hints.ai_flags = AI_PASSIVE;      // use the address the host is running on
  if (int rv = getaddrinfo(nullptr, port.c_str(), &hints, &ai); rv != 0) {
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
            } else {
              std::cerr << "recv error: " << strerror(errno) << '\n';
            }

            close(pfds[i].fd);
            del_from_pfds(i, pfds);
            --i; // Adjust the index since we removed an element
          } else {
            std::cout << "Received data: " << std::string(buf, nbytes) << '\n';
            /*
            std::string command(buffer);
            json commandJson = json::parse(command);

            if (commandJson["command"] == "respSnapshot") {
              processSnapshot(commandJson);
            } else if (commandJson["command"] == "respNotify") {
              processNotify(commandJson);
            } else if (commandJson["command"] == "respListFiles") {
              processListFiles(commandJson);
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

 * This function is used to connect to other the servers of other peers within
 * the P2P network. After connecting, we save the connection to our internal
 * state so that if we would like to contact this peer in the future, we do not
 * have to reconnect to the server
void Peer::connectToPeerServer(int port, const std::string &ip,
                               const std::string &peerServerName) {
  // create the socket
  logger.log("Trying to connect to " + peerServerName + "...", LogLevel::Debug);
  int peerClientFd = socket(AF_INET, SOCK_STREAM, 0);
  if (peerClientFd < 0) {
    logger.log("Failed to create socket", LogLevel::Error);
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
    logger.log("Unable to connect to " + peerServerName, LogLevel::Error);
    close(peerClientFd);
    return;
  }
  logger.log("Successfully connected to " + peerServerName, LogLevel::Debug);

  // register the new connection
  connectedServers[peerServerName] = peerClientFd;
}

 * This fuction sends a new peer message to the bootstrap server. This is the
 * peers first contact with the bootstrap and it will send a snapshot of all the
 * current peers back to be processed
void Peer::contactBootstrap() {
  // construct the boostrap message
  json requestJson;
  requestJson["command"] = "reqNewPeer";

  json data;
  data["name"] = name;
  data["port"] = port;
  data["ip"] = "127.0.0.1";
  data["files"] = files;

  requestJson["data"] = data;
  std::string requestMessage = requestJson.dump();

  sendMessage("bootstrap", requestMessage);
}

 * This funciton is for sending a message to another peer in the P2P network. If
 * we have not previoulsy connected to this peer and it is a new one, we will
 * make sure to connect to and register it before sending our message
void Peer::sendMessage(const std::string &peerServerName,
                       const std::string &payload) {
  auto it = connectedServers.find(peerServerName);
  if (it != connectedServers.end()) {
    send(it->second, payload.c_str(), payload.size(), 0);
  }
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

// Functions for message processing
void processSnapshot(json command) {
  json peers = command["data"];

  // go through all of the peers and connect to them
  for (auto &[peerName, peerDetails] : peers.items()) {
    std::string ip = peerDetails["ip"];
    std::string port = peerDetails["port"];
    connectToPeerServer(std::stoi(port), ip, peerName);
  }
}

void processNotify(json command) {
  // processNotify()
  //  parse the peer data
  std::string peerName = command["name"];
  std::string ip = command["ip"];
  std::string port = command["port"];

  // connect to the peer
  connectToPeerServer(std::stoi(port), ip, peerName);
}

void processListfiles(json command) {
  for (auto &fileName : command["data"].items()) {
    std::cout << fileName << " ";
  }
  std::cout << std::endl;
}

void processPeerWithFile(json command) {}
};
// processPeerWithFile

// processSnapshot(commandJson);
// processNotify()
//  processListFiles()
//  processPeerWithFile



// start a new thread to listen to messages that are sent to this socket?
int addrlen = sizeof(sockaddr);
while (true && isRunning) {
        // accept the new connection
        logger.log("Server waiting for a new connection...", LogLevel::Debug);
        int newPeerClientFd = accept(serverSocket, (struct sockaddr*)&sockaddr,
(socklen_t*)&addrlen); if (newPeerClientFd < 0) { logger.log("Failed to accept
connection", LogLevel::Error); continue;
        }
        logger.log("Server got a new connection, starting new thread...",
LogLevel::Debug);

        // set a timeout on the socket
        struct timeval tv;
        tv.tv_sec = 5;  // Set the timeout to 5 seconds
        tv.tv_usec = 0;  // Not using microseconds

        if (setsockopt(newPeerClientFd, SOL_SOCKET, SO_RCVTIMEO, (const
char*)&tv, sizeof tv) < 0) { perror("setsockopt"); continue;
        }

}
*/
