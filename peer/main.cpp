#include "peer.h"
#include "logger.h"
#include <string>
#include <arpa/inet.h>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <filesystem>
#include <atomic>

using namespace peer;
namespace fs = std::filesystem;

std::atomic<bool> isRunning(true);

Logger logger(LogLevel::Debug);

int main(int argc, char* argv[]) {
        // parse the command line arguments
        if (argc < 3) {
                logger.log("Invalid arguments: please run in the form of p2papp {port} {peer name}", LogLevel::Error);
                return 1;
        }
        int port = std::stoi(argv[1]); // port that we want to the server to use
        std::string peerName = std::string(argv[2]);

        // open the file directory and save the filenames
        std::string folderName = "files";
        std::vector<std::string> fileNames;
        if (fs::exists(folderName) && fs::is_directory(folderName)) {
                for (const auto& entry : fs::directory_iterator(folderName)) {
                        fileNames.push_back(entry.path().filename());
                }
        }


        // construct the peer
        Peer myPeer(peerName, port, fileNames);

        // start the server in a new thread
        std::thread serverThread(&Peer::startServer, &myPeer, port);

        // Send initial message to the bootstrap server
        myPeer.contactBootstrap();

        // get commands and process 
        std::string userInput;
        bool cond = true;
        while (cond) {
                std::getline(std::cin, userInput);
                std::cout << "User input got: " << userInput << std::endl;

                if (userInput == "exit") {
                        break;
                }
                myPeer.processCommand(userInput);
        }

        std::cout << "out of the loop" << std::endl;
        isRunning = false;

        serverThread.join();
}





