// util.cpp
// helpful utility functions
#ifndef UTIL_H
#define UTIL_H

#include <unordered_map>
#include <string>

// function to serialize a map into a string for sending over a network
std::string serializeMap(const std::unordered_map<std::string, int> &map) {
        std::string serializedMap;
        for(const auto &pair: map) {
                serializedMap += pair.first + ":" + std::to_string(pair.second) + ";";
        }
        return serializedMap;
}

// function to deserialize a string to a map
std::unordered_map<std::string, int> deserializeMap(std::string& serializedMap) {
    std::unordered_map<std::string, int> map;
    size_t pos = 0;
    while ((pos = serializedMap.find(';')) != std::string::npos) {
        std::string token = serializedMap.substr(0, pos);
        size_t sepPos = token.find(':');
        if (sepPos != std::string::npos) {
            std::string key = token.substr(0, sepPos);
            int value = std::stoi(token.substr(sepPos + 1));
            map[key] = value;
        }
        serializedMap.erase(0, pos + 1);
    }
    return map;
}

#endif