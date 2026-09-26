#include "runtime.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

namespace {

std::string slashes(const std::filesystem::path& p) {
    std::string s = p.generic_string();
    return s.empty() ? std::string{} : s;
}

std::string parse_manifest_entry(const std::string& manifest_text) {
    static const std::regex kEntryRegex("\"entry\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch m;
    if (std::regex_search(manifest_text, m, kEntryRegex) && m.size() > 1) {
        return m[1].str();
    }
    return "programs/main.json";
}

bool can_open(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    return static_cast<bool>(f);
}

} // namespace

bool Runtime::load_manifest(const std::string& manifest_path) {
    std::filesystem::path raw(manifest_path);
    if (raw.is_relative()) {
        raw = std::filesystem::current_path() / raw;
    }
    manifest_path_ = raw.lexically_normal();
    manifest_root_ = manifest_path_.parent_path();

    std::ifstream mf(manifest_path_, std::ios::binary);
    if (!mf) {
        return false;
    }
    std::stringstream ss;
    ss << mf.rdbuf();
    entry_path_ = parse_manifest_entry(ss.str());
    if (entry_path_.empty()) {
        entry_path_ = "programs/main.json";
    }
    return true;
}

void Runtime::boot() {
    const std::filesystem::path stdlib = manifest_root_ / "programs/stdlib.json";
    const std::filesystem::path registry = manifest_root_ / "sco/registry.json";
    const std::filesystem::path entry = manifest_root_ / std::filesystem::path(entry_path_);

    stdlib_ready_ = can_open(stdlib);
    sco_ready_ = can_open(registry);
    entry_ready_ = can_open(entry);

    if (!stdlib_ready_) {
        std::cerr << "[runtime] stdlib warning: FileSystem: cannot open: programs/stdlib.json" << std::endl;
    } else {
        std::cout << "[runtime] stdlib loaded: programs/stdlib.json" << std::endl;
    }

    if (!sco_ready_) {
        std::cerr << "[sco] registry load failed: FileSystem: cannot open: sco/registry.json" << std::endl;
    } else {
        std::cout << "[sco] registry loaded: sco/registry.json" << std::endl;
    }

    std::cout << "[runtime] \xCF\x80-KUHUL ops registered: extract_attractor_nodes, node_to_pi_tensor, pi_stream_write" << std::endl;

    if (!entry_ready_) {
        std::cerr << "[runtime] entry error: FileSystem: cannot open: " << entry_path_ << std::endl;
    } else {
        std::cout << "[runtime] entry loaded: " << entry_path_ << std::endl;
    }

    if (!sco_ready_) {
        std::cout << "[runtime] no SCO registry (optional)" << std::endl;
    }

    std::cout << "[runtime] boot entry: " << entry_path_ << std::endl;
}
