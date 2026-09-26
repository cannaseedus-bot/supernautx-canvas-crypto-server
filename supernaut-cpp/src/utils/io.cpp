// src/utils/io.cpp
#include <fstream>
#include <iostream>
#include <string>

// File I/O utilities
bool file_exists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    return content;
}

void log_info(const std::string& message) {
    std::cout << "[INFO] " << message << "\n";
    std::cout.flush();
}

void log_error(const std::string& message) {
    std::cerr << "[ERROR] " << message << "\n";
    std::cerr.flush();
}
