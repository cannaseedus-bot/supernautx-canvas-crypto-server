/// workspace_editor_test.cpp
/// Phase 7.19 Editor Test — Single-Window D2D Editor Verification

#include "workspace_editor_d2d_minimal.hpp"
#include <windows.h>
#include <iostream>

D2DEditor* g_editor = nullptr;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT: {
            ValidateRect(hwnd, NULL);
            return 0;
        }
            
        case WM_KEYDOWN:
            if (g_editor) {
                if (wParam >= 32 && wParam <= 126) {
                    g_editor->InsertChar((char)wParam);
                }
            }
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
            
        case WM_KEYUP:
            return 0;
            
        case WM_CHAR:
            return 0;
            
        case WM_MOUSEMOVE:
            return 0;
            
        case WM_LBUTTONDOWN:
            return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    const wchar_t CLASS_NAME[] = L"EditorWindowClass";
    
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Window Registration Failed", L"Error", MB_ICONEXCLAMATION);
        return 0;
    }
    
    // Create window
    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Phase 7.19 Editor Test",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
        NULL, NULL, hInstance, NULL);
    
    if (!hwnd) {
        MessageBoxW(NULL, L"Window Creation Failed", L"Error", MB_ICONEXCLAMATION);
        return 0;
    }
    
    // Create editor
    g_editor = new D2DEditor();
    g_editor->Initialize(1024, 768);
    g_editor->InsertChar('H');
    g_editor->InsertChar('i');
    
    std::cout << "Editor initialized: " << g_editor->GetLines().size() << " lines\n";
    
    // Message loop
    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    delete g_editor;
    return (int)msg.wParam;
}
