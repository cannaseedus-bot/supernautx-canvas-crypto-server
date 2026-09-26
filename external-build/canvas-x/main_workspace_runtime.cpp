/// main_workspace_runtime.cpp
/// Phase 7.19 Workspace Runtime — Full 8-Panel IDE Application Entry Point

#include "workspace_runtime.hpp"
#include <windows.h>
#include <iostream>
#include <memory>

WorkspaceRuntime* g_runtime = nullptr;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT:
            if (g_runtime) {
                g_runtime->RenderFrame();
            }
            ValidateRect(hwnd, NULL);
            return 0;
            
        case WM_KEYDOWN:
            if (g_runtime) {
                g_runtime->OnKeyPress((int)wParam);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
            
        case WM_KEYUP:
            if (g_runtime) {
                g_runtime->OnKeyRelease((int)wParam);
            }
            return 0;
            
        case WM_CHAR:
            if (g_runtime) {
                g_runtime->OnChar((wchar_t)wParam);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
            
        case WM_LBUTTONDOWN:
            if (g_runtime) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                g_runtime->OnMouseClick(x, y, 1);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
            
        case WM_MOUSEMOVE:
            if (g_runtime) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                g_runtime->OnMouseMove(x, y);
            }
            return 0;
            
        case WM_MOUSEWHEEL:
            if (g_runtime) {
                int x = GET_X_LPARAM(lParam);
                int y = GET_Y_LPARAM(lParam);
                int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                g_runtime->OnMouseWheel(x, y, delta);
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
            
        case WM_SIZE:
            if (g_runtime) {
                // Trigger resize handling if needed
            }
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main(int argc, char* argv[]) {
    const wchar_t CLASS_NAME[] = L"Phase7.19WorkspaceRuntimeClass";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClass(&wc);
    
    // Create main window (1280×1024 for 8-panel layout)
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Phase 7.19: Unified Workspace Runtime",
        WS_OVERLAPPEDWINDOW,
        
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 1024,
        
        NULL,
        NULL,
        NULL,
        NULL
    );
    
    if (hwnd == NULL) {
        std::cerr << "Failed to create window" << std::endl;
        return 1;
    }
    
    // Initialize workspace runtime
    g_runtime = new WorkspaceRuntime();
    if (!g_runtime->InitializeWithCanvas(hwnd, 1280, 1024)) {
        std::cerr << "Failed to initialize workspace runtime" << std::endl;
        return 1;
    }
    
    std::cout << "Phase 7.19 Workspace Runtime initialized" << std::endl;
    std::cout << "8 subsystems online:" << std::endl;
    std::cout << "  - Editor (center)" << std::endl;
    std::cout << "  - Chat (right top)" << std::endl;
    std::cout << "  - Files (bottom left)" << std::endl;
    std::cout << "  - Terminal (bottom center)" << std::endl;
    std::cout << "  - Canvas (right bottom)" << std::endl;
    std::cout << "  - Timeline (bottom full-width)" << std::endl;
    std::cout << "  - Agents (registry)" << std::endl;
    std::cout << "  - Workflows (orchestrator)" << std::endl;
    
    ShowWindow(hwnd, SW_SHOW);
    
    // Main message loop with frame timing
    MSG msg = {};
    LARGE_INTEGER freq, lastTime, currentTime;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);
    
    double totalTime = 0;
    int frameCount = 0;
    
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        
        // Measure frame time
        QueryPerformanceCounter(&currentTime);
        double deltaTime = (double)(currentTime.QuadPart - lastTime.QuadPart) / freq.QuadPart;
        lastTime = currentTime;
        
        // Update and render
        if (g_runtime) {
            g_runtime->UpdateFrame(deltaTime);
            g_runtime->RenderFrame();
            
            totalTime += deltaTime;
            frameCount++;
            
            // Print FPS every 60 frames
            if (frameCount % 60 == 0) {
                double avgTime = totalTime / frameCount;
                double fps = 1.0 / avgTime;
                std::cout << "FPS: " << fps << " (avg " << (avgTime * 1000) << "ms)" << std::endl;
            }
        }
    }
    
    // Print final stats
    if (frameCount > 0) {
        double avgTime = totalTime / frameCount;
        std::cout << "Rendered " << frameCount << " frames in " << totalTime << "s" << std::endl;
        std::cout << "Average frame time: " << (avgTime * 1000) << "ms" << std::endl;
    }
    
    // Cleanup
    if (g_runtime) {
        g_runtime->Shutdown();
        delete g_runtime;
    }
    
    std::cout << "Phase 7.19 Workspace Runtime shutdown complete" << std::endl;
    return 0;
}
