/// workspace_editor_d2d.cpp
/// Phase 7.19 Task 1: D2D Code Editor - Core Implementation
/// Part 1: Data Structures + Basic Operations

#include "workspace_editor_d2d.hpp"
#include <algorithm>
#include <cctype>

// ============================================================================
// EDITOR CURSOR
// ============================================================================

const float EditorCursor::BLINK_PERIOD = 1.0f;

void EditorCursor::Update(float deltaTime) {
    blinkTime += deltaTime;
    if (blinkTime > BLINK_PERIOD) {
        blinkTime -= BLINK_PERIOD;
    }
}

bool EditorCursor::IsVisible() const {
    float phase = blinkTime / BLINK_PERIOD;
    return phase < 0.5f;  // Visible first half of blink cycle
}

// ============================================================================
// EDITOR SELECTION
// ============================================================================

void EditorSelection::Start(int x, int y) {
    startX = x;
    startY = y;
    endX = x;
    endY = y;
    isActive = true;
}

void EditorSelection::End(int x, int y) {
    endX = x;
    endY = y;
}

void EditorSelection::Clear() {
    isActive = false;
    startX = startY = endX = endY = 0;
}

bool EditorSelection::Contains(int x, int y) const {
    if (!isActive) return false;
    
    // Normalize start/end (handle reverse selection)
    int minY = std::min(startY, endY);
    int maxY = std::max(startY, endY);
    
    if (y < minY || y > maxY) return false;
    
    if (minY == maxY) {
        // Single line selection
        int minX = std::min(startX, endX);
        int maxX = std::max(startX, endX);
        return x >= minX && x < maxX;
    }
    
    if (y == minY) {
        // First line of multi-line selection
        int minX = (startY < endY) ? startX : endX;
        return x >= minX;
    }
    
    if (y == maxY) {
        // Last line of multi-line selection
        int maxX = (startY < endY) ? endX : startX;
        return x < maxX;
    }
    
    // Middle lines are fully selected
    return true;
}

std::vector<std::pair<int, int>> EditorSelection::GetSelectedRanges() const {
    std::vector<std::pair<int, int>> ranges;
    if (!isActive) return ranges;
    
    int minY = std::min(startY, endY);
    int maxY = std::max(startY, endY);
    
    for (int y = minY; y <= maxY; ++y) {
        int startPos = 0, endPos = INT_MAX;
        
        if (minY == maxY) {
            startPos = std::min(startX, endX);
            endPos = std::max(startX, endX);
        } else if (y == minY) {
            startPos = (startY < endY) ? startX : endX;
        } else if (y == maxY) {
            endPos = (startY < endY) ? endX : startX;
        }
        
        ranges.push_back({startPos, endPos});
    }
    
    return ranges;
}

// ============================================================================
// EDITOR UNDO STACK
// ============================================================================

void EditorUndoStack::Push(const EditorAction& action) {
    undoStack.push_back(action);
    redoStack.clear();  // Clear redo stack on new action
}

EditorAction EditorUndoStack::Undo() {
    if (undoStack.empty()) {
        return EditorAction{EditorAction::INSERT_CHAR, 0, 0, "", 0, 0};
    }
    
    EditorAction action = undoStack.back();
    undoStack.pop_back();
    redoStack.push_back(action);
    
    return action;
}

EditorAction EditorUndoStack::Redo() {
    if (redoStack.empty()) {
        return EditorAction{EditorAction::INSERT_CHAR, 0, 0, "", 0, 0};
    }
    
    EditorAction action = redoStack.back();
    redoStack.pop_back();
    undoStack.push_back(action);
    
    return action;
}

bool EditorUndoStack::CanUndo() const {
    return !undoStack.empty();
}

bool EditorUndoStack::CanRedo() const {
    return !redoStack.empty();
}

// ============================================================================
// EDITOR LINE CACHE
// ============================================================================

void EditorLineCache::Invalidate(int line) {
    if (line >= 0 && line < cache.size()) {
        cache[line].isDirty = true;
    }
}

void EditorLineCache::InvalidateAll() {
    for (auto& entry : cache) {
        entry.isDirty = true;
    }
}

LineCacheEntry& EditorLineCache::GetEntry(int line) {
    while (cache.size() <= line) {
        cache.push_back(LineCacheEntry());
    }
    return cache[line];
}

