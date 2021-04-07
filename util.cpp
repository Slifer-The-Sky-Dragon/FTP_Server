#include <cerrno>
#include <cstring>
#include "./util.hpp"

std::string str_err() {
    return std::string(std::strerror(errno));
}

std::vector<std::string> tokenize(std::string line, char seperator) {
    int start = 0;
    std::string token;
    std::vector<std::string> tokens;
    for (size_t i = 0; i < line.length(); ++i) {
        if (i == line.length() - 1 && line[i] != seperator) {
            token = line.substr(start, i - start + 1);
            tokens.push_back(token);
        }
        else {
            if(line[i] == seperator){
            token = line.substr(start, i - start);
            start = i + 1;
            tokens.push_back(token);
            }
        }
    }
    return tokens;
}