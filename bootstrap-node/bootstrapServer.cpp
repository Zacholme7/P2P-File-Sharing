#include "bootstrapServer.h"
#include <unistd.h>
#include "../json.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
using json = nlohmann::json;



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

  std::unique_ptr<struct addrinfo, decltype(&freeaddrinfo)> ai_ptr(
      ai, freeaddrinfo);

  for (p = ai; p != nullptr; p = p->ai_next) {
    serverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    if (serverSocket == -1) {
      continue;
    }
    // set the socket options
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
        -1) {
      throw std::system_error(errno, std::generic_category(),
                              "setsockopt failed");
    }

    // bind the server to the port
    if (bind(serverSocket, p->ai_addr, p->ai_addrlen) == -1) {
      close(serverSocket);
      continue;
    }
    break;
  }

  if (p == nullptr) {
    throw std::runtime_error("Failed to bind");
  }

  // listen on the server socket
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
void BootstrapServer::listenForConnections() {
  char buf[1024];
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
            std::string request(buf);
            json requestJson = json::parse(request);

            if (requestJson["command"] == "respSnapshot") {
              processSnapshotRequest(requestJson);
            } else if (requestJson["command"] == "respNotify") {
              processListFilesRequest(requestJson);
            } else if (requestJson["command"] == "respListFiles") {
              processPeerWithFileRequest(requestJson);
            } 
          }
        }
      }
    }
  }
}

void BootstrapServer::processSnapshotRequest(json requestJson) {
        /*
    // construct the response
    json responseSnapshotJson;
    responseSnapshotJson["command"] = "respSnapshot";
    responseSnapshotJson["data"] = snapshot;
    std::string responseSnapshotMessage = responseSnapshotJson.dump();

    // connect to the peer and send the response back
    connectToPeerServer(port, ip, name);
    sendMessage(name, responseSnapshotMessage);

    // send all other peers the new peer
    json responseNotifyJson;
    responseNotifyJson["command"] = "respNotify";
    responseNotifyJson["name"] = name;
    responseNotifyJson["ip"] = ip;
    responseNotifyJson["port"] = port;
    std::string responseNotifyMessage = responseNotifyJson.dump();
    for(auto &pair: connectedPeers) {
        if(pair.first != name) {
            sendMessage(pair.first, responseNotifyMessage);
        }
    }

    // add the new peer to our active peers 
    snapshot[name]["ip"] = ip;
    snapshot[name]["port"] = std::to_string(port);
    */

}

void BootstrapServer::processListFilesRequest(json requestJson) {
        /*
        std::vector<std::string> peerNames;
        for (const auto& item : filePeerList) {
                peerNames.push_back(std::get<0>(item));
        }

        // construct the response
        json responseListFilesJson;
        responseListFilesJson["command"] = "respListFiles";
        responseListFilesJson["data"] = peerNames;
        std::string responseListFilesMessage = responseListFilesJson.dump();

        // connect to the peer and send the response back
        sendMessage(name, responseListFilesMessage);
        */
}

void BootstrapServer::processPeerWithFileRequest(json requestJson) {
        return;
}



/*
* This function is used to connect to other the servers of other peers within the P2P network.
* After connecting, we save the connection to our internal state so that if we would like to contact
* this peer in the future, we do not have to reconnect to the server
*/
void BootstrapServer::connectToPeerServer(int port, const std::string &ip, std::string &peerServerName) {
        // create the socket
        int peerClientFd = socket(AF_INET, SOCK_STREAM, 0);
        if (peerClientFd < 0) {
                //logger.log("Failed to create socket", LogLevel::Error);
                return;
        }

        // set up the server that we want to connect to
        sockaddr_in sockaddr{};
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &sockaddr.sin_addr);

        // connect to the peer server specified by ip:port
        if (connect(peerClientFd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
                //logger.log("Unable to connect to the server", LogLevel::Error);
                close(peerClientFd);
                return;
        }

        // register the new connection
        connectedPeers[peerServerName] = peerClientFd;
}





void * BootstrapServer::get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void BootstrapServer::add_to_pfds(int newfd, std::vector<struct pollfd> &pfds) {
  if (pfds.size() == pfds.capacity()) {
    pfds.reserve(pfds.size() * 2);
  }
  pfds.push_back({newfd, POLLIN, 0});
}

void BootstrapServer::del_from_pfds(size_t index, std::vector<struct pollfd> &pfds) {
  std::swap(pfds[index], pfds.back());
  pfds.pop_back();
}