// ============================================================================
// D2D EDITOR - MAIN CLASS
// ============================================================================

D2DEditor::D2DEditor() {
    lines.push_back("");  // Start with empty line
}

D2DEditor::~D2DEditor() {
    // Cleanup handled by ComPtr
}

void D2DEditor::Initialize(const EditorRenderContext& ctx, int width, int height) {
    viewportWidth = width;
    viewportHeight = height;
    visibleLines = (int)(height / ctx.lineHeight);
}

// ============================================================================
// CHARACTER INPUT
// ============================================================================

void D2DEditor::InsertChar(char c) {
    ClampCursor();
    
    if (cursor.y >= 0 && cursor.y < lines.size()) {
        // Record undo action
        EditorAction action;
        action.type = EditorAction::INSERT_CHAR;
        action.x = cursor.x;
        action.y = cursor.y;
        action.data = std::string(1, c);
        action.cursorX = cursor.x + 1;
        action.cursorY = cursor.y;
        
        lines[cursor.y].insert(cursor.x, 1, c);
        cursor.x++;
        
        undoStack.Push(action);
        lineCache.Invalidate(cursor.y);
        isModified = true;
    }
}

void D2DEditor::InsertString(const std::string& str) {
    for (char c : str) {
        if (c == '\n') {
            NewLine();
        } else if (c == '\t') {
            InsertChar(' ');
            InsertChar(' ');
            InsertChar(' ');
            InsertChar(' ');
        } else {
            InsertChar(c);
        }
    }
}

void D2DEditor::DeleteChar() {
    ClampCursor();
    
    if (cursor.y >= 0 && cursor.y < lines.size() && cursor.x < lines[cursor.y].size()) {
        EditorAction action;
        action.type = EditorAction::DELETE_CHAR;
        action.x = cursor.x;
        action.y = cursor.y;
        action.data = std::string(1, lines[cursor.y][cursor.x]);
        action.cursorX = cursor.x;
        action.cursorY = cursor.y;
        
        lines[cursor.y].erase(cursor.x, 1);
        
        undoStack.Push(action);
        lineCache.Invalidate(cursor.y);
        isModified = true;
    }
}

void D2DEditor::Backspace() {
    if (cursor.x > 0) {
        cursor.x--;
        DeleteChar();
    } else if (cursor.y > 0) {
        // Merge with previous line
        cursor.y--;
        cursor.x = lines[cursor.y].size();
        lines[cursor.y] += lines[cursor.y + 1];
        lines.erase(lines.begin() + cursor.y + 1);
        
        EditorAction action;
        action.type = EditorAction::DELETE_LINE;
        action.y = cursor.y + 1;
        action.cursorX = cursor.x;
        action.cursorY = cursor.y;
        
        undoStack.Push(action);
        lineCache.Invalidate(cursor.y);
        isModified = true;
    }
}

void D2DEditor::Delete() {
    DeleteChar();
}

// ============================================================================
// LINE OPERATIONS
// ============================================================================

void D2DEditor::NewLine() {
    ClampCursor();
    
    if (cursor.y >= 0 && cursor.y < lines.size()) {
        EditorAction action;
        action.type = EditorAction::INSERT_LINE;
        action.y = cursor.y + 1;
        action.cursorX = 0;
        action.cursorY = cursor.y + 1;
        
        std::string remainder = lines[cursor.y].substr(cursor.x);
        lines[cursor.y] = lines[cursor.y].substr(0, cursor.x);
        
        lines.insert(lines.begin() + cursor.y + 1, remainder);
        
        cursor.y++;
        cursor.x = 0;
        
        // Preserve indentation
        if (cursor.y - 1 >= 0) {
            std::string indent = GetIndentString();
            lines[cursor.y] = indent + remainder;
            cursor.x = indent.size();
        }
        
        undoStack.Push(action);
        lineCache.Invalidate(cursor.y - 1);
        lineCache.Invalidate(cursor.y);
        isModified = true;
    }
}

void D2DEditor::DeleteLine() {
    if (cursor.y >= 0 && cursor.y < lines.size()) {
        EditorAction action;
        action.type = EditorAction::DELETE_LINE;
        action.y = cursor.y;
        action.data = lines[cursor.y];
        action.cursorX = 0;
        action.cursorY = cursor.y;
        
        lines.erase(lines.begin() + cursor.y);
        
        if (cursor.y >= lines.size()) {
            cursor.y = lines.size() - 1;
        }
        cursor.x = 0;
        
        undoStack.Push(action);
        isModified = true;
    }
}

