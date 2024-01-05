#ifndef BOOTSTRAP_SERVER_H
#define BOOTSTRAP_SERVER_H

#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <thread>

class BootstrapServer {
        public:
                void listenToPeer(int peerFd);
                std::vector<std::thread> peerThreads;
        private:
                // commands
                void processNewPeerCommand(std::string &name, std::string &value, int port, int peerFd);
                void processListFilesCommand();
                void processFileSearchCommand();
                void processPeerClose();

                // networking 
                void connectToPeerServer(int port, const std::string &ip, std::string &peerServerName);
                void sendMessage(const std::string &peerServerName, const std::string &payload);

                std::unordered_map<std::string, std::unordered_map<std::string, std::string>> snapshot;
                std::unordered_map<std::string, int> connectedPeers;
                std::unordered_map<std::string, std::vector<std::string>> fileMap;
};

#endif