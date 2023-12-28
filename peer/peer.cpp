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
        /*
        * Constructor
        */
        Peer::Peer(std::string&name )
                : name(name), serverSocket(-1) {};

        /*
        * Destructor
        */
        Peer::~Peer() {};


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
                        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                        if (bytesRead <= 0) {
                                break;
                        }
                        buffer[bytesRead] = '\0'; // Null-terminate the received data
                        std::string command(buffer);
                        //  the various commands that a peer has to process from another peer
                }
                close(clientSocket);
        }

        /*
        * This function is used to connect to other the servers of other peers within the P2P network.
        * After connecting, we save the connection to our internal state so that if we would like to contact
        * this peer in the future, we do not have to reconnect to the server
        */
        void Peer::connectToPeerServer(int port, const std::string &ip, std::string &peerServerName) {
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

                // send a new_client_connection message to the server so that this client can be saved
                send(peerClientFd, this->name.c_str(), this->name.length(), 0);

                // register this connection in our peer
                connectedServers[peerServerName] = peerClientFd;
        }

        /*
        * This funciton is for sending a message to another peer in the P2P network. If we have not
        * previoulsy connected to this peer and it is a new one, we will make sure to connect to and 
        * register it before sending our message
        */
        void Peer::sendMessage(const std::string &peerServerName, const std::string &command) {
                // look if we are conncected to the peerServer that we want to connect to
                // if we are not, conenct to the peerServer
                // once we are connected, send the command
                /*
                auto it = connectedPeers.find(peerServerName);
                if (command == "bootstrap") {
                }
                auto it = connectedPeers.find(serverName);
                if (it != connectedPeers.end()) {
                        send(it->second, message.c_str(), message.size(), 0);
                } else {
                        logger.log("Server not found: " + serverName, LogLevel::Error);
                }
                */
        }

        /*
        * This function will process commands from the cli and
        * redirect it to the proper execution functions in the peer
        */
        void Peer::processCommand(std::string& command) {
                return;
        }
}