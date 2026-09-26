/// workspace_canvas_d2d.hpp
/// D2D rendering for canvas preview

#pragma once

#include <string>
#include <vector>
#include <windows.h>

// Forward declaration
interface ID2D1RenderTarget;
interface ID2D1Factory;
interface IDWriteFactory;

// ============================================================================
// D2D SHAPE DEFINITION
// ============================================================================

struct D2DShape {
    std::string type;  // rect, circle, line, text
    float x = 0, y = 0, width = 100, height = 100;
    unsigned int fillColor = 0xFF000000;
    unsigned int strokeColor = 0xFF000000;
    float strokeWidth = 1.0f;
    std::string text = "";
};

// ============================================================================
// CANVAS RENDERER
// ============================================================================

class CanvasRenderer {
public:
    CanvasRenderer();
    ~CanvasRenderer();

    // Initialize D2D
    bool Initialize(HWND hwnd);
    void Shutdown();

    // Drawing
    void BeginDraw();
    void EndDraw();
    void Clear(unsigned int color = 0xFFFFFFFF);

    // Shape drawing
    void DrawRect(const D2DShape& shape);
    void DrawCircle(const D2DShape& shape);
    void DrawLine(const D2DShape& shape);
    void DrawText(const D2DShape& shape);

    // Rendering
    void RenderShapes(const std::vector<D2DShape>& shapes);

    // Query
    bool IsInitialized() const;
    int GetWidth() const;
    int GetHeight() const;

private:
    ID2D1Factory* factory_ = nullptr;
    ID2D1RenderTarget* renderTarget_ = nullptr;
    IDWriteFactory* writeFactory_ = nullptr;
    HWND hwnd_ = NULL;
    bool initialized_ = false;
    int width_ = 800;
    int height_ = 600;

    unsigned int ConvertARGBtoD2D(unsigned int argb);
};