void D2DEditor::IndentLine() {
    ClampCursor();
    if (cursor.y < lines.size()) {
        std::string indent(tabWidth, ' ');
        lines[cursor.y].insert(0, indent);
        cursor.x += tabWidth;
        lineCache.Invalidate(cursor.y);
        isModified = true;
    }
}

void D2DEditor::UnindentLine() {
    ClampCursor();
    if (cursor.y < lines.size() && lines[cursor.y].size() > 0) {
        int removeCount = 0;
        for (int i = 0; i < tabWidth && i < lines[cursor.y].size(); ++i) {
            if (lines[cursor.y][i] == ' ') {
                removeCount++;
            } else {
                break;
            }
        }
        
        if (removeCount > 0) {
            lines[cursor.y].erase(0, removeCount);
            cursor.x = std::max(0, cursor.x - removeCount);
            lineCache.Invalidate(cursor.y);
            isModified = true;
        }
    }
}

// ============================================================================
// NAVIGATION
// ============================================================================

void D2DEditor::MoveCursorLeft() {
    if (cursor.x > 0) {
        cursor.x--;
    } else if (cursor.y > 0) {
        cursor.y--;
        if (cursor.y < lines.size()) {
            cursor.x = lines[cursor.y].size();
        }
    }
}

void D2DEditor::MoveCursorRight() {
    ClampCursor();
    if (cursor.y < lines.size()) {
        if (cursor.x < lines[cursor.y].size()) {
            cursor.x++;
        } else if (cursor.y < lines.size() - 1) {
            cursor.y++;
            cursor.x = 0;
        }
    }
}

void D2DEditor::MoveCursorUp() {
    if (cursor.y > 0) {
        cursor.y--;
        ClampCursor();
    }
}

void D2DEditor::MoveCursorDown() {
    if (cursor.y < lines.size() - 1) {
        cursor.y++;
        ClampCursor();
    }
}

void D2DEditor::MoveCursorHome() {
    // Move to first non-whitespace, then to column 0
    ClampCursor();
    if (cursor.y < lines.size()) {
        int firstNonSpace = 0;
        while (firstNonSpace < lines[cursor.y].size() && 
               std::isspace(lines[cursor.y][firstNonSpace])) {
            firstNonSpace++;
        }
        
        cursor.x = (cursor.x == firstNonSpace) ? 0 : firstNonSpace;
    }
}

void D2DEditor::MoveCursorEnd() {
    ClampCursor();
    if (cursor.y < lines.size()) {
        cursor.x = lines[cursor.y].size();
    }
}

void D2DEditor::MoveCursorPageUp() {
    cursor.y = std::max(0, cursor.y - visibleLines);
    ClampCursor();
    ScrollUp(visibleLines);
}

void D2DEditor::MoveCursorPageDown() {
    cursor.y = std::min((int)lines.size() - 1, cursor.y + visibleLines);
    ClampCursor();
    ScrollDown(visibleLines);
}

void D2DEditor::MoveCursorToLine(int line) {
    cursor.y = std::max(0, std::min((int)lines.size() - 1, line));
    cursor.x = 0;
}

// ============================================================================
// SELECTION
// ============================================================================

void D2DEditor::SelectAll() {
    selection.startX = 0;
    selection.startY = 0;
    selection.endX = lines.back().size();
    selection.endY = lines.size() - 1;
    selection.isActive = true;
}

void D2DEditor::SelectLine() {
    ClampCursor();
    selection.startX = 0;
    selection.startY = cursor.y;
    selection.endX = lines[cursor.y].size();
    selection.endY = cursor.y;
    selection.isActive = true;
}

void D2DEditor::SelectWord() {
    ClampCursor();
    if (cursor.y >= 0 && cursor.y < lines.size()) {
        int startX = cursor.x;
        int endX = cursor.x;
        
        // Find word boundaries
        while (startX > 0 && !std::isspace(lines[cursor.y][startX - 1])) {
            startX--;
        }
        while (endX < lines[cursor.y].size() && !std::isspace(lines[cursor.y][endX])) {
            endX++;
        }
        
        selection.startX = startX;
        selection.startY = cursor.y;
        selection.endX = endX;
        selection.endY = cursor.y;
        selection.isActive = true;
    }
}

