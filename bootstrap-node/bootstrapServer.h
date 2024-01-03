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
                void processNewPeerCommand();
                void processListFilesCommand();
                void processFileSearchCommand();
                void processPeerClose();

                std::unordered_map<std::string, int> activePeers;
                std::unordered_map<std::string, std::vector<std::string>> fileMap;
};

#endif