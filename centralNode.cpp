#include "centralNode.h"


// add a new peer to our map of active peers
void CentralNode::registerPeer(const std::string &peerAddress, int port) {
        std::lock_guard<std::mutex> lock(mutex);
        connectedPeers[peerAddress] = port;
}

// delete a peer from our map of active peers
void CentralNode::deregisterPeer(const std::string &peerAddress) {
        std::lock_guard<std::mutex> lock(mutex);

}