void D2DEditor::ClearSelection() {
    selection.Clear();
}

// ============================================================================
// SCROLLING
// ============================================================================

void D2DEditor::ScrollUp(int lines) {
    scrollY = std::max(0, scrollY - lines);
}

void D2DEditor::ScrollDown(int lines) {
    scrollY += lines;
}

void D2DEditor::ScrollLeft(int chars) {
    scrollX = std::max(0, scrollX - chars);
}

void D2DEditor::ScrollRight(int chars) {
    scrollX += chars;
}

void D2DEditor::EnsureCursorVisible() {
    // Adjust scroll so cursor is visible
    if (cursor.y < scrollY) {
        scrollY = cursor.y;
    } else if (cursor.y >= scrollY + visibleLines) {
        scrollY = cursor.y - visibleLines + 1;
    }
}

// ============================================================================
// CONTENT ACCESS & MODIFICATION
// ============================================================================

std::string D2DEditor::GetSelectedText() const {
    if (!selection.isActive) return "";
    
    std::string result;
    int minY = std::min(selection.startY, selection.endY);
    int maxY = std::max(selection.startY, selection.endY);
    
    for (int y = minY; y <= maxY; ++y) {
        if (y >= lines.size()) break;
        
        int startX = 0, endX = lines[y].size();
        
        if (minY == maxY) {
            startX = std::min(selection.startX, selection.endX);
            endX = std::max(selection.startX, selection.endX);
        } else if (y == minY) {
            startX = (selection.startY < selection.endY) ? selection.startX : selection.endX;
        } else if (y == maxY) {
            endX = (selection.startY < selection.endY) ? selection.endX : selection.startX;
        }
        
        result += lines[y].substr(startX, endX - startX);
        if (y < maxY) result += '\n';
    }
    
    return result;
}

std::string D2DEditor::GetLineText(int line) const {
    if (line >= 0 && line < lines.size()) {
        return lines[line];
    }
    return "";
}

void D2DEditor::SetContent(const std::string& content) {
    lines.clear();
    
    size_t pos = 0;
    while (pos < content.size()) {
        size_t newlinePos = content.find('\n', pos);
        if (newlinePos == std::string::npos) {
            lines.push_back(content.substr(pos));
            break;
        }
        lines.push_back(content.substr(pos, newlinePos - pos));
        pos = newlinePos + 1;
    }
    
    if (lines.empty()) {
        lines.push_back("");
    }
    
    cursor.x = 0;
    cursor.y = 0;
    scrollX = 0;
    scrollY = 0;
    selection.Clear();
    isModified = false;
}

void D2DEditor::Clear() {
    lines.clear();
    lines.push_back("");
    cursor.x = 0;
    cursor.y = 0;
    scrollX = 0;
    scrollY = 0;
    selection.Clear();
    undoStack = EditorUndoStack();
    lineCache.InvalidateAll();
    isModified = false;
}

void D2DEditor::ReplaceSelection(const std::string& text) {
    if (!selection.isActive) return;
    
    // Delete selected content
    int minY = std::min(selection.startY, selection.endY);
    int maxY = std::max(selection.startY, selection.endY);
    
    if (minY == maxY) {
        int startX = std::min(selection.startX, selection.endX);
        int endX = std::max(selection.startX, selection.endX);
        lines[minY].erase(startX, endX - startX);
        cursor.x = startX;
        cursor.y = minY;
    }
    
    InsertString(text);
    selection.Clear();
    isModified = true;
}

// ============================================================================
// UNDO/REDO
// ============================================================================

void D2DEditor::Undo() {
    if (!undoStack.CanUndo()) return;
    
    EditorAction action = undoStack.Undo();
    
    // Reverse the action
    switch (action.type) {
        case EditorAction::INSERT_CHAR:
            lines[action.y].erase(action.x, action.data.size());
            cursor.x = action.x;
            cursor.y = action.y;
            break;
        case EditorAction::DELETE_CHAR:
            lines[action.y].insert(action.x, action.data);
            cursor.x = action.x + action.data.size();
            cursor.y = action.y;
            break;
        // ... more cases
    }
    
    isModified = true;
}

