#include "bootstrapServer.h"
#include <unistd.h>
#include "../json.hpp"
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <netinet/in.h>
#include <arpa/inet.h>
using json = nlohmann::json;


/*
* This function will continuously listen for messages to be sent to the boostrap server from the connected peers.
* When it receives a message, it will decode it and then redirect it to the correct processing function. 
*/
void BootstrapServer::listenToPeer(int peerFd) {
        char buffer[1024];
        while (true) {
                // recieve data from the socket
                int bytesRead = recv(peerFd, buffer, sizeof(buffer) - 1, 0);
                if (bytesRead <= 0) {
                        break;
                }

                // convert the message into json
                buffer[bytesRead] = '\0';
                std::string requestMessage(buffer);
                json requestJson = json::parse(requestMessage);
                std::cout << requestJson.dump() << std::endl;

                // process the commands
                if (requestJson["command"] == "reqNewPeer") {
                    // parse the rest of the command
                    json data = requestJson["data"];
                    std::string name = data["name"];
                    std::string ip = data["ip"];
                    int port = data["port"];
                    std::vector<std::string> files = data["files"];

                    // insert the files into our filelist
                    for(const auto &fileName: files) {
                        filePeerList.push_back(std::make_tuple(fileName, name));
                    }
                    // process the new peer
                    processNewPeerCommand(name, ip, port, peerFd);
                } else if (requestJson["command"] == "reqListFiles") {

                        std::string name = requestJson["name"];
                        // process the command to list all of the available files
                        processListFilesCommand(name);
                }
        }

        // done with this peer, close it
        close(peerFd);
}

/*
* This function will process a new peer that connets to the bootstrap server. It will
* send out a snapshot of all the peers we currently have in the network
*/
void BootstrapServer::processNewPeerCommand(std::string &name, std::string &ip, int port, int peerFd) {
    // construct the response
    json responseSnapshotJson;
    responseSnapshotJson["command"] = "respSnapshot";
    responseSnapshotJson["data"] = snapshot;
    std::string responseSnapshotMessage = responseSnapshotJson.dump();

    // connect to the peer and send the response back
    connectToPeerServer(port, ip, name);
    sendMessage(name, responseSnapshotMessage);

    // send all other peers the new peer
    json responseNotifyJson;
    responseNotifyJson["command"] = "respNotify";
    responseNotifyJson["name"] = name;
    responseNotifyJson["ip"] = ip;
    responseNotifyJson["port"] = port;
    std::string responseNotifyMessage = responseNotifyJson.dump();
    for(auto &pair: connectedPeers) {
        if(pair.first != name) {
            sendMessage(pair.first, responseNotifyMessage);
        }
    }

    // add the new peer to our active peers 
    snapshot[name]["ip"] = ip;
    snapshot[name]["port"] = std::to_string(port);

}

/*
* This function is used to process the "listFiles" command from a Peer. It will
* go through all of our active files and send back a list of them to the peer. 
*/
void BootstrapServer::processListFilesCommand(std::string &name) {
        std::vector<std::string> peerNames;
        for (const auto& item : filePeerList) {
                peerNames.push_back(std::get<0>(item));
        }

        // construct the response
        json responseListFilesJson;
        responseListFilesJson["command"] = "respListFiles";
        responseListFilesJson["data"] = peerNames;
        std::string responseListFilesMessage = responseListFilesJson.dump();

        // connect to the peer and send the response back
        sendMessage(name, responseListFilesMessage);
}

/*
* This funciton is for sending a message to another peer in the P2P network. If we have not
* previoulsy connected to this peer and it is a new one, we will make sure to connect to and 
* register it before sending our message
*/
void BootstrapServer::sendMessage(const std::string &peerServerName, const std::string &payload) {
    send(connectedPeers[peerServerName], payload.c_str(), payload.size(), 0);
}

/*
* This function is used to connect to other the servers of other peers within the P2P network.
* After connecting, we save the connection to our internal state so that if we would like to contact
* this peer in the future, we do not have to reconnect to the server
*/
void BootstrapServer::connectToPeerServer(int port, const std::string &ip, std::string &peerServerName) {
        // create the socket
        int peerClientFd = socket(AF_INET, SOCK_STREAM, 0);
        if (peerClientFd < 0) {
                //logger.log("Failed to create socket", LogLevel::Error);
                return;
        }

        // set up the server that we want to connect to
        sockaddr_in sockaddr{};
        sockaddr.sin_family = AF_INET;
        sockaddr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &sockaddr.sin_addr);

        // connect to the peer server specified by ip:port
        if (connect(peerClientFd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
                //logger.log("Unable to connect to the server", LogLevel::Error);
                close(peerClientFd);
                return;
        }

        // register the new connection
        connectedPeers[peerServerName] = peerClientFd;
}


void BootstrapServer::processFileSearchCommand() {
        // get a message from the peer that says reqPeerWithFilej
        // decode the file
        // need some varifiction like response code to confirm 
}

void BootstrapServer::processPeerClose() {
        // get the message from the peer that says reqPeerClose
        // need to remove this peer from our map
        // need to remove this peers files
}
















