#include <cerrno>
#include <cstring>
#include "./util.hpp"

std::string str_err() {
    return std::string(std::strerror(errno));
}