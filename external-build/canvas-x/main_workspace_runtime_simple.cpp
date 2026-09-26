/// main_workspace_runtime_simple.cpp
/// Minimal workspace runtime executable (MVP)
/// Demonstrates editor + chat + HTTP agent dispatch

#include "workspace_runtime.hpp"
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>

WorkspaceRuntime g_workspace;
int g_width = 1024;
int g_height = 768;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN: {
            int vkey = (int)wParam;
            g_workspace.OnKeyPress(vkey);
            return 0;
        }
        case WM_KEYUP: {
            int vkey = (int)wParam;
            g_workspace.OnKeyRelease(vkey);
            return 0;
        }
        case WM_CHAR: {
            wchar_t c = (wchar_t)wParam;
            g_workspace.OnChar(c);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            g_workspace.OnMouseClick(x, y, 0);
            return 0;
        }
        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            g_workspace.OnMouseMove(x, y);
            return 0;
        }
        case WM_MOUSEWHEEL: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            g_workspace.OnMouseWheel(x, y, delta);
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            // Basic paint (MVP: just white background)
            FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
            EndPaint(hwnd, &ps);
            return 0;
        }
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register window class
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"WorkspaceRuntimeClass";
    RegisterClassW(&wc);

    // Create window
    HWND hwnd = CreateWindowExW(
        0,
        L"WorkspaceRuntimeClass",
        L"Phase 7.19 Workspace Runtime (MVP)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, g_width, g_height,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 1;

    // Initialize workspace
    g_workspace.Initialize(g_width, g_height);

    // Show window
    ShowWindow(hwnd, nCmdShow);

    // Message loop
    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        // Update and render
        static DWORD lastTime = GetTickCount();
        DWORD now = GetTickCount();
        float deltaTime = (now - lastTime) / 1000.0f;
        lastTime = now;

        g_workspace.Update(deltaTime);
        InvalidateRect(hwnd, NULL, FALSE);
    }

    g_workspace.Shutdown();
    return (int)msg.wParam;
}
