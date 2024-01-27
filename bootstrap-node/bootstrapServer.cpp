#include "bootstrapServer.h"
#include "../json.hpp"
#include "logger.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using json = nlohmann::json;
extern Logger logger;

/*
 * This function is to start our peer. This will allow other peers to connect
 * to this peer and send commands that need to be processed by the application
 */
void BootstrapServer::startServer(std::string &port) {

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

  // ptr to the linked list of address information
  std::unique_ptr<struct addrinfo, decltype(&freeaddrinfo)> ai_ptr(
      ai, freeaddrinfo);

  // look for a valid socket address
  for (p = ai; p != nullptr; p = p->ai_next) {
    serverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    if (serverSocket == -1) {
      continue;
    }
    // set the socket options
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
        -1) {
      logger.log("Set socket options failed", LogLevel::Error);
      return;
    }

    // bind the server to the port
    if (bind(serverSocket, p->ai_addr, p->ai_addrlen) == -1) {
      close(serverSocket);
      continue;
    }
    break;
  }

  if (p == nullptr) {
    logger.log("Failed to bind", LogLevel::Debug);
  }

  // listen on the server socket
  if (listen(serverSocket, 10) == -1) {
    logger.log("Failed to listen", LogLevel::Debug);
  }

  pfds.push_back({serverSocket, POLLIN, 0});
  logger.log("Server started. Listening for connections...", LogLevel::Info);
  listenForConnections();
}

/*
 * this function will listen for incomming connections on the server and process
 * them
 */
