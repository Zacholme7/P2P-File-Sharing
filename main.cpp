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

using namespace peer;

Logger logger(LogLevel::Debug);

int main(int argc, char* argv[]) {
        // parse the command line arguments
        if (argc < 3) {
                logger.log("Invalid arguments: please run in the form of p2papp {port} {sharedDirectory}", LogLevel::Error);
                return 1;
        }
        int port = std::stoi(argv[1]); // port that we want to the server to use
        std::string sharedDir = argv[2];

        // construct the peer
        Peer myPeer(port, sharedDir);

        // start the server in a new thread
        std::thread serverThread(&Peer::startServer, &myPeer, port);

        // get commands and process 
        std::string userInput;
        while (true) {
                std::cout << "Enter command: " << std::endl;
                std::getline(std::cin, userInput);

                if (userInput == "exit") {
                        break;
                }
                myPeer.processCommand(userInput);
        }

        serverThread.join();

        //myPeer.shutdown();
}





