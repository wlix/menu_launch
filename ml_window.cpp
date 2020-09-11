#include <shlwapi.h>
#include <algorithm>

#include "ml_xml.hpp"
#include "ml_folder.hpp"
#include "ml_window.hpp"

#include "Utility.hpp"

#include "resource.h"

#pragma comment(lib, "shlwapi.lib")

#pragma comment(linker, "/section:.shared,rws")
#pragma data_seg(".shared")
HHOOK  g_hKeyhook { nullptr };
#pragma data_seg()

MLMenu g_MLMenu;

extern HINSTANCE g_hInst;
extern HWND      g_hMainWnd;

HWND WINAPI CreateMainWnd(HINSTANCE hInst, LPCTSTR ClassName, LPCTSTR WindowName) {
    WNDCLASS wc;
    HWND     hWnd;

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInst;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = ClassName;

    RegisterClass(&wc);

    hWnd = CreateWindowEx(
        WS_EX_TOOLWINDOW, ClassName, WindowName,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        200, 200, NULL, NULL, hInst, NULL);

    return hWnd;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    static HMENU hSelectedMenu;
    static int   iSelectedMenuItemIndex;

    switch (msg) {
    case WM_MENURBUTTONUP:
        OnMenuRbuttonUp(hWnd, wp, lp);
        return 0;

    case WM_MENUSELECT:
        hSelectedMenu = (HMENU)lp;
        if (HIWORD(wp) & MF_POPUP)
            iSelectedMenuItemIndex = LOWORD(wp);
        else
            iSelectedMenuItemIndex = GetMenuItemPosFromID(hSelectedMenu, LOWORD(wp));
        return 0;

    case WM_INITMENUPOPUP:
        folder_HandleMenuMsg(msg, wp, lp);
        folder_OnInitMenuPopup((HMENU)wp, g_MLMenu.FolderMenus);

        return TRUE;

    case WM_UNINITMENUPOPUP:
        break;

    case WM_MENUCHAR:
        folder_HandleMenuMsg(msg, wp, lp);
        break;

    case WM_VK_APPS:
        if (hSelectedMenu != NULL && iSelectedMenuItemIndex != -1) {
            RECT rc;
            GetMenuItemRect(NULL, hSelectedMenu, iSelectedMenuItemIndex, &rc);
            POINT pt = { (rc.right + rc.left) / 2, (rc.bottom + rc.top) / 2 };
            if (folder_ContextMenu(hWnd, hSelectedMenu, iSelectedMenuItemIndex, &pt))
                break;
            else
                window_ContextMenu(hWnd, GetMenuItemID(hSelectedMenu, iSelectedMenuItemIndex), pt);
        }
        return 0;

    case WM_EXECUTE_COMMAND:
        OnExecuteCommand(hWnd, wp, lp);
        return 0;

    case WM_CREATE:
        Init(hWnd);
        return 0;

    case WM_MEASUREITEM:
        if (folder_HandleMenuMsg(msg, wp, lp))
            return TRUE;
        if (wp == 0)
        { // デフォルトアイテムを隠す
            LPMEASUREITEMSTRUCT pmis = (LPMEASUREITEMSTRUCT)lp;
            pmis->itemWidth = 0;
            pmis->itemHeight = 0;
        }
        return TRUE;

    case WM_DRAWITEM:
        folder_HandleMenuMsg(msg, wp, lp);
        return TRUE;

    case WM_DESTROY:
        Uninit();
        return 0;
    }
    return DefWindowProc(hWnd, msg, wp, lp);
}

void WINAPI OnMenuRbuttonUp(HWND hWnd, WPARAM wp, LPARAM lp) {
    HMENU hmenu = (HMENU)lp;

    POINT pt;
    GetCursorPos(&pt);
    if (folder_ContextMenu(hWnd, hmenu, wp, &pt)) {
        return;
    }
    else {
        window_ContextMenu(hWnd, GetMenuItemID(hmenu, wp), pt);
    }
}

int WINAPI GetMenuItemPosFromID(HMENU hMenu, int ID) {
    int c = GetMenuItemCount(hMenu);
    for (int i = 0; i < c; i++) {
        if (ID == GetMenuItemID(hMenu, i))
            return i;
    }
    return -1;
}

