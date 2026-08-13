#ifdef _WIN32
#include "gui.h"
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <string.h>

#define IDC_OPEN 1001
#define IDC_START 1002
#define IDC_PAUSE 1003
#define IDC_STATUS 1004
#define IDC_PROGRESS 1005
#define IDC_TORRENT 1006
#define IDC_DEST 1007

static HWND hTorrent, hDest, hStatus, hProgress;
static HINSTANCE gInst;
static PROCESS_INFORMATION child = {0};

static void set_status(const char *s) { SetWindowTextA(hStatus, s); }

static void open_torrent(HWND owner) {
    char path[MAX_PATH] = {0};
    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = owner;
    ofn.lpstrFilter = "BitTorrent files (*.torrent)\0*.torrent\0All files\0*.*\0";
    ofn.lpstrFile = path; ofn.nMaxFile = MAX_PATH; ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) SetWindowTextA(hTorrent, path);
}

static void start_download(HWND owner) {
    char torrent[MAX_PATH] = {0}, dest[MAX_PATH] = {0}, exe[MAX_PATH] = {0};
    GetWindowTextA(hTorrent, torrent, MAX_PATH); GetWindowTextA(hDest, dest, MAX_PATH);
    if (!torrent[0] || !dest[0]) { set_status("Choose a torrent and destination first."); return; }
    if (child.hProcess) { DWORD code; if (GetExitCodeProcess(child.hProcess, &code) && code == STILL_ACTIVE) { set_status("Download already running."); return; } CloseHandle(child.hProcess); CloseHandle(child.hThread); memset(&child,0,sizeof(child)); }
    GetModuleFileNameA(NULL, exe, MAX_PATH);
    char cmd[3*MAX_PATH + 32];
    snprintf(cmd, sizeof(cmd), "\"%s\" \"%s\" \"%s\"", exe, torrent, dest);
    STARTUPINFOA si = {0}; si.cb = sizeof(si);
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &child)) {
        char msg[128]; snprintf(msg,sizeof(msg),"Could not start downloader (error %lu).",GetLastError()); set_status(msg); return;
    }
    set_status("Download started. The CLI engine is running in the background.");
    (void)owner;
}

static LRESULT CALLBACK wndproc(HWND w, UINT m, WPARAM wp, LPARAM lp) {
    switch (m) {
    case WM_CREATE:
        CreateWindowA("STATIC", "Torrent:", WS_CHILD|WS_VISIBLE, 18, 20, 70, 22, w, 0, gInst, 0);
        hTorrent = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL, 85, 17, 390, 26, w, (HMENU)IDC_TORRENT, gInst, 0);
        CreateWindowA("BUTTON", "Browse...", WS_CHILD|WS_VISIBLE, 485, 17, 90, 26, w, (HMENU)IDC_OPEN, gInst, 0);
        CreateWindowA("STATIC", "Save to:", WS_CHILD|WS_VISIBLE, 18, 58, 70, 22, w, 0, gInst, 0);
        hDest = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "download.iso", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL, 85, 55, 490, 26, w, (HMENU)IDC_DEST, gInst, 0);
        CreateWindowA("BUTTON", "Start", WS_CHILD|WS_VISIBLE, 85, 98, 90, 30, w, (HMENU)IDC_START, gInst, 0);
        CreateWindowA("BUTTON", "Stop", WS_CHILD|WS_VISIBLE, 185, 98, 90, 30, w, (HMENU)IDC_PAUSE, gInst, 0);
        hProgress = CreateWindowExA(0, PROGRESS_CLASSA, "", WS_CHILD|WS_VISIBLE, 18, 145, 557, 24, w, (HMENU)IDC_PROGRESS, gInst, 0);
        SendMessageA(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 1000));
        hStatus = CreateWindowA("STATIC", "Ready", WS_CHILD|WS_VISIBLE, 18, 180, 557, 24, w, (HMENU)IDC_STATUS, gInst, 0);
        break;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_OPEN: open_torrent(w); break;
        case IDC_START: start_download(w); break;
        case IDC_PAUSE:
            if (child.hProcess) { TerminateProcess(child.hProcess, 0); CloseHandle(child.hProcess); CloseHandle(child.hThread); memset(&child,0,sizeof(child)); }
            set_status("Stopped"); break;
        }
        break;
    case WM_DESTROY:
        if (child.hProcess) { TerminateProcess(child.hProcess, 0); CloseHandle(child.hProcess); CloseHandle(child.hThread); }
        PostQuitMessage(0); break;
    }
    return DefWindowProcA(w, m, wp, lp);
}

int gui_run(int argc, char **argv) {
    (void)argc; (void)argv;
    INITCOMMONCONTROLSEX ic = {sizeof(ic), ICC_PROGRESS_CLASS}; InitCommonControlsEx(&ic);
    gInst = GetModuleHandleA(NULL);
    WNDCLASSA wc = {0}; wc.lpfnWndProc = wndproc; wc.hInstance = gInst; wc.lpszClassName = "BTClientWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClassA(&wc);
    HWND w = CreateWindowA("BTClientWindow", "BitTorrent Client", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 260, NULL, NULL, gInst, NULL);
    ShowWindow(w, SW_SHOW); UpdateWindow(w);
    MSG msg; while (GetMessageA(&msg, NULL, 0, 0) > 0) { TranslateMessage(&msg); DispatchMessageA(&msg); }
    return (int)msg.wParam;
}
#endif
