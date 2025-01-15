<<<<<<< HEAD
#include <windows.h>
#include <Scintilla.h>
#include <stdlib.h>
#include <stdio.h>

#define IDC_MAIN_EDIT 100

// Scintilla function pointers
typedef sptr_t (*SciFnDirect)(sptr_t ptr, unsigned int msg, uptr_t wParam, sptr_t lParam);
typedef sptr_t (*SciFnDirectStatus)(sptr_t ptr, unsigned int msg, uptr_t wParam, sptr_t lParam, int *status);

// Global variables
HMODULE hScintilla = NULL;
SciFnDirect Scintilla_DirectFunction;

// Function to create the Scintilla editor
HWND CreateScintillaEditor(HWND hwndParent, HINSTANCE hInstance) {
    // Load Scintilla DLL
    hScintilla = LoadLibrary("Scintilla.dll");
    if (!hScintilla) {
        MessageBox(NULL, "Failed to load Scintilla.dll!", "Error", MB_ICONERROR | MB_OK);
        return NULL;
    }

    // Get Scintilla's Direct Function
    Scintilla_DirectFunction = (SciFnDirect)GetProcAddress(hScintilla, "Scintilla_DirectFunction");
    if (!Scintilla_DirectFunction) {
        MessageBox(NULL, "Failed to get Scintilla_DirectFunction!", "Error", MB_ICONERROR | MB_OK);
        FreeLibrary(hScintilla);
        return NULL;
    }

    // Register Scintilla class
    WNDCLASS wc;
    if (!GetClassInfo(hScintilla, "Scintilla", &wc)) {
        MessageBox(NULL, "Scintilla class not registered!", "Error", MB_ICONERROR | MB_OK);
        FreeLibrary(hScintilla);
        return NULL;
    }

    // Create Scintilla window
    HWND hwndScintilla = CreateWindowEx(
        0,
        "Scintilla",
        "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN,
        0, 0, 0, 0,
        hwndParent,
        (HMENU)IDC_MAIN_EDIT,
        hInstance,
        NULL);

    if (!hwndScintilla) {
        DWORD error = GetLastError();
        char errorMsg[256];
        sprintf(errorMsg, "Failed to create Scintilla window! Error code: %lu", error);
        MessageBox(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
        FreeLibrary(hScintilla);
        return NULL;
    }

    return hwndScintilla;
}

// Function to send a message to Scintilla
sptr_t SendEditor(HWND hwndScintilla, UINT Msg, WPARAM wParam, LPARAM lParam) {
    return SendMessage(hwndScintilla, Msg, wParam, lParam);
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hwndScintilla = NULL;
    
    switch (msg) {
        case WM_CREATE: {
            // Create Scintilla editor window
            hwndScintilla = CreateScintillaEditor(hwnd, ((LPCREATESTRUCT)lParam)->hInstance);
            if (!hwndScintilla) {
                return -1;  // Fail window creation
            }

            // Initialize editor settings
            SendEditor(hwndScintilla, SCI_STYLECLEARALL, 0, 0);
            SendEditor(hwndScintilla, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
            
            // Set up basic styling
            SendEditor(hwndScintilla, SCI_STYLESETFORE, STYLE_DEFAULT, RGB(0, 0, 0));
            SendEditor(hwndScintilla, SCI_STYLESETBACK, STYLE_DEFAULT, RGB(255, 255, 255));
            
            // Enable line numbers
            SendEditor(hwndScintilla, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
            SendEditor(hwndScintilla, SCI_SETMARGINWIDTHN, 0, 45);

            // Set basic features
            SendEditor(hwndScintilla, SCI_SETUNDOCOLLECTION, 1, 0);
            SendEditor(hwndScintilla, SCI_SETCARETLINEVISIBLE, 1, 0);
            SendEditor(hwndScintilla, SCI_SETCARETLINEBACK, RGB(240, 240, 240), 0);
            
            // Set tab width
            SendEditor(hwndScintilla, SCI_SETTABWIDTH, 4, 0);
            
            // Enable automatic indentation
            SendEditor(hwndScintilla, SCI_SETINDENTATIONGUIDES, SC_IV_REAL, 0);
            SendEditor(hwndScintilla, SCI_SETINDENT, 4, 0);
            
            // Set word wrap
            SendEditor(hwndScintilla, SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
            
            // Set font
            SendEditor(hwndScintilla, SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)"Consolas");
            SendEditor(hwndScintilla, SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
            
            return 0;
        }
        
        case WM_SIZE: {
            if (hwndScintilla) {
                SetWindowPos(hwndScintilla,
                    NULL,
                    0, 0,
                    LOWORD(lParam), HIWORD(lParam),
                    SWP_NOZORDER);
            }
            return 0;
        }

        case WM_NOTIFY: {
            LPNMHDR notification = (LPNMHDR)lParam;
            if (notification->idFrom == IDC_MAIN_EDIT) {
                SCNotification *scn = (SCNotification*)lParam;
                switch (notification->code) {
                    case SCN_CHARADDED:
                    case SCN_MODIFIED:
                        // Handle notifications if needed
                        break;
                }
            }
            return 0;
        }

        case WM_DESTROY:
            if (hScintilla) {
                FreeLibrary(hScintilla);
            }
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    
    // Register window class
    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW,
        WndProc,
        0, 0,
        hInstance,
        LoadIcon(NULL, IDI_APPLICATION),
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW + 1),
        NULL,
        "ScintillaEditor",
        LoadIcon(NULL, IDI_APPLICATION)
    };
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    // Create main window
    HWND hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "ScintillaEditor",
        "Simple Scintilla Editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL,
        hInstance,
        NULL
    );
    
    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return msg.wParam;
=======
#include <windows.h>
#include <Scintilla.h>
#include <stdlib.h>
#include <stdio.h>

#define IDC_MAIN_EDIT 100

// Scintilla function pointers
typedef sptr_t (*SciFnDirect)(sptr_t ptr, unsigned int msg, uptr_t wParam, sptr_t lParam);
typedef sptr_t (*SciFnDirectStatus)(sptr_t ptr, unsigned int msg, uptr_t wParam, sptr_t lParam, int *status);

// Global variables
HMODULE hScintilla = NULL;
SciFnDirect Scintilla_DirectFunction;

// Function to create the Scintilla editor
HWND CreateScintillaEditor(HWND hwndParent, HINSTANCE hInstance) {
    // Load Scintilla DLL
    hScintilla = LoadLibrary("Scintilla.dll");
    if (!hScintilla) {
        MessageBox(NULL, "Failed to load Scintilla.dll!", "Error", MB_ICONERROR | MB_OK);
        return NULL;
    }

    // Get Scintilla's Direct Function
    Scintilla_DirectFunction = (SciFnDirect)GetProcAddress(hScintilla, "Scintilla_DirectFunction");
    if (!Scintilla_DirectFunction) {
        MessageBox(NULL, "Failed to get Scintilla_DirectFunction!", "Error", MB_ICONERROR | MB_OK);
        FreeLibrary(hScintilla);
        return NULL;
    }

    // Register Scintilla class
    WNDCLASS wc;
    if (!GetClassInfo(hScintilla, "Scintilla", &wc)) {
        MessageBox(NULL, "Scintilla class not registered!", "Error", MB_ICONERROR | MB_OK);
        FreeLibrary(hScintilla);
        return NULL;
    }

    // Create Scintilla window
    HWND hwndScintilla = CreateWindowEx(
        0,
        "Scintilla",
        "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN,
        0, 0, 0, 0,
        hwndParent,
        (HMENU)IDC_MAIN_EDIT,
        hInstance,
        NULL);

    if (!hwndScintilla) {
        DWORD error = GetLastError();
        char errorMsg[256];
        sprintf(errorMsg, "Failed to create Scintilla window! Error code: %lu", error);
        MessageBox(NULL, errorMsg, "Error", MB_ICONERROR | MB_OK);
        FreeLibrary(hScintilla);
        return NULL;
    }

    return hwndScintilla;
}

// Function to send a message to Scintilla
sptr_t SendEditor(HWND hwndScintilla, UINT Msg, WPARAM wParam, LPARAM lParam) {
    return SendMessage(hwndScintilla, Msg, wParam, lParam);
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hwndScintilla = NULL;
    
    switch (msg) {
        case WM_CREATE: {
            // Create Scintilla editor window
            hwndScintilla = CreateScintillaEditor(hwnd, ((LPCREATESTRUCT)lParam)->hInstance);
            if (!hwndScintilla) {
                return -1;  // Fail window creation
            }

            // Initialize editor settings
            SendEditor(hwndScintilla, SCI_STYLECLEARALL, 0, 0);
            SendEditor(hwndScintilla, SCI_SETCODEPAGE, SC_CP_UTF8, 0);
            
            // Set up basic styling
            SendEditor(hwndScintilla, SCI_STYLESETFORE, STYLE_DEFAULT, RGB(0, 0, 0));
            SendEditor(hwndScintilla, SCI_STYLESETBACK, STYLE_DEFAULT, RGB(255, 255, 255));
            
            // Enable line numbers
            SendEditor(hwndScintilla, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);
            SendEditor(hwndScintilla, SCI_SETMARGINWIDTHN, 0, 45);

            // Set basic features
            SendEditor(hwndScintilla, SCI_SETUNDOCOLLECTION, 1, 0);
            SendEditor(hwndScintilla, SCI_SETCARETLINEVISIBLE, 1, 0);
            SendEditor(hwndScintilla, SCI_SETCARETLINEBACK, RGB(240, 240, 240), 0);
            
            // Set tab width
            SendEditor(hwndScintilla, SCI_SETTABWIDTH, 4, 0);
            
            // Enable automatic indentation
            SendEditor(hwndScintilla, SCI_SETINDENTATIONGUIDES, SC_IV_REAL, 0);
            SendEditor(hwndScintilla, SCI_SETINDENT, 4, 0);
            
            // Set word wrap
            SendEditor(hwndScintilla, SCI_SETWRAPMODE, SC_WRAP_WORD, 0);
            
            // Set font
            SendEditor(hwndScintilla, SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)"Consolas");
            SendEditor(hwndScintilla, SCI_STYLESETSIZE, STYLE_DEFAULT, 10);
            
            return 0;
        }
        
        case WM_SIZE: {
            if (hwndScintilla) {
                SetWindowPos(hwndScintilla,
                    NULL,
                    0, 0,
                    LOWORD(lParam), HIWORD(lParam),
                    SWP_NOZORDER);
            }
            return 0;
        }

        case WM_NOTIFY: {
            LPNMHDR notification = (LPNMHDR)lParam;
            if (notification->idFrom == IDC_MAIN_EDIT) {
                SCNotification *scn = (SCNotification*)lParam;
                switch (notification->code) {
                    case SCN_CHARADDED:
                    case SCN_MODIFIED:
                        // Handle notifications if needed
                        break;
                }
            }
            return 0;
        }

        case WM_DESTROY:
            if (hScintilla) {
                FreeLibrary(hScintilla);
            }
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    
    // Register window class
    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW,
        WndProc,
        0, 0,
        hInstance,
        LoadIcon(NULL, IDI_APPLICATION),
        LoadCursor(NULL, IDC_ARROW),
        (HBRUSH)(COLOR_WINDOW + 1),
        NULL,
        "ScintillaEditor",
        LoadIcon(NULL, IDI_APPLICATION)
    };
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    // Create main window
    HWND hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "ScintillaEditor",
        "Simple Scintilla Editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL, NULL,
        hInstance,
        NULL
    );
    
    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return msg.wParam;
>>>>>>> 3be01a2 (The repository in git is created with the latest progress in the code uploaded)
}