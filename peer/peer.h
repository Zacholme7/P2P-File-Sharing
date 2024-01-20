#ifndef INCLUDED_PEER
#define INCLUDED_PEER

#include "../json.hpp"
#include <mutex>
#include <poll.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

namespace peer {
class Peer {
public:
  Peer(std::string &name, int, std::vector<std::string> &files);
  ~Peer();

  // Starts the server
  void startServer(int port);

  // Contacts the bootstrap server to get initial snapshot
  void requestSnapshot();

  // Processes a command from the cli
  void processCommand(std::string &command);

private:
  void listenForConnections();
  // Processes the initial snapshot message from the bootstrap with all current
  // peers
  void processBootstrapSnapshot();

  void processSnapshot(json responseJson);
  void processNotify(json responseJson);
  void processListFiles(json responseJson);

  // Processes a file request
  void processFileRequest();
  void requestFileRequest();

  // Handles list file request
  void processAllFiles();
  void processPeerWithFile(json responseJson);

  void requestAllFiles();

  void handleSocketClose(int port);

  // Handles file transfer
  void sendFile(const std::string &filename, int socket);
  void recieveFile(const std::string &outputFilename, int sockfd);

  // Processes the message from the
  void connectToPeerServer(std::string &name, int port);

  // Internal helper function to send messages to another peer or bootstrap
  // server
  void sendMessage(const std::string &serverName, const std::string &payload);
  void listenForIncommingConnection(int serverSocket);

  // Will continuoulsy listen form messages from a connected peer
  void listenToClient(int clientSocket);

  std::string name;
  int serverSocket;
  std::unordered_map<std::string, int> connectedServers;
  std::unordered_map<std::string, int> nameToFd;
  std::unordered_map<int, std::string> fdToName;

  std::vector<std::thread> clientThreads;
  std::mutex mutex;
  int port;
  std::vector<std::string> files;
  std::vector<struct pollfd> pfds;
};
} // namespace peer

#endif
