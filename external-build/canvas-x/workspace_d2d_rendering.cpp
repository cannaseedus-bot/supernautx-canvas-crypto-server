/// workspace_d2d_rendering.cpp
/// D2D rendering engine implementation

#include "workspace_d2d_rendering.hpp"
#include <cmath>

// ============================================================================
// D2D RESOURCE MANAGER
// ============================================================================

D2DResourceManager::D2DResourceManager() {}

D2DResourceManager::~D2DResourceManager() {
    Shutdown();
}

bool D2DResourceManager::Initialize(HWND hwnd, int width, int height) {
    if (initialized_) return true;

    hwnd_ = hwnd;
    width_ = width;
    height_ = height;

    // Create D2D factory
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory_);
    if (FAILED(hr)) return false;

    // Create render target
    hr = CreateDeviceResources();
    if (FAILED(hr)) return false;

    // Create DirectWrite factory
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&writeFactory_);
    if (FAILED(hr)) return false;

    // Create WIC imaging factory
    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&imagingFactory_));
    if (FAILED(hr)) {
        // WIC is optional, continue without it
    }

    initialized_ = true;
    return true;
}

void D2DResourceManager::Shutdown() {
    DiscardDeviceResources();

    // Release brush cache
    for (auto& pair : brushCache_) {
        if (pair.second) {
            pair.second->Release();
        }
    }
    brushCache_.clear();

    // Release font cache
    for (auto& pair : fontCache_) {
        if (pair.second) {
            pair.second->Release();
        }
    }
    fontCache_.clear();

    // Release factories
    if (writeFactory_) {
        writeFactory_->Release();
        writeFactory_ = nullptr;
    }
    if (imagingFactory_) {
        imagingFactory_->Release();
        imagingFactory_ = nullptr;
    }
    if (factory_) {
        factory_->Release();
        factory_ = nullptr;
    }

    initialized_ = false;
}

HRESULT D2DResourceManager::CreateDeviceResources() {
    if (renderTarget_) return S_OK;

    RECT rc;
    GetClientRect(hwnd_, &rc);

    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    HRESULT hr = factory_->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd_, size),
        &renderTarget_
    );

    if (SUCCEEDED(hr)) {
        // Create default solid brush
        hr = renderTarget_->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 1.0f), &currentBrush_);
    }

    return hr;
}

void D2DResourceManager::DiscardDeviceResources() {
    if (currentBrush_) {
        currentBrush_->Release();
        currentBrush_ = nullptr;
    }
    if (renderTarget_) {
        renderTarget_->Release();
        renderTarget_ = nullptr;
    }
}

void D2DResourceManager::BeginDraw() {
    if (!renderTarget_) return;
    renderTarget_->BeginDraw();
}

void D2DResourceManager::EndDraw() {
    if (!renderTarget_) return;
    HRESULT hr = renderTarget_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        DiscardDeviceResources();
    }
}

HRESULT D2DResourceManager::Present() {
    if (!renderTarget_) return E_FAIL;
    return renderTarget_->Flush();
}

D2D1_COLOR_F D2DResourceManager::ARGBtoD2D(unsigned int argb) {
    float a = ((argb >> 24) & 0xFF) / 255.0f;
    float r = ((argb >> 16) & 0xFF) / 255.0f;
    float g = ((argb >> 8) & 0xFF) / 255.0f;
    float b = (argb & 0xFF) / 255.0f;
    return D2D1::ColorF(r, g, b, a);
}

ID2D1SolidColorBrush* D2DResourceManager::GetOrCreateBrush(unsigned int color) {
    auto it = brushCache_.find(color);
    if (it != brushCache_.end()) {
        return it->second;
    }

    if (!renderTarget_) return nullptr;

    ID2D1SolidColorBrush* brush = nullptr;
    HRESULT hr = renderTarget_->CreateSolidColorBrush(ARGBtoD2D(color), &brush);
    if (SUCCEEDED(hr)) {
        brushCache_[color] = brush;
        return brush;
    }

    return nullptr;
}