void WINAPI window_ContextMenu(HWND hWnd, int wID, POINT pt) {
    static BOOL bPopuped = FALSE; //2重にポップアップするのを防ぐ

    if (bPopuped)
        return;

    bPopuped = TRUE;

    if (0 <= wID && wID <= g_MLMenu.MenuItems.size())
    {
        HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU1));
        HMENU hSubMenu = GetSubMenu(hMenu, 0);

        if (wID == 0 || g_MLMenu.MenuItems[wID - 1].type != MenuItem::EXECUTE) {
            EnableMenuItem(hSubMenu, 1, MF_BYPOSITION | MF_GRAYED);
        }

        int ret = TrackPopupMenuEx(hSubMenu, TPM_RECURSE | TPM_RETURNCMD, pt.x, pt.y, hWnd, NULL);

        DestroyMenu(hMenu);

        switch (ret) {
        case ID_EXECUTE:

            switch (g_MLMenu.MenuItems[wID - 1].type)
            {
            case MenuItem::EXECUTE:
                Execute(g_MLMenu.MenuItems[wID - 1].execute.Path,
                    g_MLMenu.MenuItems[wID - 1].execute.Param, hWnd);
                break;
            case MenuItem::PLUGINCOMMAND:
                if (g_MLMenu.MenuItems[wID - 1].command.PluginFilename == g_info.Filename)
                    SendMessage(hWnd, WM_EXECUTE_COMMAND, g_MLMenu.MenuItems[wID - 1].command.CommandID, 0);
                else
                    ExecutePluginCommand(g_MLMenu.MenuItems[wID - 1].command.PluginFilename.c_str(),
                        g_MLMenu.MenuItems[wID - 1].command.CommandID);
                break;
            }
            break;
        case ID_OPENFOLDER:
            TCHAR szFolder[MAX_PATH];
            lstrcpyn(szFolder, g_MLMenu.MenuItems[wID - 1].execute.Path.c_str(), MAX_PATH);
            PathRemoveFileSpec(szFolder);
            if (32 >= (int)ShellExecute(NULL, NULL, szFolder, NULL, NULL, SW_SHOWNORMAL))
                MessageBox(hWnd, TEXT("開けません"), TEXT("エラー"), MB_OK);
            break;
        case ID_OPENXML:
            TCHAR szfilename[MAX_PATH];
            GetModuleFileName(g_hInst, szfilename, MAX_PATH);
            PathRenameExtension(szfilename, TEXT(".xml"));
            ShellExecute(NULL, NULL, szfilename, NULL, NULL, SW_SHOWNORMAL);
            break;
        case ID_RELOADXML:
            // メニューを消す
            keybd_event(VK_MENU, 0, 0, 0);
            keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);

            Uninit();
            Init(hWnd);
            break;
        }
    }
    bPopuped = FALSE;
}

void WINAPI OnExecuteCommand(HWND hWnd, WPARAM wp, LPARAM lp) {
    POINT pt;
    int ret;
    HWND hForeground;
    TPMPOS tp = { CURSOR };

    switch (wp) {
    case CMDID_LAUNCH:
    case CMDID_LAUNCH_ON_SPECIFIED:
    case CMDID_LAUNCH_ON_CURSOR:
        hForeground = GetForegroundWindow();
        SetForegroundWindow(hWnd);
        GetCursorPos(&pt);

        switch (wp) {
        case CMDID_LAUNCH:
        case CMDID_LAUNCH_ON_CURSOR:
            ret = TrackPopupMenuExPos(g_MLMenu.hMenu, TPM_RETURNCMD, &tp, hWnd, NULL);
            break;

        case CMDID_LAUNCH_ON_SPECIFIED:
            ret = TrackPopupMenuExPos(g_MLMenu.hMenu, TPM_RETURNCMD, &g_MLMenu.MenuPos, hWnd, NULL);
            break;
        }

        if (0 < ret && ret <= g_MLMenu.MenuItems.size())
        {
            switch (g_MLMenu.MenuItems[ret - 1].type)
            {
            case MenuItem::EXECUTE:
                Execute(g_MLMenu.MenuItems[ret - 1].execute.Path,
                    g_MLMenu.MenuItems[ret - 1].execute.Param, hWnd);
                break;
            case MenuItem::PLUGINCOMMAND:
                if (g_MLMenu.MenuItems[ret - 1].command.PluginFilename == g_info.Filename)
                    SendMessage(hWnd, WM_EXECUTE_COMMAND, g_MLMenu.MenuItems[ret - 1].command.CommandID, 0);
                else
                    ExecutePluginCommand(g_MLMenu.MenuItems[ret - 1].command.PluginFilename.c_str(),
                        g_MLMenu.MenuItems[ret - 1].command.CommandID);
                break;
            }
        }
        else
            folder_Execute(ret);

        folder_clear();
        SetForegroundWindow(hForeground);
        SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
        break;
    case CMDID_RELOAD_SETTINGS:
        Uninit();
        Init(hWnd);
        break;
    }
}

