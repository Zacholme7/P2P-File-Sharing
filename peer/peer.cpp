#include "peer.h"
#include "logger.h"
#include <sys/socket.h>
#include <thread>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include "../json.hpp"

using json = nlohmann::json;

extern std::atomic<bool> isRunning;
extern Logger logger;

namespace peer {
        /*
        * Constructor
        */
        Peer::Peer(std::string&name, int port, std::vector<std::string> &files)
                : name(name), port(port), serverSocket(-1), files(files) {
                        // establish connection to bootstrap server
                        std::string bootstrap = "bootstrap";
                        connectToPeerServer(50000, "127.0.0.1", bootstrap);
                };

        /*
        * Destructor
        */
        Peer::~Peer() {
                // send close message to all connected servers
                std::cout << "the peer is being destoryed" << std::endl;
                for(auto server: connectedServers) {
                        std::cout << server.first << std::endl;
                        sendMessage(server.first, "respClientClosed");
                }
        };

        /*
        * This function is to start our peer. This will allow other peers to connect to this peer and 
        * send commands that need to be processed by the application
        */
        void Peer::startServer(int port) {
                // create the socket

                logger.log("Starting the server...", LogLevel::Debug);
                serverSocket = socket(AF_INET, SOCK_STREAM, 0);
                if (serverSocket == -1) {
                        logger.log("Failed to create the server", LogLevel::Error);
                        return;
                }

                // define the address information
                sockaddr_in sockaddr{};
                sockaddr.sin_family = AF_INET;
                sockaddr.sin_addr.s_addr = INADDR_ANY;
                sockaddr.sin_port = htons(port);

                // bind the socket to the server address
                if (bind(serverSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
                        logger.log("Failed to bind to the port " + std::to_string(port), LogLevel::Error);
                        return;
                }

                // begin listening on this socket
                listen(serverSocket, 10);
                logger.log("Successfully started the server", LogLevel::Debug);

                // start a new thread to listen to messages that are sent to this socket?
                int addrlen = sizeof(sockaddr);
                while (true && isRunning) {
                        // accept the new connection
                        logger.log("Server waiting for a new connection...", LogLevel::Debug);
                        int newPeerClientFd = accept(serverSocket, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
                        if (newPeerClientFd < 0) {
                                logger.log("Failed to accept connection", LogLevel::Error);
                                continue;
                        }
                        logger.log("Server got a new connection, starting new thread...", LogLevel::Debug);

                        // listen to this connection in its own thread
                        std::thread newPeerClientThread(&Peer::listenToClient, this, newPeerClientFd);
                        clientThreads.push_back(std::move(newPeerClientThread));
                }
        }

        /*
        * This function will continuously listen for messages to be send to the peer from the connected client
        * When it recieves a message, it will decode it and then redirect it to the correct processing function
        */
        void Peer::listenToClient(int clientSocket) {
                char buffer[1024];
                while (true) {
                        std::string logString = "Listening for a message on socket " + std::to_string(clientSocket);
                        logger.log(logString, LogLevel::Debug);
                        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                        if (bytesRead <= 0) {
                                break;
                        }
                        buffer[bytesRead] = '\0'; // Null-terminate the received data
                        std::string command(buffer);

                        json j = json::parse(command);
                        logString = "Got a message on socket " + std::to_string(clientSocket) + ": " + j.dump();
                        logger.log(logString, LogLevel::Debug);

                        // what are we needing to do here?
                        if (j["command"] == "respSnapshot") {
                            // get all of the active peers
                            json peers = j["data"];

                            // go through all of the peers and connect to them
                            for (auto &[peerName, peerDetails] : peers.items()) {
                                std::string ip = peerDetails["ip"];
                                std::string port = peerDetails["port"];
                                connectToPeerServer(std::stoi(port), ip, peerName);
                            }

                        } else if (command == "respNotify") {
                                // parse the peer data
                                std::string peerName = j["name"];
                                std::string ip = j["ip"];
                                std::string port = j["port"];
                                
                                // connect to the peer
                                connectToPeerServer(std::stoi(port), ip, peerName);
                        } else if (command == "respListFiles") {
                                for(auto &fileName : j["data"].items()) {
                                        std::cout << fileName << " ";
                                }
                                std::cout << std::endl;
                        }
                }
                close(clientSocket);
        }

        /*
        * This function is used to connect to other the servers of other peers within the P2P network.
        * After connecting, we save the connection to our internal state so that if we would like to contact
        * this peer in the future, we do not have to reconnect to the server
        */
        void Peer::connectToPeerServer(int port, const std::string &ip, const std::string &peerServerName) {
                // create the socket
                logger.log("Trying to connect to " + peerServerName + "...", LogLevel::Debug);
                int peerClientFd = socket(AF_INET, SOCK_STREAM, 0);
                if (peerClientFd < 0) {
                        logger.log("Failed to create socket", LogLevel::Error);
                        return;
                }

                // set up the server that we want to connect to
                sockaddr_in sockaddr{};
                sockaddr.sin_family = AF_INET;
                sockaddr.sin_port = htons(port);
                inet_pton(AF_INET, ip.c_str(), &sockaddr.sin_addr);

                // connect to the peer server specified by ip:port
                if (connect(peerClientFd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
                        logger.log("Unable to connect to " + peerServerName, LogLevel::Error);
                        close(peerClientFd);
                        return;
                }
                logger.log("Successfully connected to " + peerServerName, LogLevel::Debug);

                // register the new connection
                connectedServers[peerServerName] = peerClientFd;
        }

        /*
        * This fuction sends a new peer message to the bootstrap server. This is the peers first 
        * contact with the bootstrap and it will send a snapshot of all the current peers back to
        * be processed
        */
        void Peer::contactBootstrap() {
            // construct the boostrap message
            json requestJson;
            requestJson["command"] = "reqNewPeer"; 

            json data;
            data["name"] = name; 
            data["port"] = port;
            data["ip"] = "127.0.0.1";
            data["files"] = files;

            requestJson["data"] = data;
            std::string requestMessage = requestJson.dump();

            sendMessage("bootstrap", requestMessage);
        }

        /*
        * This funciton is for sending a message to another peer in the P2P network. If we have not
        * previoulsy connected to this peer and it is a new one, we will make sure to connect to and 
        * register it before sending our message
        */
        void Peer::sendMessage(const std::string &peerServerName, const std::string &payload) {
                auto it = connectedServers.find(peerServerName);
                if(it !=connectedServers.end())  {
                        send(it->second, payload.c_str(), payload.size(), 0);
                }
        }

        /*
        * This function will process commands from the cli and
        * redirect it to the proper execution functions in the peer
        */
        void Peer::processCommand(std::string& command) {
                std::string payload;
                if (command == "listFiles") {
                        // construct the listFiles message
                        json requestJson;
                        requestJson["command"] = "reqListFiles"; 
                        requestJson["name"] = name;
                        std::string requestMessage = requestJson.dump();

                        // send the message to the bootstrap server
                        sendMessage("bootstrap", requestMessage);
                } else if (command == "getfile") {
                        std::string filename;
                        std::cout << "What file would you to download: ";
                        getline(std::cin, filename);
                        payload = "getfile;filename";
                        sendMessage("bootstrap", payload);
                }
        }
}