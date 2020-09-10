#include <algorithm>
#include <shlobj.h>
#include <shobjidl.h>
#include "ml_xml.hpp"
#include "ml_folder.hpp"

#pragma comment(lib, "comctl32.lib")

int g_baseID = 0;
HMENU g_hRootMenu = NULL;
BOOL g_bIcon = FALSE;
BOOL g_bFolderOnly = FALSE;
HWND g_hWnd = NULL;
IContextMenu2* g_pcm2;

std::map<HMENU, CItemIDList> g_folders;
std::vector<HMENU> g_enumeratedmenu;
std::vector<CItemIDList> g_idls;
std::vector<HBITMAP> g_icons;

void WINAPI folder_Init(HWND hWnd, int baseID) {
	g_hWnd = hWnd;
	g_baseID = baseID;
}

HBITMAP WINAPI GetIconBitmapFromIDL(LPITEMIDLIST p_pIDL) {
	SHFILEINFO sfi = { 0 };
	HIMAGELIST hImage = (HIMAGELIST)SHGetFileInfo((LPCTSTR)p_pIDL, 0, &sfi, sizeof(sfi),
		SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	if (hImage == NULL)
		return NULL;

	int IconXsize = GetSystemMetrics(SM_CXSMICON);
	int IconYsize = GetSystemMetrics(SM_CYSMICON);
	HDC hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
	HDC hMemDc = CreateCompatibleDC(hdc);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, IconXsize, IconYsize);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDc, hBitmap);
	RECT rc = { 0, 0, IconXsize, IconYsize };
	FillRect(hMemDc, &rc, GetSysColorBrush(COLOR_MENU));
	ImageList_Draw(hImage, sfi.iIcon, hMemDc, 0, 0, ILD_TRANSPARENT);
	SelectObject(hMemDc, hOldBitmap);
	DeleteDC(hMemDc);
	DeleteDC(hdc);
	return hBitmap;
}

void WINAPI EnumFiles(std::vector<item>* folders, std::vector<item>* files, LPITEMIDLIST idl)
{
	IShellFolder* desktop = NULL, * current = NULL;
	IEnumIDList* enumidlist = NULL;
	std::vector<CItemIDList> folderidl, fileidl;
	if (FAILED(SHGetDesktopFolder(&desktop)))
		return;

	if (FAILED(desktop->BindToObject(idl, NULL, IID_IShellFolder, (LPVOID*)&current)))
	{
		current = desktop;
		desktop->AddRef();
	}

	if (FAILED(current->EnumObjects(/*g_hWnd*/ NULL, SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN | SHCONTF_FOLDERS,
		&enumidlist)))
	{
		current->Release(); desktop->Release();
		return;
	}

	while (true)
	{
		CItemIDList newidl;
		if (enumidlist->Next(1, &newidl.m_pidl, NULL) != S_OK)
			break;

		SFGAOF attr = SFGAO_FOLDER | SFGAO_STREAM;
		current->GetAttributesOf(1, newidl, &attr);
		if (attr & SFGAO_FOLDER)
			folderidl.push_back(newidl);
		else if (files != NULL)
			fileidl.push_back(newidl);

	}

	class compare {
	public:
		LPSHELLFOLDER shfolder;
		compare(const LPSHELLFOLDER s) : shfolder(s) {}
		bool operator()(const LPITEMIDLIST pi1, const LPITEMIDLIST pi2) {
			return (short)HRESULT_CODE(shfolder->CompareIDs(MAKELPARAM(0, SHCIDS_CANONICALONLY), pi1, pi2)) < 0;
		}
	};
	std::sort(folderidl.begin(), folderidl.end(), compare(current));
	std::sort(fileidl.begin(), fileidl.end(), compare(current));

	TCHAR buff[MAX_PATH];
	for (std::vector<CItemIDList>::iterator itr = folderidl.begin();
		itr != folderidl.end(); itr++)
	{
		CItemIDList::GetDispName(current, *itr, SHGDN_NORMAL, buff);
		item it = { buff, CItemIDList::ConcatPidls(idl, *itr) };
		folders->push_back(it);
	}
	for (std::vector<CItemIDList>::iterator itr = fileidl.begin();
		itr != fileidl.end(); itr++)
	{
		CItemIDList::GetDispName(current, *itr, SHGDN_NORMAL, buff);
		item it = { buff, CItemIDList::ConcatPidls(idl, *itr) };
		files->push_back(it);
	}

	enumidlist->Release();
	current->Release();
	desktop->Release();
}