void D2DResourceManager::DrawRect(float x, float y, float w, float h, unsigned int fillColor, float strokeWidth, unsigned int strokeColor) {
    if (!renderTarget_) return;

    D2D1_RECT_F rect = D2D1::RectF(x, y, x + w, y + h);
    ID2D1SolidColorBrush* fillBrush = GetOrCreateBrush(fillColor);
    if (fillBrush) {
        renderTarget_->FillRectangle(rect, fillBrush);
    }

    if (strokeWidth > 0.0f) {
        ID2D1SolidColorBrush* strokeBrush = GetOrCreateBrush(strokeColor);
        if (strokeBrush) {
            renderTarget_->DrawRectangle(rect, strokeBrush, strokeWidth);
        }
    }
}

void D2DResourceManager::DrawCircle(float x, float y, float radius, unsigned int fillColor, float strokeWidth, unsigned int strokeColor) {
    if (!renderTarget_) return;

    D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius);
    ID2D1SolidColorBrush* fillBrush = GetOrCreateBrush(fillColor);
    if (fillBrush) {
        renderTarget_->FillEllipse(ellipse, fillBrush);
    }

    if (strokeWidth > 0.0f) {
        ID2D1SolidColorBrush* strokeBrush = GetOrCreateBrush(strokeColor);
        if (strokeBrush) {
            renderTarget_->DrawEllipse(ellipse, strokeBrush, strokeWidth);
        }
    }
}

void D2DResourceManager::DrawLine(float x1, float y1, float x2, float y2, unsigned int color, float strokeWidth) {
    if (!renderTarget_) return;

    ID2D1SolidColorBrush* brush = GetOrCreateBrush(color);
    if (brush) {
        renderTarget_->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), brush, strokeWidth);
    }
}

void D2DResourceManager::DrawText(const std::string& text, float x, float y, float w, float h, unsigned int textColor, const std::string& fontName, float fontSize) {
    if (!renderTarget_ || !writeFactory_) return;

    // Convert text to wide string
    int textLen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
    if (textLen <= 1) return;
    
    std::wstring wideText(textLen - 1, L' ');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wideText[0], textLen);

    // Get or create text format
    IDWriteTextFormat* textFormat = GetOrCreateFont(fontName, fontSize);
    if (!textFormat) return;

    // Create layout to measure and draw text
    IDWriteTextLayout* textLayout = nullptr;
    HRESULT hr = writeFactory_->CreateTextLayout(
        wideText.c_str(),
        wideText.length(),
        textFormat,
        w - 10,
        h - 10,
        &textLayout
    );

    if (SUCCEEDED(hr)) {
        D2D1_RECT_F rect = D2D1::RectF(x + 5, y + 5, x + w, y + h);
        ID2D1SolidColorBrush* brush = GetOrCreateBrush(textColor);
        if (brush) {
            renderTarget_->DrawTextLayout(D2D1::Point2F(x + 5, y + 5), textLayout, brush);
        }
        textLayout->Release();
    }
}

void D2DResourceManager::Clear(unsigned int color) {
    if (!renderTarget_) return;
    renderTarget_->Clear(ARGBtoD2D(color));
}

void D2DResourceManager::FillRect(float x, float y, float w, float h, unsigned int color) {
    if (!renderTarget_) return;

    D2D1_RECT_F rect = D2D1::RectF(x, y, x + w, y + h);
    ID2D1SolidColorBrush* brush = GetOrCreateBrush(color);
    if (brush) {
        renderTarget_->FillRectangle(rect, brush);
    }
}

