#include "bootstrapserver.h"
#include <sys/socket.h>
#include <unistd.h>


void BootstrapServer::listenToPeer(int peerFd) {
        char buffer[1024];
        while (true) {
                int bytesRead = recv(peerFd, buffer, sizeof(buffer) - 1, 0);
                if (bytesRead <= 0) {
                        break;
                }
                buffer[bytesRead] = '\0'; // Null-terminate the received data
                std::string command(buffer);
                //  the various commands that a peer has to process from another peer

                // what are the various things we need to listen for
                // a client close
                // sending snapshot out
                // list files?


                if (command == "reqNewPeer") {
                        processNewPeerCommand();
                } else if (command == "reqListFiles") {
                        processListFilesCommand();
                } else if (command == "reqPeerWithFile") {
                        processFileSearchCommand();
                }
        }
        close(peerFd);
}


void BootstrapServer::processNewPeerCommand() {
        // What are we needing to do here
        // get a message from the peer that says "reqNewPeer"
        // What do we need from this peer?
        // we need their name, and their file list
        // once we have the name and filelist
        // save the list of files
        // send a snapshot of the currently connected peers
        // need command like respNewPeer then the data, needt his in json??
        // save the name and the socket so we can communicate with it
        // we do this after so that we dont include it in the message
        // this will register the new peer
}

void BootstrapServer::processListFilesCommand() {
        // get a message from the peer that says reqFileList
        // loop through the filelist and create a vector that contains all of the files
        // transform it into json or some sort and send it back
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
















