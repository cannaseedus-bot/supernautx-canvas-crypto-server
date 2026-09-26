/// workspace_editor_d2d.hpp
/// Phase 7.19 Task 1: D2D Code Editor Implementation
/// Complete header for D2D-based text editor with cursor management

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <d2d1.h>
#include <dwrite.h>

// ============================================================================
// EDITOR RENDERING CONTEXT
// ============================================================================

struct EditorRenderContext {
    ID2D1RenderTarget* renderTarget = nullptr;
    IDWriteFactory* writeFactory = nullptr;
    IDWriteTextFormat* textFormat = nullptr;
    ID2D1Brush* textBrush = nullptr;
    ID2D1Brush* cursorBrush = nullptr;
    ID2D1Brush* lineNumBrush = nullptr;
    ID2D1Brush* selectionBrush = nullptr;
    
    float lineHeight = 18.0f;
    float charWidth = 10.0f;
    float marginLeft = 50.0f;  // For line numbers
    float marginTop = 10.0f;
};

// ============================================================================
// EDITOR CURSOR
// ============================================================================

struct EditorCursor {
    int x = 0;  // Character position in line
    int y = 0;  // Line number
    bool isBlinking = true;
    float blinkTime = 0.0f;
    static const float BLINK_PERIOD;
    
    void Update(float deltaTime);
    bool IsVisible() const;
};

// ============================================================================
// EDITOR SELECTION
// ============================================================================

struct EditorSelection {
    int startX = 0, startY = 0;
    int endX = 0, endY = 0;
    bool isActive = false;
    
    void Start(int x, int y);
    void End(int x, int y);
    void Clear();
    bool Contains(int x, int y) const;
    std::vector<std::pair<int, int>> GetSelectedRanges() const;
};

// ============================================================================
// EDITOR UNDO/REDO STACK
// ============================================================================

struct EditorAction {
    enum Type {
        INSERT_CHAR,
        DELETE_CHAR,
        INSERT_LINE,
        DELETE_LINE,
        REPLACE_TEXT
    };
    
    Type type;
    int x, y;
    std::string data;
    int cursorX, cursorY;  // Cursor position after action
};

class EditorUndoStack {
public:
    void Push(const EditorAction& action);
    EditorAction Undo();
    EditorAction Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    
private:
    std::vector<EditorAction> undoStack;
    std::vector<EditorAction> redoStack;
};

// ============================================================================
// EDITOR SYNTAX HIGHLIGHTING (Phase 7.20, but hooks here)
// ============================================================================

enum TokenType {
    TOKEN_UNKNOWN,
    TOKEN_KEYWORD,
    TOKEN_STRING,
    TOKEN_COMMENT,
    TOKEN_NUMBER,
    TOKEN_OPERATOR,
    TOKEN_IDENTIFIER
};

struct TokenStyle {
    int type;  // TokenType
    D2D1_COLOR_F color;
    bool isBold;
    
    TokenStyle() : type(TOKEN_UNKNOWN), isBold(false) {
        color = D2D1::ColorF(D2D1::ColorF::White);
    }
};

class SyntaxHighlighter {
public:
    virtual ~SyntaxHighlighter() {}
    virtual int GetTokenType(const std::string& token) = 0;
    virtual TokenStyle GetStyle(int type) = 0;
};

// ============================================================================
// EDITOR LINE CACHE (Performance optimization)
// ============================================================================

struct LineCacheEntry {
    std::string content;
    std::vector<TokenType> tokens;
    bool isDirty = true;
};

class EditorLineCache {
public:
    void Invalidate(int line);
    void InvalidateAll();
    LineCacheEntry& GetEntry(int line);
    
private:
    std::vector<LineCacheEntry> cache;
};

// ============================================================================
// MAIN EDITOR CLASS
// ============================================================================

class D2DEditor {
public:
    D2DEditor();
    ~D2DEditor();
    
    // Initialization
    void Initialize(const EditorRenderContext& ctx, int width, int height);
    
    // Rendering
    void Render(const EditorRenderContext& ctx);
    void RenderLines(const EditorRenderContext& ctx);
    void RenderLineNumbers(const EditorRenderContext& ctx);
    void RenderCursor(const EditorRenderContext& ctx);
    void RenderSelection(const EditorRenderContext& ctx);
    void RenderScrollBar(const EditorRenderContext& ctx);
    
    // Input: Character
    void InsertChar(char c);
    void InsertString(const std::string& str);
    void DeleteChar();
    void Backspace();
    void Delete();
    
    // Input: Line Operations
    void NewLine();
    void DeleteLine();
    void IndentLine();
    void UnindentLine();
    
    // Input: Navigation
    void MoveCursorLeft();
    void MoveCursorRight();
    void MoveCursorUp();
    void MoveCursorDown();
    void MoveCursorHome();
    void MoveCursorEnd();
    void MoveCursorPageUp();
    void MoveCursorPageDown();
    void MoveCursorToLine(int line);
    
    // Input: Selection
    void SelectAll();
    void SelectLine();
    void SelectWord();
    void ClearSelection();
    
    // Input: Scrolling
    void ScrollUp(int lines = 1);
    void ScrollDown(int lines = 1);
    void ScrollLeft(int chars = 1);
    void ScrollRight(int chars = 1);
    void EnsureCursorVisible();
    
    // Content access
    const std::vector<std::string>& GetLines() const { return lines; }
    std::string GetSelectedText() const;
    std::string GetLineText(int line) const;
    int GetLineCount() const { return lines.size(); }
    
    // Content modification
    void SetContent(const std::string& content);
    void Clear();
    void ReplaceSelection(const std::string& text);
    
    // State queries
    const EditorCursor& GetCursor() const { return cursor; }
    bool IsModified() const { return isModified; }
    void MarkClean() { isModified = false; }
    float GetScrollPercentY() const;
    float GetScrollPercentX() const;
    
    // Undo/Redo
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    
    // Syntax highlighting integration
    void SetHighlighter(std::shared_ptr<SyntaxHighlighter> hlighter);
    
    // Update loop
    void Update(float deltaTime);
    
    // Mouse input (wired to workspace runtime)
    void OnMouseClick(int x, int y);
    void OnMouseDrag(int x, int y);
    void OnMouseWheel(int delta);
    
private:
    // State
    std::vector<std::string> lines;
    EditorCursor cursor;
    EditorSelection selection;
    EditorUndoStack undoStack;
    EditorLineCache lineCache;
    
    // Display
    int viewportWidth = 0;
    int viewportHeight = 0;
    int scrollX = 0;
    int scrollY = 0;
    int visibleLines = 0;
    
    // Configuration
    int tabWidth = 4;
    bool useSpacesForTab = true;
    bool isModified = false;
    
    // Syntax highlighting
    std::shared_ptr<SyntaxHighlighter> highlighter = nullptr;
    
    // Helpers
    void ClampCursor();
    void EnsureLinesExist();
    int PixelToCharX(float px) const;
    int PixelToCharY(float py) const;
    float CharToPixelX(int cx) const;
    float CharToPixelY(int cy) const;
    std::string GetIndentString() const;
    void RecordAction(const EditorAction& action);
};

#endif // WORKSPACE_EDITOR_D2D_HPP