void D2DEditor::Redo() {
    if (!undoStack.CanRedo()) return;
    
    EditorAction action = undoStack.Redo();
    
    // Re-apply the action
    switch (action.type) {
        case EditorAction::INSERT_CHAR:
            lines[action.y].insert(action.x, action.data);
            cursor.x = action.cursorX;
            cursor.y = action.cursorY;
            break;
        // ... more cases
    }
    
    isModified = true;
}

bool D2DEditor::CanUndo() const {
    return undoStack.CanUndo();
}

bool D2DEditor::CanRedo() const {
    return undoStack.CanRedo();
}

// ============================================================================
// HELPERS
// ============================================================================

void D2DEditor::ClampCursor() {
    if (lines.empty()) {
        lines.push_back("");
    }
    
    cursor.y = std::max(0, std::min((int)lines.size() - 1, cursor.y));
    
    if (cursor.y < lines.size()) {
        cursor.x = std::max(0, std::min((int)lines[cursor.y].size(), cursor.x));
    }
}

void D2DEditor::EnsureLinesExist() {
    while (lines.size() <= cursor.y) {
        lines.push_back("");
    }
}

int D2DEditor::PixelToCharX(float px) const {
    return (int)((px - 50.0f) / 10.0f);  // Approximate
}

int D2DEditor::PixelToCharY(float py) const {
    return (int)(py / 18.0f) + scrollY;  // Approximate
}

float D2DEditor::CharToPixelX(int cx) const {
    return 50.0f + cx * 10.0f;
}

float D2DEditor::CharToPixelY(int cy) const {
    return (cy - scrollY) * 18.0f;
}

std::string D2DEditor::GetIndentString() const {
    if (cursor.y == 0 || cursor.y - 1 >= lines.size()) {
        return "";
    }
    
    const std::string& prevLine = lines[cursor.y - 1];
    size_t indent = 0;
    
    while (indent < prevLine.size() && std::isspace(prevLine[indent])) {
        indent++;
    }
    
    return prevLine.substr(0, indent);
}

void D2DEditor::RecordAction(const EditorAction& action) {
    undoStack.Push(action);
}

// ============================================================================
// INPUT WIRING
// ============================================================================

void D2DEditor::OnMouseClick(int x, int y) {
    cursor.x = PixelToCharX(x);
    cursor.y = PixelToCharY(y);
    ClampCursor();
}

void D2DEditor::OnMouseDrag(int x, int y) {
    int newX = PixelToCharX(x);
    int newY = PixelToCharY(y);
    
    if (!selection.isActive) {
        selection.Start(cursor.x, cursor.y);
    }
    
    selection.End(newX, newY);
    cursor.x = newX;
    cursor.y = newY;
}

void D2DEditor::OnMouseWheel(int delta) {
    ScrollUp(delta > 0 ? 3 : -3);
}

// ============================================================================
// UPDATE & RENDER (Stubs - wired to workspace render system)
// ============================================================================

void D2DEditor::Update(float deltaTime) {
    cursor.Update(deltaTime);
    EnsureCursorVisible();
}

void D2DEditor::Render(const EditorRenderContext& ctx) {
    RenderLines(ctx);
    RenderLineNumbers(ctx);
    RenderCursor(ctx);
    RenderSelection(ctx);
    RenderScrollBar(ctx);
}

void D2DEditor::RenderLines(const EditorRenderContext& ctx) {
    // Implemented in rendering module
}

void D2DEditor::RenderLineNumbers(const EditorRenderContext& ctx) {
    // Implemented in rendering module
}

void D2DEditor::RenderCursor(const EditorRenderContext& ctx) {
    // Implemented in rendering module
}

void D2DEditor::RenderSelection(const EditorRenderContext& ctx) {
    // Implemented in rendering module
}

void D2DEditor::RenderScrollBar(const EditorRenderContext& ctx) {
    // Implemented in rendering module
}

float D2DEditor::GetScrollPercentY() const {
    if (lines.size() <= visibleLines) return 0.0f;
    return (float)scrollY / (lines.size() - visibleLines);
}

float D2DEditor::GetScrollPercentX() const {
    return (float)scrollX / 100.0f;  // Approximate
}

void D2DEditor::SetHighlighter(std::shared_ptr<SyntaxHighlighter> hlighter) {
    highlighter = hlighter;
    lineCache.InvalidateAll();
}
