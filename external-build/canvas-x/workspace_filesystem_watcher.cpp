/// workspace_filesystem_watcher.cpp
/// Win32 file system monitoring implementation

#include "workspace_filesystem_watcher.hpp"
#include <algorithm>
#include <chrono>

// ============================================================================
// DIRECTORY WATCHER IMPLEMENTATION
// ============================================================================

DirectoryWatcher::DirectoryWatcher(std::string watchPath)
    : watchPath_(watchPath) {}

DirectoryWatcher::~DirectoryWatcher() {
    StopWatch();
}

void DirectoryWatcher::StartWatch() {
    if (isWatching_) return;

    isWatching_ = true;
    stopEvent_ = CreateEventW(NULL, TRUE, FALSE, NULL);
    
    watchThread_ = std::thread(&DirectoryWatcher::WatchThreadProc, this);
}

void DirectoryWatcher::StopWatch() {
    if (!isWatching_) return;

    isWatching_ = false;
    if (stopEvent_) {
        SetEvent(stopEvent_);
    }

    if (watchThread_.joinable()) {
        watchThread_.join();
    }

    if (watchHandle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(watchHandle_);
        watchHandle_ = INVALID_HANDLE_VALUE;
    }

    if (stopEvent_) {
        CloseHandle(stopEvent_);
        stopEvent_ = NULL;
    }
}

void DirectoryWatcher::WatchThreadProc() {
    // Convert to wide string
    int size = MultiByteToWideChar(CP_UTF8, 0, watchPath_.c_str(), -1, NULL, 0);
    std::vector<wchar_t> widePath(size);
    MultiByteToWideChar(CP_UTF8, 0, watchPath_.c_str(), -1, widePath.data(), size);

    // Open directory
    HANDLE dirHandle = CreateFileW(
        widePath.data(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if (dirHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    watchHandle_ = dirHandle;

    // Buffer for changes
    char buffer[4096];
    DWORD bytesReturned = 0;
    HANDLE events[2] = { dirHandle, stopEvent_ };

    while (isWatching_) {
        DWORD waitResult = WaitForMultipleObjects(2, events, FALSE, 1000);

        if (waitResult == WAIT_OBJECT_0) {
            // Directory changed
            if (ReadDirectoryChangesW(
                dirHandle,
                buffer,
                sizeof(buffer),
                TRUE,  // recursive
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
                &bytesReturned,
                NULL,
                NULL
            )) {
                FILE_NOTIFY_INFORMATION* pNotify = (FILE_NOTIFY_INFORMATION*)buffer;

                while (pNotify) {
                    // Convert filename
                    int len = pNotify->FileNameLength / sizeof(wchar_t);
                    std::wstring wfilename(pNotify->FileName, len);
                    int mblen = WideCharToMultiByte(CP_UTF8, 0, wfilename.c_str(), -1, NULL, 0, NULL, NULL);
                    std::vector<char> mbfilename(mblen);
                    WideCharToMultiByte(CP_UTF8, 0, wfilename.c_str(), -1, mbfilename.data(), mblen, NULL, NULL);

                    FileChangeEvent event;
                    event.filepath = std::string(mbfilename.data());
                    event.timestamp = std::chrono::system_clock::now().time_since_epoch().count();

                    // Detect type
                    switch (pNotify->Action) {
                        case FILE_ACTION_ADDED:
                            event.type = FileChangeType::ADDED;
                            break;
                        case FILE_ACTION_REMOVED:
                            event.type = FileChangeType::DELETED;
                            break;
                        case FILE_ACTION_MODIFIED:
                            event.type = FileChangeType::MODIFIED;
                            break;
                        case FILE_ACTION_RENAMED_OLD_NAME:
                        case FILE_ACTION_RENAMED_NEW_NAME:
                            event.type = FileChangeType::RENAMED;
                            break;
                        default:
                            event.type = FileChangeType::MODIFIED;
                    }

                    if (OnFileChanged) {
                        OnFileChanged(event);
                    }

                    // Next entry
                    if (pNotify->NextEntryOffset == 0) {
                        break;
                    }
                    pNotify = (FILE_NOTIFY_INFORMATION*)((char*)pNotify + pNotify->NextEntryOffset);
                }
            }
        } else if (waitResult == WAIT_OBJECT_0 + 1) {
            // Stop event signaled
            break;
        }
    }

    CloseHandle(dirHandle);
    watchHandle_ = INVALID_HANDLE_VALUE;
}

std::vector<std::string> DirectoryWatcher::GetFileList() {
    std::vector<std::string> files;

    // Convert to wide string
    int size = MultiByteToWideChar(CP_UTF8, 0, watchPath_.c_str(), -1, NULL, 0);
    std::vector<wchar_t> widePath(size);
    MultiByteToWideChar(CP_UTF8, 0, watchPath_.c_str(), -1, widePath.data(), size);

    WIN32_FIND_DATAW findData;
    HANDLE findHandle = FindFirstFileW(widePath.data(), &findData);

    if (findHandle != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                int mblen = WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, NULL, 0, NULL, NULL);
                std::vector<char> mbfilename(mblen);
                WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, mbfilename.data(), mblen, NULL, NULL);
                files.push_back(std::string(mbfilename.data()));
            }
        } while (FindNextFileW(findHandle, &findData));

        FindClose(findHandle);
    }

    return files;
}

bool DirectoryWatcher::IsWatching() const {
    return isWatching_;
}

FileChangeType DirectoryWatcher::DetectChangeType(const std::string& filename) {
    return FileChangeType::MODIFIED;  // Simple fallback
}

// ============================================================================
// FILE SYSTEM WATCHER STATE
// ============================================================================

void FileSystemWatcherState::Initialize(std::string rootPath) {
    if (watcher) return;

    watcher = new DirectoryWatcher(rootPath);
    watcher->OnFileChanged = [this](const FileChangeEvent& event) {
        recentChanges.push_back(event);
        changeCount++;
        lastChangeTime = event.timestamp;
    };

    watcher->StartWatch();
}

void FileSystemWatcherState::Shutdown() {
    if (watcher) {
        watcher->StopWatch();
        delete watcher;
        watcher = nullptr;
    }
}

void FileSystemWatcherState::ProcessChanges() {
    // Process recent changes and update UI
    // This is called from main thread during Update()
    if (!recentChanges.empty()) {
        // UI would iterate recentChanges and update sidebar
        recentChanges.clear();
    }
}