void WINAPI folder_OnInitMenuPopup(HMENU hMenu, std::map<HMENU, FolderMenu>& fms) {
	std::map<HMENU, FolderMenu>::iterator it = fms.find(hMenu);
	if (it != fms.end())
	{
		folder_clear();
		try {
			CItemIDList idl;
			if (it->second.Path != TEXT(""))
			{
				idl = it->second.Path.c_str();
			}
			g_hRootMenu = hMenu;
			g_bIcon = it->second.bShowIcon;
			g_bFolderOnly = it->second.bFolderOnly;
			g_folders.insert(std::make_pair(hMenu, idl));
		}
		catch (...) {
			g_hRootMenu = hMenu;
			tstring s(TEXT("ƒtƒHƒ‹ƒ_‚ªŒ©‚Â‚©‚è‚Ü‚¹‚ñ : \""));
			s += it->second.Path + TEXT("\"");
			AppendMenu(hMenu, MF_GRAYED | MF_DISABLED, 0, s.c_str());
			return;
		}
	}

	std::vector<HMENU>::iterator f1 = std::find(g_enumeratedmenu.begin(), g_enumeratedmenu.end(), hMenu);
	if (f1 == g_enumeratedmenu.end())
	{
		std::map<HMENU, CItemIDList>::iterator f2 = g_folders.find(hMenu);
		if (f2 != g_folders.end())
		{
			std::vector<item> folders, files;

			EnumFiles(&folders, g_bFolderOnly ? NULL : &files, f2->second);

			TCHAR buff[MAX_PATH];
			f2->second.GetDisplayName(buff);

			g_idls.push_back(f2->second);
			AppendMenu(hMenu, MF_STRING, g_baseID + g_idls.size() - 1, buff);
			AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			SetMenuDefaultItem(hMenu, 0, TRUE);

			for (int i = 0; i < folders.size(); i++)
			{
				HMENU hSubmenu = CreatePopupMenu();
				MENUINFO mi = { 0 };
				mi.cbSize = sizeof mi;
				mi.fMask = MIM_STYLE;
				mi.dwStyle = MNS_CHECKORBMP;
				SetMenuInfo(hSubmenu, &mi);

				g_folders.insert(std::make_pair(hSubmenu, folders[i].idl));
				HBITMAP hBitmap = g_bIcon ? GetIconBitmapFromIDL(folders[i].idl) : NULL;
				if (hBitmap != NULL)
					g_icons.push_back(hBitmap);
				AppendMenuItem(hMenu, folders[i].name.c_str(), hBitmap, hSubmenu, 0);
			}

			for (int i = 0; i < files.size(); i++)
			{
				HBITMAP hBitmap = g_bIcon ? GetIconBitmapFromIDL(files[i].idl) : NULL;
				if (hBitmap != NULL)
					g_icons.push_back(hBitmap);
				g_idls.push_back(files[i].idl);
				AppendMenuItem(hMenu, files[i].name.c_str(), hBitmap, 0, g_baseID + g_idls.size() - 1);
			}
			g_enumeratedmenu.push_back(hMenu);
		}
	}
}

