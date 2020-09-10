#pragma once

#include "ml_defs.hpp"
#include "ml_item_id_list.hpp"

struct item {
	tstring name;
	CItemIDList idl;
};

void WINAPI folder_Init(HWND hWnd, int baseID);
HBITMAP WINAPI GetIconBitmapFromIDL(LPITEMIDLIST p_pIDL);
void WINAPI EnumFiles(std::vector<item>* folders, std::vector<item>* files, LPITEMIDLIST idl);
void WINAPI folder_OnInitMenuPopup(HMENU hMenu, std::map<HMENU, FolderMenu>& fms);
BOOL WINAPI PopupContextMenu(LPITEMIDLIST pidl, POINT* pt);
BOOL WINAPI folder_ContextMenu(HWND hWnd, HMENU hMenu, int pos, POINT* pt);
BOOL WINAPI folder_Execute(int ID);
void WINAPI folder_clear();
BOOL WINAPI folder_HandleMenuMsg(UINT Mess, WPARAM wp, LPARAM lp);
