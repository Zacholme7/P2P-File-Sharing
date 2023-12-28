#ifndef BOOTSTRAP_NODE_H
#define BOOTSTRAP_NODE_H

#include <string>
#include <mutex>
#include <unordered_map>

class BootstrapNode {
        public:
                void registerPeer(const std::string &peerAddress, int port);
                std::vector<std::string> getActivePeers();
                void deregisterPeer(const std::string &peerAddress);
        private:
                std::string serializeConnectedPeers(std::unordered_map<std::string, int> &connectedPeerMap);
                std::unordered_map<std::string, int> deserializeConnectedPeers(std::string &serializedConnectedPeerString);
                std::mutex mutex;
                std::unordered_map<std::string, int> connectedPeerMap;
};

#endif