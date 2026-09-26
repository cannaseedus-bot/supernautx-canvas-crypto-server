#include "runtime.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    std::string manifest_path = "manifest.json";
    if (argc > 1) {
        const std::string arg1 = argv[1];
        if (arg1 == "--help" || arg1 == "-h") {
            std::cout << "Usage: skill_factory_engine [manifest.json]\n";
            std::cout << "Default manifest path: manifest.json (relative to current working directory)\n";
            return 0;
        }
        manifest_path = arg1;
    }

    std::cout << "--- ASXR MOS JSON Server ---" << std::endl;
    if (const char* dns = std::getenv("MOS_DNS"))  std::cout << "  [ENV] DNS:  " << dns << std::endl;
    if (const char* port = std::getenv("MOS_PORT")) std::cout << "  [ENV] PORT: " << port << std::endl;
    if (const char* host = std::getenv("MOS_HOST")) std::cout << "  [ENV] HOST: " << host << std::endl;

    Runtime runtime;
    if (!runtime.load_manifest(manifest_path)) {
        std::cerr << "[json_runtime] Failed to load manifest: " << manifest_path << std::endl;
        return 1;
    }

    std::cout << "[OK] Bootstrap Loaded. Booting..." << std::endl;
    runtime.boot();
    return 0;
}
