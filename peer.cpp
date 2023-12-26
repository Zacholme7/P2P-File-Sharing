#include "peer.h"
#include "logger.h"
#include <sys/socket.h>
#include <thread>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <string>
#include <iostream>
#include <unistd.h>


extern Logger logger;

namespace peer {
        Peer::Peer(int port, std::string sharedDir) 
                : port(port), sharedDir(sharedDir) {};

        Peer::~Peer() {};

        // Starts up the server portion of the peer
        // Will accept connections from other peers
        void Peer::startServer(int port) {
                logger.log("Creating the server...", LogLevel::Debug);
                // create the socket (IPv4, TCP)
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd == -1) {
                        logger.log("Failed to create the server", LogLevel::Error);
                        exit(EXIT_FAILURE);
                }

                // Listen to the passed port on any address
                sockaddr_in sockaddr;
                sockaddr.sin_family = AF_INET;
                sockaddr.sin_addr.s_addr = INADDR_ANY;
                sockaddr.sin_port = htons(port);

                // bind the file descriptor to the address
                logger.log("Binding the server...", LogLevel::Debug);
                if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
                        logger.log("Failed to bind to the port " + std::to_string(port), LogLevel::Error);
                        exit(EXIT_FAILURE);
                }

                listen(sockfd, 10);

                logger.log("Server successfully created", LogLevel::Debug);

                int addrlen = sizeof(sockaddr);
                while(true) {
                        logger.log("Listening for new connections...", LogLevel::Debug);
                        int clientSockfd = accept(sockfd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
                        logger.log("Got a new connection!", LogLevel::Debug);

                        std::thread clientThread(&Peer::listenToClient, this, clientSockfd);



                        //logger.log("Closing the new connection", LogLevel::Debug);
                        //close(clientSockfd);
                }
        }

        void Peer::listenToClient(int clientSocket) {
                while(true) {
                        char buffer[1024];
                        int bytesRead = recv(clientSocket, buffer, 1024, 0);
                        buffer[bytesRead] = '\0';
                        std::string clientMessage(buffer);
                        std::cout << clientMessage << std::endl;

                }
        }

        // Processes commands from the cli
        void Peer::processCommand(std::string& command) {
                if (command == "connect") {
                        logger.log("Processing 'connect' command...", LogLevel::Debug);
                        std::string ip;
                        std::string port;

                        std::cout << "Enter the IP that you would like to connect to: ";
                        std::getline(std::cin, ip);
                        std::cout << "Enter the port that you would like to connect to: ";
                        std::getline(std::cin, port);

                        connectToPeer(std::stoi(port), ip);
                }
        }

        // connects to another peer
        // peer acts as a client here
        void Peer::connectToPeer(int port, std::string &ip) {
                logger.log("Connecting to a server...", LogLevel::Debug);

                int sock = socket(AF_INET, SOCK_STREAM, 0);

                // define the server address
                sockaddr_in sockaddr;
                sockaddr.sin_family = AF_INET;
                sockaddr.sin_port = htons(port);
                inet_pton(AF_INET, ip.c_str(), &sockaddr.sin_addr);

                if (connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
                        logger.log("Unable to connect to the server", LogLevel::Error);
                        exit(EXIT_FAILURE);
                }

                std::string message = "hello world";
                send(sock, message.c_str(), message.size(), 0);

                // close the socket
                close(sock);
        }
}