void WINAPI Init(HWND hWnd) {
    TCHAR szfilename[MAX_PATH];
    GetModuleFileName(g_hInst, szfilename, MAX_PATH);
    PathRenameExtension(szfilename, TEXT(".xml"));

    if (!LoadXML(szfilename, g_MLMenu)) {
        AppendMenu(g_MLMenu.hMenu, MFS_DISABLED, 0, TEXT("(empty)"));
    }

    folder_Init(hWnd, g_MLMenu.MenuItems.size() + 1);

    g_hKeyhook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, GetCurrentThreadId());
}

void WINAPI Uninit() {
    UnhookWindowsHookEx(g_hKeyhook);

    std::for_each(g_MLMenu.hBitmaps.begin(), g_MLMenu.hBitmaps.end(), DeleteObject);

    g_MLMenu.hBitmaps.clear();
    g_MLMenu.FolderMenus.clear();
    g_MLMenu.MenuItems.clear();

    DestroyMenu(g_MLMenu.hMenu);
}

BOOL WINAPI Execute(const tstring& Path, const tstring& Param, HWND hWnd) {
    TCHAR szDir[MAX_PATH];
    LPSTR pParam = NULL;

    lstrcpyn(szDir, Path.c_str(), MAX_PATH);
    PathRemoveFileSpec(szDir);
    int ret = (int)ShellExecute(NULL, NULL, Path.c_str(), Param != TEXT("") ? Param.c_str() : NULL, szDir, SW_SHOWNORMAL);
    if (!(ret > 32))
    {
        TCHAR szBuff[1024];
        wsprintf(szBuff, TEXT("実行できません\n パス : %s\n パラメータ : %s"),
            Path.c_str(), Param.c_str());
        MessageBox(hWnd, szBuff, TEXT("エラー"), MB_OK);
    }
    return TRUE;
}

//---------------------------------------------------------
//  マウスカーソルやスクリーンを基準にしてメニューの位置を指定する
//  TrackPopupMenuEx
//---------------------------------------------------------
BOOL WINAPI TrackPopupMenuExPos(HMENU hmenu, UINT fuFlags, TPMPOS* ptp, HWND hwnd, LPTPMPARAMS lptpm) {
    int x, y;
    POINT pt;
    RECT rc;

    switch (ptp != NULL ? ptp->type : CENTER) {
    case CURSOR:
        GetCursorPos(&pt);
        x = pt.x;
        y = pt.y;
        break;
    case CENTER:
        fuFlags |= TPM_CENTERALIGN | TPM_VCENTERALIGN;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
        x = rc.right / 2;
        y = rc.bottom / 2;
        break;
    case TOPLEFT:
        fuFlags |= TPM_TOPALIGN | TPM_LEFTALIGN;
        x = 0;
        y = 0;
        break;
    case TOPRIGHT:
        fuFlags |= TPM_TOPALIGN | TPM_RIGHTALIGN;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
        x = rc.right;
        y = 0;
        break;
    case BOTTOMLEFT:
        fuFlags |= TPM_BOTTOMALIGN | TPM_LEFTALIGN;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
        x = 0;
        y = rc.bottom;
        break;
    case BOTTOMRIGHT:
        fuFlags |= TPM_BOTTOMALIGN | TPM_RIGHTALIGN;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
        x = rc.right;
        y = rc.bottom;
        break;
    case COORDINATE:
        x = ptp->x;
        y = ptp->y;
        break;
    }
    return TrackPopupMenuEx(hmenu, fuFlags, x, y, hwnd, lptpm);
}

LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        switch (wParam) {
        case VK_APPS:
            if (lParam & 0x80000000)
                PostMessage(g_hMainWnd, WM_VK_APPS, 0, 0);
            break;
        case VK_F10:
            if (GetKeyState(VK_SHIFT) < 0) {
                if (!(lParam & 0x80000000))
                    PostMessage(g_hMainWnd, WM_VK_APPS, 0, 0);
                return 1;
            }
        }
    }
    return CallNextHookEx(g_hKeyhook, code, wParam, lParam);
}
