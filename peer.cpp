#include "peer.h"
#include "logger.h"
#include "util.h"
#include <sys/socket.h>
#include <thread>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <unistd.h>

extern Logger logger;

namespace peer {
        Peer::Peer(std::string&name )
                : name(name), serverSocket(-1) {};

        Peer::~Peer() {
                for (auto& thread: clientThreads) {
                        if (thread.joinable()) {
                                thread.join();
                        }
                }
                if (serverSocket != -1) {
                        close(serverSocket);
                }
                for (auto& pair : connectedPeers) {
                        close(pair.second);
                }
        };

        std::unordered_map<std::string, int> exchangePeerMap(int serverFd, int recvFd, std::unordered_map<std::string, int> connectedPeers) {
                // recieve the peer map from the client
                char buffer[1024] = {0};
                int valread = read(serverFd, buffer, 1024);
                std::string serializedMap(buffer, valread);
                auto peerMap = deserializeMap(serializedMap);

                // send the servers peer map
                std::string connectedPeersSerialized = serializeMap(connectedPeers);
                send(recvFd, connectedPeersSerialized.c_str(), connectedPeersSerialized.size(), 0);

                return peerMap;
        }


        void Peer::startServer(int port) {
                serverSocket = socket(AF_INET, SOCK_STREAM, 0);
                if (serverSocket == -1) {
                        logger.log("Failed to create the server", LogLevel::Error);
                        return;
                }

                sockaddr_in sockaddr{};
                sockaddr.sin_family = AF_INET;
                sockaddr.sin_addr.s_addr = INADDR_ANY;
                sockaddr.sin_port = htons(port);

                if (bind(serverSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
                        logger.log("Failed to bind to the port " + std::to_string(port), LogLevel::Error);
                        return;
                }

                listen(serverSocket, 10);
                logger.log("Server sucessfully created", LogLevel::Debug);

                // register the server with itself
                {
                        std::lock_guard<std::mutex> lock(mutex);
                        connectedPeers[this->name] = serverSocket;
                }

                int addrlen = sizeof(sockaddr);
                while(true) {
                        int clientSockfd = accept(serverSocket, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
                        if (clientSockfd < 0) {
                                logger.log("Failed to accept connection", LogLevel::Error);
                                continue;
                        }


                        // exchange peer maps with the client
                        auto newPeerMap = exchangePeerMap(serverSocket, clientSockfd, connectedPeers);

                        // process the new peer map, connect to any peers that we are not currently connected to
                        processPeerMap(newPeerMap);

                        // listen to the new client
                        std::thread clientThread(&Peer::listenToClient, this, clientSockfd);
                        clientThreads.push_back(std::move(clientThread));

                        /*
                        // send the connected client the server name 
                        send(clientSockfd, this->name.c_str(), this->name.length(), 0);

                        // recieve the clients peerList
                        char buffer[1024];
                        int bytesRead = recv(clientSockfd, buffer, sizeof(buffer) - 1, 0);
                        if (bytesRead > 0) {
                                buffer[bytesRead] = '\0';
                                std::string clientName(buffer);

                                // Register the connection with the server
                                std::lock_guard<std::mutex> lock(mutex);
                                connectedPeers[clientName] = clientSockfd;
                                logger.log(this->name + " registered connection from " + clientName, LogLevel::Info);
                        }
                        */

                }
        }



        void Peer::connectToPeer(int port, const std::string &ip) {
                int sock = socket(AF_INET, SOCK_STREAM, 0);
                if (sock < 0) {
                        logger.log("Failed to create socket", LogLevel::Error);
                        return;
                }

                sockaddr_in sockaddr{};
                sockaddr.sin_family = AF_INET;
                sockaddr.sin_port = htons(port);
                inet_pton(AF_INET, ip.c_str(), &sockaddr.sin_addr);

                if (connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
                        logger.log("Unable to connect to the server", LogLevel::Error);
                        close(sock);
                        return;
                }

                send(sock, this->name.c_str(), this->name.length(), 0);
                
                // record the connected peer for future communcation
                char buffer[1024];
                int bytesRead = recv(sock, buffer, sizeof(buffer) - 1, 0);
                if (bytesRead > 0) {
                        buffer[bytesRead] = '\0';
                        std::string serverName(buffer);
                        {
                                std::lock_guard<std::mutex> lock(mutex);
                                connectedPeers[serverName] = sock;
                                std::thread clientThread(&Peer::listenToClient, this, sock);
                                clientThreads.push_back(std::move(clientThread));
                        }
                        logger.log("Connected to server: " + serverName, LogLevel::Info);
                } else {
                        logger.log("Failed to recieve server name", LogLevel::Error);
                        close(sock);
                }
        }

        void Peer::sendMessage(const std::string &serverName, const std::string &message) {
                std::lock_guard<std::mutex> lock(mutex);
                auto it = connectedPeers.find(serverName);
                if (it != connectedPeers.end()) {
                        send(it->second, message.c_str(), message.size(), 0);
                } else {
                        logger.log("Server not found: " + serverName, LogLevel::Error);
                }
        }

        void Peer::listenToClient(int clientSocket) {
                char buffer[1024];
                while (true) {
                        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                        if (bytesRead <= 0) {
                                break;
                        }
                        buffer[bytesRead] = '\0'; // Null-terminate the received data
                        logger.log("Received message: " + std::string(buffer), LogLevel::Info);
                }
                close(clientSocket);
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

                if (command == "send") {
                        logger.log("Processig 'send message' command...", LogLevel::Info);
                        std::string message;
                        std::string serverName;

                        std::cout << "Enter the message that you would like to send: ";
                        std::getline(std::cin, message);
                        std::cout << "What is the name of the server you would like to send a message to: ";
                        std::getline(std::cin, serverName);

                        sendMessage(serverName, message);
                }
        }

}