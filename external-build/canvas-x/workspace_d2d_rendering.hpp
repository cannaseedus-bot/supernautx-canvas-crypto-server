/// workspace_d2d_rendering.hpp
/// Complete D2D rendering engine with COM initialization

#pragma once

#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <string>
#include <vector>
#include <map>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// ============================================================================
// D2D RESOURCE MANAGER
// ============================================================================

class D2DResourceManager {
public:
    D2DResourceManager();
    ~D2DResourceManager();

    // Lifecycle
    bool Initialize(HWND hwnd, int width, int height);
    void Shutdown();
    bool IsInitialized() const { return initialized_; }

    // Frame management
    void BeginDraw();
    void EndDraw();
    HRESULT Present();

    // Drawing primitives
    void DrawRect(float x, float y, float w, float h, 
                  unsigned int fillColor, float strokeWidth = 1.0f, unsigned int strokeColor = 0xFF000000);
    void DrawCircle(float x, float y, float radius, 
                    unsigned int fillColor, float strokeWidth = 1.0f, unsigned int strokeColor = 0xFF000000);
    void DrawLine(float x1, float y1, float x2, float y2, 
                  unsigned int color, float strokeWidth = 1.0f);
    void DrawText(const std::string& text, float x, float y, float w, float h, 
                  unsigned int textColor, const std::string& fontName = "Consolas", float fontSize = 12.0f);

    // Clear & fill
    void Clear(unsigned int color);
    void FillRect(float x, float y, float w, float h, unsigned int color);

    // Font management
    IDWriteTextFormat* GetOrCreateFont(const std::string& fontName, float fontSize);

    // Query
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }

private:
    // COM interfaces
    ID2D1Factory* factory_ = nullptr;
    ID2D1HwndRenderTarget* renderTarget_ = nullptr;
    IDWriteFactory* writeFactory_ = nullptr;
    IWICImagingFactory* imagingFactory_ = nullptr;

    // Brush cache
    std::map<unsigned int, ID2D1SolidColorBrush*> brushCache_;
    ID2D1SolidColorBrush* currentBrush_ = nullptr;

    // Font cache
    std::map<std::string, IDWriteTextFormat*> fontCache_;

    // State
    bool initialized_ = false;
    HWND hwnd_ = NULL;
    int width_ = 800;
    int height_ = 600;

    // Helpers
    D2D1_COLOR_F ARGBtoD2D(unsigned int argb);
    ID2D1SolidColorBrush* GetOrCreateBrush(unsigned int color);
    HRESULT CreateDeviceResources();
    void DiscardDeviceResources();
};

// ============================================================================
// D2D RENDERER (High-level drawing API)
// ============================================================================

class D2DRenderer {
public:
    D2DRenderer();
    ~D2DRenderer();

    bool Initialize(HWND hwnd, int width, int height);
    void Shutdown();

    // Frame
    void BeginFrame();
    void EndFrame();

    // Drawing
    void DrawShape(const std::string& type, float x, float y, float w, float h,
                   unsigned int fillColor, unsigned int strokeColor = 0xFF000000, float strokeWidth = 1.0f);
    void DrawText(const std::string& text, float x, float y, float w, float h,
                  unsigned int color, float fontSize = 12.0f);
     void DrawRect(float x, float y, float w, float h, unsigned int color) {
        resource_.get()->DrawRect(x, y, w, h, color);
    }
    void DrawCircle(float x, float y, float radius, unsigned int color) {
        resource_.get()->DrawCircle(x, y, radius, color);
    }
    void Clear(unsigned int color) {
        resource_.get()->Clear(color);
    }
    void FillRect(float x, float y, float w, float h, unsigned int color) {
        resource_.get()->FillRect(x, y, w, h, color);
    }
    void DrawLine(float x1, float y1, float x2, float y2, unsigned int color, float strokeWidth = 1.0f) {
        resource_.get()->DrawLine(x1, y1, x2, y2, color, strokeWidth);
    }

    // Query
    D2DResourceManager* GetResourceManager() { return resource_.get(); }
    int GetWidth() const;
    int GetHeight() const;

private:
    class D2DResourcePtr {
    public:
        D2DResourcePtr() : ptr_(nullptr) {}
        ~D2DResourcePtr() { if (ptr_) delete ptr_; }
        void reset(D2DResourceManager* p) { if (ptr_) delete ptr_; ptr_ = p; }
        D2DResourceManager* get() { return ptr_; }
        D2DResourceManager* get() const { return ptr_; }
    private:
        D2DResourceManager* ptr_;
    };

    D2DResourcePtr resource_;
    bool frameInProgress_ = false;
};

// ============================================================================
// PANEL RENDERER (Layout + rendering combined)
// ============================================================================

struct PanelRenderContext {
    float x, y, w, h;
    std::string name;
    unsigned int bgColor;
    unsigned int borderColor;
};

class PanelRenderer {
public:
    PanelRenderer(D2DRenderer* renderer);

    void DrawPanel(const PanelRenderContext& ctx);
    void DrawPanelBorder(float x, float y, float w, float h, unsigned int color);
    void DrawPanelBackground(float x, float y, float w, float h, unsigned int color);
    void DrawPanelText(const std::string& text, float x, float y, float w, float h, unsigned int color);

private:
    D2DRenderer* renderer_;
};
