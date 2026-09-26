/// workspace_timeline.hpp
/// State snapshot, undo/redo, timeline serialization

#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

// ============================================================================
// STATE SNAPSHOT
// ============================================================================

struct StateSnapshot {
    int64_t timestamp = 0;
    std::string action = "";
    std::map<std::string, std::string> editorState;
    std::map<std::string, std::string> chatState;
    std::map<std::string, std::string> fileSystemState;
    int snapshotIndex = 0;
};

// ============================================================================
// TIMELINE MANAGER
// ============================================================================

class TimelineManager {
public:
    TimelineManager();
    ~TimelineManager();

    // Capture current state
    void CaptureSnapshot(std::string action);

    // Navigate timeline
    void Undo();
    void Redo();
    void JumpToFrame(int frameIndex);

    // Query
    int GetCurrentFrame() const;
    int GetFrameCount() const;
    StateSnapshot GetSnapshot(int frameIndex) const;

    // Serialization
    std::string SerializeTimeline();
    void DeserializeTimeline(const std::string& json);

    // Persistence
    void SaveToFile(const std::string& filepath);
    void LoadFromFile(const std::string& filepath);

private:
    std::vector<StateSnapshot> timeline_;
    int currentFrame_ = -1;
    static const int MAX_TIMELINE_SIZE = 1000;

    void PruneIfNeeded();
    std::string SnapshotToJson(const StateSnapshot& snap);
    StateSnapshot JsonToSnapshot(const std::string& json);
};

// ============================================================================
// UNDO/REDO STACK
// ============================================================================

class UndoRedoStack {
public:
    void Push(std::string action, const StateSnapshot& state);
    StateSnapshot Undo();
    StateSnapshot Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    void Clear();

private:
    std::vector<StateSnapshot> undoStack_;
    std::vector<StateSnapshot> redoStack_;
    static const int MAX_STACK_SIZE = 100;
};
