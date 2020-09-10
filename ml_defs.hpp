#pragma once

//---------------------------------------------------------------------------//
//
// Main.hpp
//  TTB Plugin Template (C++11)
//
//---------------------------------------------------------------------------//

#include <string>
#include <vector>
#include <map>
#include <windows.h>

#define IDM_POPUPMENU_BYCOMMAND 10000
#define IDM_POPUPMENU_BYMOUSE   10001

#define WM_EXECUTE_COMMAND  (WM_APP + 1)
#define WM_VK_APPS          (WM_APP + 2)

// コマンドID
enum CMD : INT32 {
    CMDID_LAUNCH = 0,
    CMDID_LAUNCH_ON_SPECIFIED,
    CMDID_LAUNCH_ON_CURSOR,
    CMDID_RELOAD_SETTINGS,
    NUMBER_OF_COMMAND
};

// ポップアップメニューの位置の設定
enum POPUP_POSISION {
    CURSOR = 0,
    CENTER,
    TOPLEFT,
    TOPRIGHT,
    BOTTOMLEFT,
    BOTTOMRIGHT,
    COORDINATE,
    NUMBER_OF_POPUP_POSISION
};

typedef std::basic_string<TCHAR> tstring;

typedef struct _TPMPOS {
    int type, x, y;
} TPMPOS;

// メニューの項目の情報
typedef struct _MenuItem {
    enum Type {
        EXECUTE,
        PLUGINCOMMAND
    } type;
    // type == EXECUTE の場合
    struct Execute {
        tstring Path;
        tstring Param;
    } execute;
    // type == PLUGINCOMMAND の場合
    struct Command {
        tstring PluginFilename;
        int CommandID;
    } command;
} MenuItem;

// フォルダを展開していくメニューの情報
typedef struct _FolderMenu {
    tstring Path;
    BOOL bShowIcon;
    BOOL bFolderOnly;
} FolderMenu;

// MenuLaunch のメニューの情報
typedef struct _MLMenu {
    // メニューハンドル
    HMENU hMenu;
    // メニューの表示位置
    TPMPOS  MenuPos;
    // アイコンを表示するか
    BOOL bShowIcon;
    // メニュー項目のIDと情報
    std::vector<MenuItem> MenuItems;
    // フォルダを展開していくメニューの情報
    std::map<HMENU, FolderMenu> FolderMenus;
    // アイコンのビットマップ
    std::vector<HBITMAP> hBitmaps;
} MLMenu;

//---------------------------------------------------------------------------//

// Main.hpp