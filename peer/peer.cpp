#include "peer.h"
#include "logger.h"
#include <sys/socket.h>
#include <thread>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

extern Logger logger;

namespace peer {
        /*
        * Constructor
        */
        Peer::Peer(std::string&name, int port)
                : name(name), port(port), serverSocket(-1) {
                        // establish connection to bootstrap server
                        std::string bootstrap = "bootstrap";
                        connectToPeerServer(50000, "127.0.0.1", bootstrap);
                };

        /*
        * Destructor
        */
        Peer::~Peer() {
                // send close message to all connected servers
                for(auto server: connectedServers) {
                        sendMessage(server.first, "respClientClosed");
                }
        };

        /*
        * This function is to start our peer. This will allow other peers to connect to this peer and 
        * send commands that need to be processed by the application
        */
        void Peer::startServer(int port) {
                // create the socket
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

                // start a new thread to listen to messages that are sent to this socket?
                int addrlen = sizeof(sockaddr);
                while (true) {
                        // accept the new connection
                        int newPeerClientFd = accept(serverSocket, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
                        if (newPeerClientFd < 0) {
                                logger.log("Failed to accept connection", LogLevel::Error);
                                continue;
                        }

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
                        std::cout << "waiting for message in peer" << std::endl;
                        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                        if (bytesRead <= 0) {
                                break;
                        }
                        buffer[bytesRead] = '\0'; // Null-terminate the received data
                        std::string command(buffer);

                        json j = json::parse(command);
                        std::cout << j.dump() << std::endl;

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
                            // 
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
                        logger.log("Unable to connect to the server", LogLevel::Error);
                        close(peerClientFd);
                        return;
                }

                std::cout << name << " connected to " << peerServerName << std::endl;
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
                if (command == "listfiles") {
                        sendMessage("bootstrap", "listFiles");
                } else if (command == "getfile") {
                        std::string filename;
                        std::cout << "What file would you to download: ";
                        getline(std::cin, filename);
                        payload = "getfile;filename";
                        sendMessage("bootstrap", payload);
                }
        }
}