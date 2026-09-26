/// workspace_editor_d2d_minimal.hpp
/// Minimal header to verify compilation

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <d2d1.h>
#include <dwrite.h>

struct Rect {
    float x, y, w, h;
};

struct EditorCursor {
    int x = 0;
    int y = 0;
    bool IsVisible() const { return true; }
};

class D2DEditor {
public:
    D2DEditor() {}
    ~D2DEditor() {}
    
    void Initialize(int w, int h) {}
    void InsertChar(char c);
    void Backspace();
    void Delete();
    void NewLine();
    void MoveCursorLeft();
    void MoveCursorRight();
    void MoveCursorUp();
    void MoveCursorDown();
    void Undo() {}
    void Redo() {}
    void SelectAll() {}
    std::string GetSelectedText() const { return ""; }
    const std::vector<std::string>& GetLines() const { return lines; }
    const EditorCursor& GetCursor() const { return cursor; }
    float GetScrollPercentY() const { return 0.0f; }
    void OnMouseClick(int x, int y) {}
    void OnMouseWheel(int delta) {}
    
    std::vector<std::string> lines;
    EditorCursor cursor;
};