void BootstrapServer::listenForConnections() {
  // keep listening for events
  while (true) {
    logger.log("Waiting for an event...", LogLevel::Info);

    // see how many events we got
    int poll_count = poll(pfds.data(), pfds.size(), -1);
    if (poll_count == -1) {
      logger.log("Error while polling", LogLevel::Error);
    }

    // loop through all the tracked fds and look which got an event
    for (size_t i = 0; i < pfds.size(); i++) {
      if (pfds[i].revents & POLLIN) {

        // if the event is on the server, it must be a new connection
        if (pfds[i].fd == serverSocket) {
          struct sockaddr_storage remoteaddr;
          socklen_t addrlen = sizeof(remoteaddr);
          int newfd =
              accept(serverSocket, (struct sockaddr *)&remoteaddr, &addrlen);
          if (newfd == -1) {
            logger.log("Error accepting connection", LogLevel::Error);
            continue;
          }

          // track the fd
          add_to_pfds(newfd, pfds);
          logger.log("Got a new connection on socket " + std::to_string(newfd),
                     LogLevel::Debug);
        } else {
          // recieve the data
          char buf[2048];
          int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);
          int sender_fd = pfds[i].fd;

          if (nbytes <= 0) {
            if (nbytes == 0) {
              // message to close the socket
              logger.log("Peer closed", LogLevel::Info);
              handleSocketClose(sender_fd);
            } else {
              logger.log("Error recieveing information", LogLevel::Warning);
            }

            close(pfds[i].fd);
            del_from_pfds(i, pfds);
            --i; // Adjust the index since we removed an element
          } else {
            // get the request from the peer

            std::string request(buf, nbytes);
            logger.log("Received data " + request, LogLevel::Info);
            json requestJson = json::parse(request);

            if (requestJson["command"] == "requestSnapshot") {
              processSnapshotRequest(requestJson["data"]);
            } else if (requestJson["command"] == "requestListFiles") {
              processListFilesRequest(requestJson);
            } else if (requestJson["command"] == "requestPeerWithFile") {
              processPeerWithFileRequest(requestJson);
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
void BootstrapServer::connectToPeerServer(std::string &peerServerName,
                                          int port) {
  logger.log("In connectToPeerServer", LogLevel::Debug);
  std::string ip = "127.0.0.1";
  logger.log("Connecing to " + peerServerName + " on " + std::to_string(port),
             LogLevel::Info);

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
    logger.log("Unable to connect to " + peerServerName + " on " +
                   std::to_string(port),
               LogLevel::Warning);
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
void BootstrapServer::sendMessage(const std::string &peerServerName,
                                  const std::string &payload) {
  logger.log("In sendMessage", LogLevel::Debug);
  auto it = nameToFd.find(peerServerName);
  if (it != nameToFd.end()) {
    logger.log("Sending message " + payload + " to " + peerServerName,
               LogLevel::Debug);
    send(it->second, payload.c_str(), payload.size(), 0);
  }
}

/*
 * This function is for processing a request from a peer right as it
 * joins the network. It will send the current snapshot to the new peer
 * and then forward the new peer to all of the currently connected peers
 * For them to conenct to each other
 */
void BootstrapServer::processSnapshotRequest(json requestJson) {
  logger.log("In processSnapshotRequest", LogLevel::Debug);
  std::string name = requestJson["name"];
  int port = requestJson["port"];
  std::vector<std::string> files = requestJson["files"];

  json responseSnapshotJson;
  responseSnapshotJson["command"] = "responseSnapshot";
  responseSnapshotJson["data"] = nameToPort;
  std::string responseSnapshotMessage = responseSnapshotJson.dump();

  // send a message back to the newly connected peer
  connectToPeerServer(name, port);
  sendMessage(name, responseSnapshotMessage);

  // send this peer to all other peers
  json responseNotifyJson;
  responseNotifyJson["command"] = "responseNotify";
  responseNotifyJson["port"] = port;
  responseNotifyJson["name"] = name;
  std::string responseNotifyMessage = responseNotifyJson.dump();
  for (auto &pair : nameToFd) {
    if (pair.first != name) {
      sendMessage(pair.first, responseNotifyMessage);
    }
  }

  // save the info
  nameToPort[name] = port;
  nameToFiles[name] = files;
}

/*
 * This function is to process a request from a peer to get all the available
 * files in the p2p network. The list of files consists of the files from all
 * currently active peers
 */
void BootstrapServer::processListFilesRequest(json requestJson) {
  logger.log("In processListFiles", LogLevel::Debug);
  std::string name = requestJson["name"];
  std::vector<std::string> files;

  for (const auto pair : nameToFiles) {
    const auto &values = pair.second;
    files.insert(files.end(), values.begin(), values.end());
  }

  // construct the response
  json responseListFilesJson;
  responseListFilesJson["command"] = "responseListFiles";
  responseListFilesJson["files"] = files;
  std::string responseListFilesMessage = responseListFilesJson.dump();

  // send hte message
  sendMessage(name, responseListFilesMessage);
}

/*
 * This function will process a request to search for the peer that holds a
 * specific file. This is utilized in file transfer as a peer will contact the
 * bootstrap to get the other peer that it should contact and then reach out to
 * that peer directly for file transfer
 */
void BootstrapServer::processPeerWithFileRequest(json requestJson) {
  logger.log("In peerWithFileRequest", LogLevel::Debug);
  std::string peer = "None";
  std::string target = requestJson["file"];

  // get the name of the peer that has the file
  for (const auto &pair : nameToFiles) {
    const auto &key = pair.first;
    const auto &values = pair.second;

    if (std::find(values.begin(), values.end(), target) != values.end()) {
      peer = key;
      break;
    }
  }

  // construct the response
  json responsePeerWithFile;
  responsePeerWithFile["command"] = "responsePeerWithFile";
  responsePeerWithFile["peer"] = peer;
  responsePeerWithFile["file"] = target;
  std::string responsePeerWithFileMessage = responsePeerWithFile.dump();

  // send the message
  sendMessage(requestJson["name"], responsePeerWithFileMessage);
}

/*
 * This function is for getting the correct socket information
 */
void *BootstrapServer::get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/*
 * This function is for adding a newly connected peer to our list of
 * peers for the boostrap node to listen for commands
 */
void BootstrapServer::add_to_pfds(int newfd, std::vector<struct pollfd> &pfds) {
  if (pfds.size() == pfds.capacity()) {
    pfds.reserve(pfds.size() * 2);
  }
  pfds.push_back({newfd, POLLIN, 0});
}

/*
 * This function is for removing a peer from the list of peers that our
 * server is listening to
 */
void BootstrapServer::del_from_pfds(size_t index,
                                    std::vector<struct pollfd> &pfds) {
  std::swap(pfds[index], pfds.back());
  pfds.pop_back();
}

/*
 * This function will remove the peer from our list of active peers
 */
void BootstrapServer::handleSocketClose(int socketFd) {
  std::string peerName = fdToName[socketFd];
  logger.log("Peer on " + std::to_string(socketFd) + " closed",
             LogLevel::Debug);
  fdToName.erase(socketFd);
  nameToFd.erase(peerName);
  nameToFiles.erase(peerName);
}