IDWriteTextFormat* D2DResourceManager::GetOrCreateFont(const std::string& fontName, float fontSize) {
    if (!writeFactory_) return nullptr;

    std::string key = fontName + std::to_string((int)fontSize);
    auto it = fontCache_.find(key);
    if (it != fontCache_.end()) {
        return it->second;
    }

    // Convert font name to wide string
    int nameLen = MultiByteToWideChar(CP_UTF8, 0, fontName.c_str(), -1, NULL, 0);
    std::wstring wideName(nameLen - 1, L' ');
    MultiByteToWideChar(CP_UTF8, 0, fontName.c_str(), -1, &wideName[0], nameLen);

    IDWriteTextFormat* textFormat = nullptr;
    HRESULT hr = writeFactory_->CreateTextFormat(
        wideName.c_str(),
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"en-us",
        &textFormat
    );

    if (SUCCEEDED(hr)) {
        fontCache_[key] = textFormat;
        return textFormat;
    }

    return nullptr;
}

// ============================================================================
// D2D RENDERER
// ============================================================================

D2DRenderer::D2DRenderer() {}

D2DRenderer::~D2DRenderer() {
    Shutdown();
}

bool D2DRenderer::Initialize(HWND hwnd, int width, int height) {
    resource_.reset(new D2DResourceManager());
    return resource_.get()->Initialize(hwnd, width, height);
}

void D2DRenderer::Shutdown() {
    if (resource_.get()) {
        resource_.get()->Shutdown();
    }
    resource_.reset(nullptr);
}

void D2DRenderer::BeginFrame() {
    if (!resource_.get()) return;
    resource_.get()->BeginDraw();
    frameInProgress_ = true;
}

void D2DRenderer::EndFrame() {
    if (!resource_.get()) return;
    if (frameInProgress_) {
        resource_.get()->EndDraw();
        resource_.get()->Present();
        frameInProgress_ = false;
    }
}

void D2DRenderer::DrawShape(const std::string& type, float x, float y, float w, float h, unsigned int fillColor, unsigned int strokeColor, float strokeWidth) {
    if (!resource_.get()) return;

    if (type == "rect") {
        resource_.get()->DrawRect(x, y, w, h, fillColor, strokeWidth, strokeColor);
    } else if (type == "circle") {
        resource_.get()->DrawCircle(x, y, w / 2.0f, fillColor, strokeWidth, strokeColor);
    } else if (type == "line") {
        resource_.get()->DrawLine(x, y, x + w, y + h, fillColor, strokeWidth);
    }
}

void D2DRenderer::DrawText(const std::string& text, float x, float y, float w, float h, unsigned int color, float fontSize) {
    if (!resource_.get()) return;
    resource_.get()->DrawText(text, x, y, w, h, color, "Consolas", fontSize);
}

int D2DRenderer::GetWidth() const {
    if (!resource_.get()) return 0;
    return resource_.get()->GetWidth();
}

int D2DRenderer::GetHeight() const {
    if (!resource_.get()) return 0;
    return resource_.get()->GetHeight();
}

// ============================================================================
// PANEL RENDERER
// ============================================================================

PanelRenderer::PanelRenderer(D2DRenderer* renderer) : renderer_(renderer) {}

void PanelRenderer::DrawPanel(const PanelRenderContext& ctx) {
    if (!renderer_) return;

    DrawPanelBackground(ctx.x, ctx.y, ctx.w, ctx.h, ctx.bgColor);
    DrawPanelBorder(ctx.x, ctx.y, ctx.w, ctx.h, ctx.borderColor);
}

void PanelRenderer::DrawPanelBorder(float x, float y, float w, float h, unsigned int color) {
    if (!renderer_) return;

    // Draw 4 lines for border
    renderer_->DrawLine(x, y, x + w, y, color, 1.0f);
    renderer_->DrawLine(x + w, y, x + w, y + h, color, 1.0f);
    renderer_->DrawLine(x + w, y + h, x, y + h, color, 1.0f);
    renderer_->DrawLine(x, y + h, x, y, color, 1.0f);
}

void PanelRenderer::DrawPanelBackground(float x, float y, float w, float h, unsigned int color) {
    if (!renderer_) return;

    renderer_->FillRect(x, y, w, h, color);
}

void PanelRenderer::DrawPanelText(const std::string& text, float x, float y, float w, float h, unsigned int color) {
    if (!renderer_) return;

    renderer_->DrawText(text, x + 5, y + 5, w - 10, h - 10, color, 11.0f);
}
