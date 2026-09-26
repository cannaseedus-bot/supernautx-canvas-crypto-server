/// workspace_timeline.cpp
/// Timeline and undo/redo implementation

#include "workspace_timeline.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>

// ============================================================================
// TIMELINE MANAGER
// ============================================================================

TimelineManager::TimelineManager() {
    currentFrame_ = -1;
}

TimelineManager::~TimelineManager() {}

void TimelineManager::CaptureSnapshot(std::string action) {
    StateSnapshot snap;
    snap.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    snap.action = action;
    snap.snapshotIndex = (int)timeline_.size();

    timeline_.push_back(snap);
    currentFrame_ = snap.snapshotIndex;

    PruneIfNeeded();
}

void TimelineManager::Undo() {
    if (currentFrame_ > 0) {
        currentFrame_--;
    }
}

void TimelineManager::Redo() {
    if (currentFrame_ < (int)timeline_.size() - 1) {
        currentFrame_++;
    }
}

void TimelineManager::JumpToFrame(int frameIndex) {
    if (frameIndex >= 0 && frameIndex < (int)timeline_.size()) {
        currentFrame_ = frameIndex;
    }
}

int TimelineManager::GetCurrentFrame() const {
    return currentFrame_;
}

int TimelineManager::GetFrameCount() const {
    return (int)timeline_.size();
}

StateSnapshot TimelineManager::GetSnapshot(int frameIndex) const {
    if (frameIndex >= 0 && frameIndex < (int)timeline_.size()) {
        return timeline_[frameIndex];
    }
    return StateSnapshot();
}

std::string TimelineManager::SerializeTimeline() {
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"frameCount\": " << timeline_.size() << ",\n";
    ss << "  \"currentFrame\": " << currentFrame_ << ",\n";
    ss << "  \"frames\": [\n";

    for (size_t i = 0; i < timeline_.size(); i++) {
        ss << "    " << SnapshotToJson(timeline_[i]);
        if (i < timeline_.size() - 1) ss << ",";
        ss << "\n";
    }

    ss << "  ]\n";
    ss << "}\n";

    return ss.str();
}

void TimelineManager::DeserializeTimeline(const std::string& json) {
    // Simple CSV-like parsing (MVP - full JSON parser in Phase 7.20)
    timeline_.clear();
    currentFrame_ = 0;
}

void TimelineManager::SaveToFile(const std::string& filepath) {
    std::ofstream file(filepath);
    if (file.is_open()) {
        file << SerializeTimeline();
        file.close();
    }
}

void TimelineManager::LoadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        DeserializeTimeline(ss.str());
        file.close();
    }
}

void TimelineManager::PruneIfNeeded() {
    if ((int)timeline_.size() > MAX_TIMELINE_SIZE) {
        // Remove oldest frames
        timeline_.erase(timeline_.begin(), timeline_.begin() + 100);
        currentFrame_ = std::max(0, currentFrame_ - 100);
    }
}

std::string TimelineManager::SnapshotToJson(const StateSnapshot& snap) {
    std::stringstream ss;
    ss << "{";
    ss << "\"index\":" << snap.snapshotIndex << ",";
    ss << "\"action\":\"" << snap.action << "\",";
    ss << "\"time\":" << snap.timestamp;
    ss << "}";
    return ss.str();
}

StateSnapshot TimelineManager::JsonToSnapshot(const std::string& json) {
    StateSnapshot snap;
    // MVP: basic parsing
    return snap;
}

// ============================================================================
// UNDO/REDO STACK
// ============================================================================

void UndoRedoStack::Push(std::string action, const StateSnapshot& state) {
    undoStack_.push_back(state);
    redoStack_.clear();  // Clear redo stack when new action

    if ((int)undoStack_.size() > MAX_STACK_SIZE) {
        undoStack_.erase(undoStack_.begin());
    }
}

StateSnapshot UndoRedoStack::Undo() {
    if (undoStack_.empty()) {
        return StateSnapshot();
    }

    StateSnapshot state = undoStack_.back();
    undoStack_.pop_back();
    redoStack_.push_back(state);

    if (undoStack_.empty()) {
        return StateSnapshot();
    }

    return undoStack_.back();
}

StateSnapshot UndoRedoStack::Redo() {
    if (redoStack_.empty()) {
        return StateSnapshot();
    }

    StateSnapshot state = redoStack_.back();
    redoStack_.pop_back();
    undoStack_.push_back(state);

    return state;
}

bool UndoRedoStack::CanUndo() const {
    return !undoStack_.empty();
}

bool UndoRedoStack::CanRedo() const {
    return !redoStack_.empty();
}

void UndoRedoStack::Clear() {
    undoStack_.clear();
    redoStack_.clear();
}
