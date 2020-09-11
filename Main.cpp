//---------------------------------------------------------------------------//
//
// Main.cpp
//  プラグインのメイン処理
//   Copyright (C) 2016 tapetums
//
//---------------------------------------------------------------------------//

#include <strsafe.h>

#include "Plugin.hpp"
#include "MessageDef.hpp"
#include "Utility.hpp"

#include "ml_window.hpp"
#include "ml_defs.hpp"

//---------------------------------------------------------------------------//
//
// グローバル変数
//
//---------------------------------------------------------------------------//

HINSTANCE g_hInst    { nullptr };
HWND      g_hMainWnd { nullptr };

//---------------------------------------------------------------------------//

// プラグインの名前
LPCTSTR PLUGIN_NAME   { TEXT("MenuLaunch for Win10") };
LPCTSTR WNDCLASS_NAME { TEXT("MenuLaunchWin10Class") };

// コマンドの数
DWORD COMMAND_COUNT { NUMBER_OF_COMMAND };

//---------------------------------------------------------------------------//

// コマンドの情報
PLUGIN_COMMAND_INFO g_cmd_info[] = {
    {
        TEXT("Launch"),                         // コマンド名（英名）
        TEXT("MenuLaunchを表示"),               // コマンド説明（日本語）
        CMDID_LAUNCH,                           // コマンドID
        0,                                      // Attr（未使用）
        -1,                                     // ResTd(未使用）
        dmSystemMenu,                           // DispMenu
        0,                                      // TimerInterval[msec] 0で使用しない
        0                                       // TimerCounter（未使用）
    },
    {
        TEXT("Launch On Specified"),            // コマンド名（英名）
        TEXT("指定位置にMenuLaunchを表示"),     // コマンド説明（日本語）
        CMDID_LAUNCH_ON_SPECIFIED,              // コマンドID
        0,                                      // Attr（未使用）
        -1,                                     // ResTd(未使用）
        dmHotKeyMenu,                           // DispMenu
        0,                                      // TimerInterval[msec] 0で使用しない
        0                                       // TimerCounter（未使用）
    },
    {
        TEXT("Launch On Cursor"),                       // コマンド名（英名）
        TEXT("マウスカーソルの位置にMenuLaunchを表示"), // コマンド説明（日本語）
        CMDID_LAUNCH_ON_CURSOR,                         // コマンドID
        0,                                              // Attr（未使用）
        -1,                                             // ResTd(未使用）
        dmHotKeyMenu,                                   // DispMenu
        0,                                              // TimerInterval[msec] 0で使用しない
        0                                               // TimerCounter（未使用）
    },
    {
        TEXT("Reload Setting"),                 // コマンド名（英名）
        TEXT("設定を再読み込み"),               // コマンド説明（日本語）
        CMDID_RELOAD_SETTINGS,                  // コマンドID
        0,                                      // Attr（未使用）
        -1,                                     // ResTd(未使用）
        dmHotKeyMenu,                           // DispMenu
        0,                                      // TimerInterval[msec] 0で使用しない
        0                                       // TimerCounter（未使用）
    },
};

//---------------------------------------------------------------------------//

// プラグインの情報
PLUGIN_INFO g_info = {
    0,                   // プラグインI/F要求バージョン
    (LPTSTR)PLUGIN_NAME, // プラグインの名前（任意の文字が使用可能）
    nullptr,             // プラグインのファイル名（相対パス）
    ptAlwaysLoad,        // プラグインのタイプ
    0,                   // バージョン
    0,                   // バージョン
    COMMAND_COUNT,       // コマンド個数
    &g_cmd_info[0],      // コマンド
    0,                   // ロードにかかった時間（msec）
};

//---------------------------------------------------------------------------//

// TTBEvent_Init() の内部実装
BOOL WINAPI Init() {
    g_hMainWnd = CreateMainWnd(g_hInst, WNDCLASS_NAME, PLUGIN_NAME);

    WriteLog(elInfo, TEXT("%s: %s"), PLUGIN_NAME, TEXT("Successfully initialized"));
    return TRUE;
}

//---------------------------------------------------------------------------//

// TTBEvent_Unload() の内部実装
void WINAPI Unload() {
    ::DestroyWindow(g_hMainWnd);

    WriteLog(elInfo, TEXT("%s: %s"), PLUGIN_NAME, TEXT("Successfully uninitialized"));
}

//---------------------------------------------------------------------------//

// TTBEvent_Execute() の内部実装
BOOL WINAPI Execute(INT32 CmdId, HWND) {
    SendMessage(g_hMainWnd, WM_EXECUTE_COMMAND, CmdId, 0);
    return TRUE;
}

//---------------------------------------------------------------------------//

// TTBEvent_WindowsHook() の内部実装
void WINAPI Hook(UINT, WPARAM, LPARAM) {
}

//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// CRT を使わないため new/delete を自前で実装
//---------------------------------------------------------------------------//

#if defined(_NODEFLIB)

void* __cdecl operator new(size_t size)
{
    return ::HeapAlloc(::GetProcessHeap(), 0, size);
}

void __cdecl operator delete(void* p)
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void __cdecl operator delete(void* p, size_t) // C++14
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void* __cdecl operator new[](size_t size)
{
    return ::HeapAlloc(::GetProcessHeap(), 0, size);
}

void __cdecl operator delete[](void* p)
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void __cdecl operator delete[](void* p, size_t) // C++14
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

// プログラムサイズを小さくするためにCRTを除外
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#pragma comment(linker, "/entry:DllMain")

#endif

//---------------------------------------------------------------------------//
//
// DLL エントリポイント
//
//---------------------------------------------------------------------------//

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID)
{
    if ( fdwReason == DLL_PROCESS_ATTACH ) { g_hInst = hInstance; }
    return TRUE;
}

//---------------------------------------------------------------------------//

// Main.cpp