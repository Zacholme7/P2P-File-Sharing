#ifndef BOOTSTRAP_SERVER_H
#define BOOTSTRAP_SERVER_H

#include "../json.hpp"
#include <mutex>
#include <string>
#include <sys/poll.h>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

class BootstrapServer {
public:
  void startServer(std::string &port);

private:
  // listen for a connection on the server
  void listenForConnections();

  // handle file descriptors
  void *get_in_addr(struct sockaddr *sa);
  void add_to_pfds(int newfd, std::vector<struct pollfd> &pfds);
  void del_from_pfds(size_t index, std::vector<struct pollfd> &pfds);

  // process incomming requests
  void processSnapshotRequest(json requestJson);
  void processListFilesRequest(json requestJson);
  void processPeerWithFileRequest(json requestJson);

  void handleSocketClose(int fd);

  // networking
  void connectToPeerServer(std::string &peerServerName, int port);

  void sendMessage(const std::string &peerServerName,
                   const std::string &payload);

  // data
  std::unordered_map<std::string, int> nameToPort;
  std::unordered_map<std::string, int> nameToFd;
  std::unordered_map<int, std::string> fdToName;
  std::unordered_map<std::string, std::vector<std::string>> nameToFiles;
  int serverSocket;
  std::vector<struct pollfd> pfds;
};

#endif
