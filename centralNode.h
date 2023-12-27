#ifndef CENTRAL_NODE_H
#define CENTRAL_NODE_H

#include <string>
#include <mutex>
#include <unordered_map>

class CentralNode {
        public:
                void registerPeer(const std::string &peerAddress, int port);
                std::vector<std::string> getActivePeers();
                void deregisterPeer(const std::string &peerAddress);
        private:
                std::mutex mutex;
                std::unordered_map<std::string, int> connectedPeers;





}


#endif