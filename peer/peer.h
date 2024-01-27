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
  // Listen for activity on the peer server
  void listenForActivity();

  // Process communcation commands
  void processSnapshot(json responseJson);
  void processNotify(json responseJson);
  void processListFiles(json responseJson);
  void processPeerWithFile(json responseJson);
  void processRequestFile(json responseJson);

  // Handles peer disconnection
  void handleSocketClose(int port);

  // File transfer
  void sendFile(const std::string &filename, std::string &name);
  void receiveFile(const std::string &outputFilename, int sockfd);

  // Processes the message from the
  void connectToPeerServer(std::string &name, int port);

  // Internal helper function to send messages to another peer or bootstrap
  void sendMessage(const std::string &serverName, const std::string &payload);

  // Client information
  std::string name;
  std::vector<std::string> files;

  // Server information
  int serverSocket;
  int port;
  std::vector<struct pollfd> pfds;

  //  Keeps track of peer:socket information for communication
  std::unordered_map<std::string, int> nameToFd;
  std::unordered_map<int, std::string> fdToName;

  // Coordiantion for file transfer
  bool inFileTransferMode;
  std::string fileTransferOutputName;
};
} // namespace peer

#endif