BOOL WINAPI PopupContextMenu(LPITEMIDLIST pidl, POINT* pt) {
	CItemIDList idl2;
	IShellFolder* shellfolder = NULL;
	CItemIDList::Split((LPSHELLFOLDER&)shellfolder, idl2.m_pidl, pidl);

	IContextMenu* cmenu = NULL;

	if (FAILED(shellfolder->GetUIObjectOf(g_hWnd, 1, (LPCITEMIDLIST*)idl2, IID_IContextMenu,
		0, (void**)&cmenu))) {
		shellfolder->Release();
		return FALSE;
	}

	if (FAILED(cmenu->QueryInterface(IID_IContextMenu2, (void**)&g_pcm2))) {
		shellfolder->Release(); cmenu->Release();
		return FALSE;
	}

	cmenu->Release();
	HMENU hMenu = CreatePopupMenu();
	if (FAILED(g_pcm2->QueryContextMenu(hMenu, 0, 1, INT_MAX, CMF_NORMAL))) {
		shellfolder->Release(); g_pcm2->Release(); g_pcm2 = NULL;
		return FALSE;
	}
	int r = TrackPopupMenu(hMenu, TPM_RECURSE | TPM_RETURNCMD, pt->x, pt->y, 0, g_hWnd, NULL);
	if (r > 0) {
		CMINVOKECOMMANDINFO ici = { 0 };
		ici.cbSize = sizeof(CMINVOKECOMMANDINFO);
		ici.lpVerb = MAKEINTRESOURCEA(r - 1);
		ici.nShow = SW_SHOWNORMAL;
		ici.hwnd = g_hWnd;
		g_pcm2->InvokeCommand(&ici);
	}
	DestroyMenu(hMenu);

	shellfolder->Release();
	g_pcm2->Release();
	g_pcm2 = NULL;
	return TRUE;
}

BOOL WINAPI folder_ContextMenu(HWND hWnd, HMENU hMenu, int pos, POINT* pt) {
	std::map<HMENU, CItemIDList>::iterator f = g_folders.find(hMenu);
	if (f != g_folders.end()) {
		MENUITEMINFO mii = { 0 };
		mii.cbSize = sizeof mii;
		mii.fMask = MIIM_ID | MIIM_SUBMENU;
		GetMenuItemInfo(hMenu, pos, TRUE, &mii);
		if (mii.hSubMenu != NULL) {
			std::map<HMENU, CItemIDList>::iterator f = g_folders.find(mii.hSubMenu);
			if (f != g_folders.end()) {
				PopupContextMenu(g_folders[mii.hSubMenu], pt);
			}
		}
		else if (mii.wID > 0) {
			PopupContextMenu(g_idls[mii.wID - g_baseID], pt);
		}
		return TRUE;
	}
	return FALSE;
}

BOOL WINAPI folder_Execute(int ID) {
	if (!(g_baseID <= ID && ID < g_baseID + g_idls.size())) {
		return FALSE;
	}

	TCHAR szDirectory[MAX_PATH];
	SHELLEXECUTEINFO	stExeInfo = { 0 };
	stExeInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	stExeInfo.fMask = SEE_MASK_IDLIST;
	stExeInfo.nShow = SW_SHOWNORMAL;
	stExeInfo.lpIDList = (LPVOID)(LPITEMIDLIST)g_idls[ID - g_baseID];
	stExeInfo.hwnd = g_hWnd;

	if (SHGetPathFromIDList(g_idls[ID - g_baseID], szDirectory)) {
		if (PathRemoveFileSpec(szDirectory)) {
			stExeInfo.lpDirectory = szDirectory;
		}
		else {
			stExeInfo.lpDirectory = TEXT("");
		}
	}
	return ShellExecuteEx(&stExeInfo);
}

void WINAPI folder_clear() {
	int c = GetMenuItemCount(g_hRootMenu);
	for (int i = 0; i < c; i++) {
		DeleteMenu(g_hRootMenu, 0, MF_BYPOSITION);
	}
	g_hRootMenu = NULL;
	g_folders.clear();
	g_enumeratedmenu.clear();
	g_idls.clear();
	std::for_each(g_icons.begin(), g_icons.end(), DeleteObject);
	g_icons.clear();
}

BOOL WINAPI folder_HandleMenuMsg(UINT Mess, WPARAM wp, LPARAM lp) {
	if (g_pcm2 != NULL) {
		g_pcm2->HandleMenuMsg(Mess, wp, lp);
		return TRUE;
	}
	return FALSE;
}
