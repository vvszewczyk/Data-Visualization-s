#include <windows.h>

// Windows API Application
static TCHAR lpszAppName[] = TEXT("API Windows");
HBITMAP bitmapBuffer;
HDC memoryContext;

// Window procedure for handling messages
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Create drawings when the window is created
        HGDIOBJ oldPen = SelectObject(memoryContext, CreatePen(PS_SOLID, 2, RGB(255, 0, 255)));
        HGDIOBJ oldBrush = SelectObject(memoryContext, CreateSolidBrush(RGB(255, 255, 0)));

        // Line
        MoveToEx(memoryContext, 50, 50, NULL); // Start point
        LineTo(memoryContext, 200, 200);       // End point

        // Rectangle, vertices in (100, 100), (300, 200)
        Rectangle(memoryContext, 100, 100, 300, 200);

        // Ellipse inscribed in a rectangle with vertices (150, 150) and (350, 250)
        Ellipse(memoryContext, 150, 200, 350, 250);

        // Rectangle with rounded corners (300, 300) to (500, 400), corner radius 100x100
        RoundRect(memoryContext, 300, 300, 500, 400, 100, 100);

        // A segment of a circle (a fragment of an ellipse)
        Pie(memoryContext, 200, 300, 400, 400, 300, 200, 300, 400);

        DeleteObject(SelectObject(memoryContext, oldPen));
        DeleteObject(SelectObject(memoryContext, oldBrush));
    } break;

    case WM_PAINT:
    {
        // Handle drawing when the window is refreshed
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);
        BitBlt(hDC, 0, 0, 640, 480, memoryContext, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_KEYDOWN:
    {
        // Handle keyboard input - close the application when ESC is pressed
        if (wParam == VK_ESCAPE) 
        {
            PostQuitMessage(0);
        }
    }
    break;

    case WM_DESTROY:
    {
        // Handle window destruction
        PostQuitMessage(0);
    }
    break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

// Main application function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Initialize memory context and bitmap buffer
    memoryContext = CreateCompatibleDC(NULL);
    bitmapBuffer = CreateCompatibleBitmap(GetDC(NULL), 640, 480);
    HGDIOBJ bmp = SelectObject(memoryContext, bitmapBuffer);

    // Register window class
    WNDCLASS wndclass = {};
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = MainWndProc;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszClassName = lpszAppName;

    if (!RegisterClass(&wndclass)) 
    {
        return FALSE;
    }

    // Create the window
    HWND hWnd = CreateWindow(
        lpszAppName, lpszAppName, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, NULL);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up resources
    DeleteObject(SelectObject(memoryContext, bmp));
    DeleteDC(memoryContext);

    return (int)msg.wParam;
}
