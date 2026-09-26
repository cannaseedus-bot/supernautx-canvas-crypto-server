/// test_d2d_rendering.cpp
/// Test D2D rendering engine with shapes and text

#include "workspace_d2d_rendering.hpp"
#include <iostream>
#include <chrono>
#include <thread>

const int WINDOW_WIDTH = 1024;
const int WINDOW_HEIGHT = 768;

// Global renderer
D2DRenderer* g_renderer = nullptr;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT: {
        if (!g_renderer) break;

        g_renderer->BeginFrame();
        g_renderer->Clear(0xFFFFFFFF); // White background

        // Draw rectangles
        g_renderer->DrawRect(50, 50, 200, 100, 0xFF0000FF); // Blue
        g_renderer->DrawRect(300, 50, 200, 100, 0xFF00FF00); // Green
        g_renderer->DrawRect(550, 50, 200, 100, 0xFFFF0000); // Red

        // Draw circles
        g_renderer->DrawCircle(150, 250, 50, 0xFF00FFFF); // Cyan
        g_renderer->DrawCircle(400, 250, 50, 0xFFFFFF00); // Yellow
        g_renderer->DrawCircle(650, 250, 50, 0xFFFF00FF); // Magenta

        // Draw lines (grid)
        unsigned int gridColor = 0xFF808080; // Gray
        for (int x = 0; x < WINDOW_WIDTH; x += 100) {
            g_renderer->DrawLine(x, 350, x, 500, gridColor, 1.0f);
        }
        for (int y = 350; y <= 500; y += 50) {
            g_renderer->DrawLine(0, y, WINDOW_WIDTH, y, gridColor, 1.0f);
        }

        // Draw text
        g_renderer->DrawText("D2D Rendering Test", 50, 550, 400, 50, 0xFF000000, 14.0f);
        g_renderer->DrawText("Shapes: Rects, Circles, Lines", 50, 600, 400, 50, 0xFF000000, 12.0f);
        g_renderer->DrawText("Text rendering working!", 50, 650, 400, 50, 0xFF000000, 12.0f);

        g_renderer->EndFrame();
        break;
    }
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    // Register window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"D2DRenderingTest";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    if (!RegisterClassW(&wc)) {
        std::cerr << "Failed to register window class" << std::endl;
        return 1;
    }

    // Create window
    HWND hwnd = CreateWindowExW(
        0,
        L"D2DRenderingTest",
        L"Phase 7.20 D2D Rendering Test",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        std::cerr << "Failed to create window" << std::endl;
        return 1;
    }

    // Create renderer
    g_renderer = new D2DRenderer();
    if (!g_renderer->Initialize(hwnd, WINDOW_WIDTH, WINDOW_HEIGHT)) {
        std::cerr << "Failed to initialize D2D renderer" << std::endl;
        delete g_renderer;
        return 1;
    }

    std::cout << "✓ D2D Renderer initialized successfully" << std::endl;
    std::cout << "  Window: " << WINDOW_WIDTH << "x" << WINDOW_HEIGHT << std::endl;
    std::cout << "  Rendering shapes and text..." << std::endl;

    // Show window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg = {};
    auto startTime = std::chrono::high_resolution_clock::now();
    int frameCount = 0;

    while (true) {
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Trigger paint
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
        frameCount++;

        // Run for 5 seconds
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
        if (duration.count() >= 5000) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }

    // Calculate FPS
    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    double fps = frameCount * 1000.0 / totalDuration.count();

    std::cout << "\n✓ Test completed successfully" << std::endl;
    std::cout << "  Frames rendered: " << frameCount << std::endl;
    std::cout << "  Duration: " << totalDuration.count() << " ms" << std::endl;
    std::cout << "  FPS: " << fps << " (target 60)" << std::endl;

    // Cleanup
    g_renderer->Shutdown();
    delete g_renderer;
    DestroyWindow(hwnd);

    return 0;
}
