/// workspace_canvas_d2d.cpp
/// D2D canvas rendering implementation

#include "workspace_canvas_d2d.hpp"

// ============================================================================
// CANVAS RENDERER
// ============================================================================

CanvasRenderer::CanvasRenderer() {}

CanvasRenderer::~CanvasRenderer() {
    Shutdown();
}

bool CanvasRenderer::Initialize(HWND hwnd) {
    if (initialized_) return true;

    hwnd_ = hwnd;

    // Get window dimensions
    RECT rect;
    GetClientRect(hwnd, &rect);
    width_ = rect.right - rect.left;
    height_ = rect.bottom - rect.top;

    // For MVP: stubbed D2D initialization
    // Full implementation in Phase 7.20 with actual COM initialization
    initialized_ = true;

    return true;
}

void CanvasRenderer::Shutdown() {
    if (renderTarget_) {
        renderTarget_ = nullptr;
    }
    if (factory_) {
        factory_ = nullptr;
    }
    if (writeFactory_) {
        writeFactory_ = nullptr;
    }
    initialized_ = false;
}

void CanvasRenderer::BeginDraw() {
    if (!initialized_) return;
    // D2D BeginDraw in Phase 7.20
}

void CanvasRenderer::EndDraw() {
    if (!initialized_) return;
    // D2D EndDraw and Present in Phase 7.20
}

void CanvasRenderer::Clear(unsigned int color) {
    if (!initialized_) return;
    // Fill background in Phase 7.20
}

void CanvasRenderer::DrawRect(const D2DShape& shape) {
    if (!initialized_) return;
    // D2D DrawRectangle in Phase 7.20
}

void CanvasRenderer::DrawCircle(const D2DShape& shape) {
    if (!initialized_) return;
    // D2D DrawEllipse in Phase 7.20
}

void CanvasRenderer::DrawLine(const D2DShape& shape) {
    if (!initialized_) return;
    // D2D DrawLine in Phase 7.20
}

void CanvasRenderer::DrawText(const D2DShape& shape) {
    if (!initialized_) return;
    // D2D DrawText via DirectWrite in Phase 7.20
}

void CanvasRenderer::RenderShapes(const std::vector<D2DShape>& shapes) {
    if (!initialized_) return;

    BeginDraw();
    Clear();

    for (const auto& shape : shapes) {
        if (shape.type == "rect") {
            DrawRect(shape);
        } else if (shape.type == "circle") {
            DrawCircle(shape);
        } else if (shape.type == "line") {
            DrawLine(shape);
        } else if (shape.type == "text") {
            DrawText(shape);
        }
    }

    EndDraw();
}

bool CanvasRenderer::IsInitialized() const {
    return initialized_;
}

int CanvasRenderer::GetWidth() const {
    return width_;
}

int CanvasRenderer::GetHeight() const {
    return height_;
}

unsigned int CanvasRenderer::ConvertARGBtoD2D(unsigned int argb) {
    // Convert ARGB to D2D1_COLOR_F format in Phase 7.20
    return argb;
}
