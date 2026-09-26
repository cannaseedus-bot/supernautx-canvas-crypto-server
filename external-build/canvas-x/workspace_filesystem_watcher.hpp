/// workspace_filesystem_watcher.hpp
/// Win32 file system monitoring (ReadDirectoryChangesW)

#pragma once

#include <string>
#include <vector>
#include <thread>
#include <functional>
#include <windows.h>

// ============================================================================
// FILE CHANGE EVENT
// ============================================================================

enum class FileChangeType {
    ADDED,
    MODIFIED,
    DELETED,
    RENAMED
};

struct FileChangeEvent {
    std::string filepath;
    FileChangeType type;
    int64_t timestamp;
};

// ============================================================================
// DIRECTORY WATCHER
// ============================================================================

class DirectoryWatcher {
public:
    DirectoryWatcher(std::string watchPath);
    ~DirectoryWatcher();

    // Start monitoring (blocks until StopWatch called)
    void StartWatch();
    void StopWatch();

    // Callback for file changes
    std::function<void(const FileChangeEvent&)> OnFileChanged;

    // Query current state
    std::vector<std::string> GetFileList();
    bool IsWatching() const;

private:
    std::string watchPath_;
    HANDLE watchHandle_ = INVALID_HANDLE_VALUE;
    HANDLE stopEvent_ = NULL;
    bool isWatching_ = false;
    std::thread watchThread_;

    void WatchThreadProc();
    FileChangeType DetectChangeType(const std::string& filename);
};

// ============================================================================
// FILE SYSTEM STATE INTEGRATION
// ============================================================================

struct FileSystemWatcherState {
    DirectoryWatcher* watcher = nullptr;
    std::vector<FileChangeEvent> recentChanges;
    int changeCount = 0;
    int64_t lastChangeTime = 0;

    void Initialize(std::string rootPath);
    void Shutdown();
    void ProcessChanges();
};
