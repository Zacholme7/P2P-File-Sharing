#ifndef BOOTSTRAP_SERVER_H
#define BOOTSTRAP_SERVER_H

#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <tuple>
#include "../json.hpp"
using json = nlohmann::json;

class BootstrapServer {
        public:
                void startServer(std::string &port);
        private:
                void listenForConnections();

                void *get_in_addr(struct sockaddr *sa);
                void add_to_pfds(int newfd, std::vector<struct pollfd> &pfds);
                void del_from_pfds(size_t index, std::vector<struct pollfd> &pfds);

                void processSnapshotRequest(json requestJson);
                void processListFilesRequest(json requestJson);
                void processPeerWithFileRequest(json requestJson);




                // networking 
                void connectToPeerServer(int port, const std::string &ip, std::string &peerServerName);
                void sendMessage(const std::string &peerServerName, const std::string &payload);

                // data
                std::unordered_map<std::string, std::unordered_map<std::string, std::string>> snapshot;
                std::unordered_map<std::string, int> connectedPeers;
                std::vector<std::tuple<std::string, std::string>> filePeerList;
                int serverSocket;
                std::vector<struct pollfd> pfds;
};

#endif