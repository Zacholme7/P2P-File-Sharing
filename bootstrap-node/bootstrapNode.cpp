#include "bootstrapNode.h"


// add a new peer to our map of active peers
void BootstrapNode::registerPeer(const std::string &peerAddress, int port) {
        std::lock_guard<std::mutex> lock(mutex);
        connectedPeerMap[peerAddress] = port;
}

// delete a peer from our map of active peers
void BootstrapNode::deregisterPeer(const std::string &peerAddress) {
        std::lock_guard<std::mutex> lock(mutex);
}


