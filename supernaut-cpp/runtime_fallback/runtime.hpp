#pragma once

#include <filesystem>
#include <string>

class Runtime {
public:
    bool load_manifest(const std::string& manifest_path);
    void boot();

private:
    std::filesystem::path manifest_path_;
    std::filesystem::path manifest_root_;
    std::string entry_path_ = "programs/main.json";
    bool stdlib_ready_ = false;
    bool sco_ready_ = false;
    bool entry_ready_ = false;
};

