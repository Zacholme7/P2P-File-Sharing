# P2P-File-Sharing
This local peer-to-peer file sharing application enables file transfers within a home network. It's designed for simplicity and ease of use, making it a great tool for sharing files among multiple devices in a local environment.

## How it works
The system architecture, while unique, maintains the general premise of traditional peer-to-peer networking. Coordination is managed by a bootstrap node, which acts as the master controller. New peers, upon joining the network, contact this bootstrap node to request a snapshot of current nodes in the network. After connecting to these peers, the bootstrap node notifies existing peers about the new addition. A key function of the bootstrap is to maintain a master file list, which is used to respond to queries about file availability and location within the network.

## How to use
1) Clone the Repository:
```
git clone https://github.com/Zacholme7/P2P-File-Sharing
cd P2P-File-Sharing
```
2) Build the Peer and Bootstrap:
```
cd bootstrap && make
cd ../peer && make
```
3) Start the Bootstrap
```
./boostrap
```
4) Prepare the Files
Create a 'files' folder in the peer directory with the files you want to share.
```
mkdir files && touch files/hello.txt
```
5) Start the Peer
Replace {port} and {peer name} with your desired settings.
```
./myapp {port} {peer name}
```
6) Repeat:
Start peers on different machines or directories to expand the network.

## Final Thoughtss
This project was a refreshing dive back into C++ programming and served as an excellent introduction to network programming. Although it doesn't match the complexity or scale of commercial peer-to-peer file sharing systems, it offered a solid learning experience in understanding core concepts and solving challenging design and network-related problems.




