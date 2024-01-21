#include "logger.h"
#include "peer.h"
#include <arpa/inet.h>
#include <atomic>
#include <filesystem>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <atomic>

using namespace peer;
namespace fs = std::filesystem;

std::atomic<bool> isRunning(true);

Logger logger(LogLevel::None);

int main(int argc, char *argv[]) {
  // parse the command line arguments
  if (argc < 3) {
    logger.log("Invalid arguments: please run in the form of p2papp {port} "
               "{peer name}",
               LogLevel::Error);
    return 1;
  }
  // int port = std::stoi(argv[1]); // port that we want to the server to use
  int port = std::stoi(argv[1]);
  std::string peerName = std::string(argv[2]);

  // open the file directory and save the filenames
  std::string folderName = "files";
  std::vector<std::string> fileNames;
  if (fs::exists(folderName) && fs::is_directory(folderName)) {
    for (const auto &entry : fs::directory_iterator(folderName)) {
      fileNames.push_back(entry.path().filename());
    }
  }

  // construct the peer and start it in a new thread
  Peer myPeer(peerName, port, fileNames);
  std::thread serverThread(&Peer::startServer, &myPeer, port);

  std::cout << "Enter a command: " << std::endl;
  while (true) {
    std::cout << "- ";
    std::string command;
    std::getline(std::cin, command);

    if (command == "help") {
        std::cout << "listFiles: lists all of the files available on the network" << std::endl;
        std::cout << "getFile: get a file from the network " << std::endl;
    }

    if (command == "exit") {
      break;
    }
    myPeer.processCommand(command);

  }
  return 0;
}
