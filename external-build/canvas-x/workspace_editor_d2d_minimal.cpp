// workspace_editor_d2d_minimal.cpp
// Minimal editor implementation for Phase 7.19 MVP

#include "workspace_editor_d2d_minimal.hpp"

// Only implement non-inline methods or those with complex logic
// Inline methods are defined in the header

void D2DEditor::InsertChar(char c) {
    if (lines.empty()) lines.push_back("");
    if (cursor.y >= lines.size()) cursor.y = lines.size() - 1;
    lines[cursor.y].insert(cursor.x, 1, c);
    cursor.x++;
}

void D2DEditor::Backspace() {
    if (cursor.x > 0 && cursor.y < lines.size()) {
        lines[cursor.y].erase(cursor.x - 1, 1);
        cursor.x--;
    }
}

void D2DEditor::Delete() {
    if (cursor.y < lines.size() && cursor.x < lines[cursor.y].size()) {
        lines[cursor.y].erase(cursor.x, 1);
    }
}

void D2DEditor::NewLine() {
    if (lines.empty()) lines.push_back("");
    if (cursor.y >= lines.size()) cursor.y = lines.size() - 1;
    std::string rest = lines[cursor.y].substr(cursor.x);
    lines[cursor.y].erase(cursor.x);
    lines.insert(lines.begin() + cursor.y + 1, rest);
    cursor.y++;
    cursor.x = 0;
}

