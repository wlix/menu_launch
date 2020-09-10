#include <comutil.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <fstream>
#include <tchar.h>
#include "rapidxml_utils.hpp"
#include "ml_xml.hpp"

BOOL StrToBOOL(LPCTSTR lpstr) {
	return (StrCmpI(lpstr, TEXT("true")) == 0 || StrCmpI(lpstr, TEXT("yes")) == 0) ?
		TRUE : FALSE;
}

tstring WINAPI ml_ExpandEnvironmentStrings(const tstring& str) {
	int size = ExpandEnvironmentStrings(str.c_str(), NULL, 0);
	if (size > 0) {
		// std::vector<char> buff(size); 
		// これだと，str に 日本語が含まれていない場合は動作するが，
		// そうでない場合に 次の ExpandEnvironmentStrings で
		// GetLastError() == 234 (データがさらにあります。)
		// となる．
		// 一回目の ExpandEnvironmentStrings の戻り値がおかしいみたい？
		// 対策として，バッファを大きめに確保する

		std::vector<TCHAR> buff(size * 3);
		ExpandEnvironmentStrings(str.c_str(), &buff[0], buff.size());
		return tstring(&buff[0]);
	}
	else {
		return tstring(str);
	}
}

BOOL WINAPI AppendMenuItem(HMENU hMenu, LPCTSTR lpStr, HBITMAP hBitmap, HMENU hSubmenu, UINT wID) {
	MENUITEMINFO mii = { 0 };
	mii.cbSize = sizeof(mii);
	TCHAR szStr[MAX_PATH];

	if (hBitmap != NULL) {
		mii.fMask |= MIIM_BITMAP;
		mii.hbmpItem = hBitmap;
		lstrcpyn(szStr, TEXT(" "), MAX_PATH);
		lstrcpyn(szStr + 1, lpStr, MAX_PATH - 1);
	}
	else {
		lstrcpyn(szStr, lpStr, MAX_PATH);
	}

	if (hSubmenu != NULL) {
		mii.fMask |= MIIM_SUBMENU | MIIM_STRING | MIIM_FTYPE;
		mii.fType = MFT_STRING;
		mii.hSubMenu = hSubmenu;
		mii.dwTypeData = szStr;
	}
	else {
		mii.fMask |= MIIM_ID | MIIM_STRING | MIIM_FTYPE;
		mii.fType = MFT_STRING;
		mii.wID = wID;
		mii.dwTypeData = szStr;
	}

	InsertMenuItem(hMenu, GetMenuItemCount(hMenu), TRUE, &mii);
	return TRUE;
}

HBITMAP WINAPI IconToBitmap(HICON hIcon) {
	HDC hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
	HDC hMemDc = CreateCompatibleDC(hdc);
	int IconXsize = GetSystemMetrics(SM_CXSMICON);
	int IconYsize = GetSystemMetrics(SM_CYSMICON);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdc, IconXsize, IconYsize);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDc, hBitmap);
	DrawIconEx(hMemDc, 0, 0, hIcon, IconXsize, IconYsize, 0, GetSysColorBrush(COLOR_MENU), DI_IMAGE | DI_MASK);
	SelectObject(hMemDc, hOldBitmap);
	DeleteDC(hMemDc);
	DeleteDC(hdc);

	return hBitmap;
}

HBITMAP WINAPI GetIconBitmap(const AttributeMap& attr) {
	HICON hIcon = NULL;
	HBITMAP hBitmap = NULL;
	SHFILEINFO	sfi = { 0 };

	AttributeMap::const_iterator icon = attr.find(TEXT("icon"));
	if (icon != attr.end()) {
		AttributeMap::const_iterator index = attr.find(TEXT("index"));
		if (index != attr.end()) {
			ExtractIconEx(ml_ExpandEnvironmentStrings(icon->second).c_str(),
				StrToInt(index->second.c_str()), NULL, &hIcon, 1);
		}
		else {
			if (SHGetFileInfo(ml_ExpandEnvironmentStrings(icon->second).c_str(),
				0, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON))
				hIcon = sfi.hIcon;
		}
	}
	else {
		AttributeMap::const_iterator path = attr.find(TEXT("path"));
		if (path != attr.end()) {
			if (SHGetFileInfo(ml_ExpandEnvironmentStrings(path->second).c_str(),
				0, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON))
				hIcon = sfi.hIcon;
		}
		else {
			if (SHGetFileInfo(TEXT(""), FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
				SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES))
				hIcon = sfi.hIcon;
		}
	}

	if (hIcon != NULL) {
		hBitmap = IconToBitmap(hIcon);
		DestroyIcon(hIcon);
		return hBitmap;
	}
	return NULL;
}

HBITMAP WINAPI GetDesktopIcon() {
	HBITMAP		hBitmap;
	SHFILEINFO  sfi = { 0 };
	LPITEMIDLIST	pIDL;

	SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pIDL);

	if (SHGetFileInfo((LPCTSTR)pIDL, 0, &sfi, sizeof(sfi),
		SHGFI_PIDL | SHGFI_ICON | SHGFI_SMALLICON) == 0)
		return NULL;

	if (pIDL == NULL)
		CoTaskMemFree(pIDL);

	hBitmap = IconToBitmap(sfi.hIcon);
	DestroyIcon(sfi.hIcon);
	return hBitmap;
}

BOOL WINAPI LoadMenuItems(xml_node<TCHAR>* parent_node, HMENU hMenu, MLMenu& mlm) {
	for(xml_node<TCHAR> *node = parent_node->first_node(); node; node = node->next_sibling()) {
		AttributeMap attr_map;
		GetAttributes(node, attr_map);

		if (lstrcmpi(node->name(), TEXT("item")) == 0 ||
			lstrcmpi(node->name(), TEXT("execute")) == 0) {
			MenuItem mi = { MenuItem::EXECUTE };
			mi.execute.Path = ml_ExpandEnvironmentStrings(attr_map[TEXT("path")]);
			mi.execute.Param = ml_ExpandEnvironmentStrings(attr_map[TEXT("param")]);
			mlm.MenuItems.push_back(mi);

			HBITMAP hBitmap = mlm.bShowIcon ? GetIconBitmap(attr_map) : NULL;
			if (hBitmap != NULL)
				mlm.hBitmaps.push_back(hBitmap);
			int ID = mlm.MenuItems.size();
			AppendMenuItem(hMenu, attr_map[TEXT("title")].c_str(), hBitmap, NULL, ID);
		}
		else if (lstrcmpi(node->name(), TEXT("command")) == 0) {
			MenuItem mi = { MenuItem::PLUGINCOMMAND };
			mi.command.PluginFilename = ml_ExpandEnvironmentStrings(attr_map[TEXT("filename")]);
			mi.command.CommandID = StrToInt(attr_map[TEXT("id")].c_str());
			mlm.MenuItems.push_back(mi);

			HBITMAP hBitmap = NULL;
			if (attr_map.find(TEXT("icon")) != attr_map.end()) {
				hBitmap = mlm.bShowIcon ? GetIconBitmap(attr_map) : NULL;
			}
			else {
				SHFILEINFO	sfi = { 0 };
				if (SHGetFileInfo(TEXT("*.dll"), 0, &sfi, sizeof(SHFILEINFO),
					SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES)) {
					hBitmap = IconToBitmap(sfi.hIcon);
					DestroyIcon(sfi.hIcon);
				}
			}
			int ID = mlm.MenuItems.size();
			AppendMenuItem(hMenu, attr_map[TEXT("title")].c_str(), hBitmap, NULL, ID);
		}
		else if (lstrcmpi(node->name(), TEXT("submenu")) == 0) {
			HBITMAP hBitmap = mlm.bShowIcon ? GetIconBitmap(attr_map) : NULL;
			if (hBitmap != NULL)
				mlm.hBitmaps.push_back(hBitmap);
			HMENU hSubMenu = CreatePopupMenu();
			AppendMenuItem(hMenu, attr_map[TEXT("title")].c_str(), hBitmap, hSubMenu, 0);

			LoadMenuItems(node, hSubMenu, mlm);

			if (attr_map.find(TEXT("path")) != attr_map.end()) {
				MenuItem mi = { MenuItem::EXECUTE };
				mi.execute.Path = ml_ExpandEnvironmentStrings(attr_map[TEXT("path")]);
				mi.execute.Param = ml_ExpandEnvironmentStrings(attr_map[TEXT("param")]);
				mlm.MenuItems.push_back(mi);
				int ID = mlm.MenuItems.size();
				AppendMenu(hSubMenu, MF_STRING | MF_OWNERDRAW, ID, TEXT(" "));
				SetMenuDefaultItem(hSubMenu, ID, FALSE);
			}
		}
		else if (lstrcmpi(node->name(), TEXT("separator")) == 0) {
			AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
		}
		else if (lstrcmpi(node->name(), TEXT("folder")) == 0) {
			HBITMAP hBitmap = mlm.bShowIcon ? (attr_map.find(TEXT("path")) == attr_map.end() ?
				GetDesktopIcon() : GetIconBitmap(attr_map)) : NULL;
			if (hBitmap != NULL)
				mlm.hBitmaps.push_back(hBitmap);

			HMENU hSubMenu = CreatePopupMenu();

			FolderMenu fm = { ml_ExpandEnvironmentStrings(attr_map[TEXT("path")]),
				attr_map.find(TEXT("showicon")) == attr_map.end() ?
					mlm.bShowIcon : StrToBOOL(attr_map[TEXT("showicon")].c_str()),
				attr_map.find(TEXT("folderonly")) == attr_map.end() ?
					FALSE : StrToBOOL(attr_map[TEXT("folderonly")].c_str()) };
			mlm.FolderMenus.insert(std::make_pair(hSubMenu, fm));
			AppendMenuItem(hMenu, attr_map[TEXT("title")].c_str(), hBitmap, hSubMenu, 0);
		}
	}
	return TRUE;
}

BOOL WINAPI LoadMenu(xml_node<TCHAR>* node, MLMenu& mlm) {
	LoadMenuItems(node, mlm.hMenu, mlm);

	MENUINFO mi = { 0 };
	mi.cbSize = sizeof mi;
	mi.fMask = MIM_APPLYTOSUBMENUS | MIM_STYLE;
	mi.dwStyle = MNS_CHECKORBMP;
	SetMenuInfo(mlm.hMenu, &mi);

	return TRUE;
}

BOOL WINAPI LoadXML(LPCTSTR szFileName, MLMenu& mlm) {
	std::vector<TCHAR>         xml;
	size_t                     xml_size;
	std::basic_ifstream<TCHAR> ifs(szFileName, std::ios::binary);
	if (!ifs) {
		MessageBox(NULL, TEXT("XMLファイルが開けませんでした"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	ifs.unsetf(std::ios::skipws);
	ifs.seekg(0, std::ios::end);
	xml_size = ifs.tellg();
	ifs.seekg(0);
	xml.resize(xml_size + 1);
	ifs.read(&xml.front(), static_cast<std::streamsize>(xml_size));
	xml[xml_size] = 0;
	ifs.close();
	xml_document<TCHAR> xml_doc;
	try {
		xml_doc.parse<parse_default | parse_declaration_node>(&xml.front());
	} catch (parse_error &err) {
		LPTSTR buf;
#if UNICODE || _UNICODE
		wchar_t wch_err_what[BUFSIZ];
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, err.what(), strlen(err.what()), wch_err_what, sizeof(wch_err_what) / sizeof(wchar_t));
		wsprintf(buf, TEXT("%s %s"), wch_err_what, err.where<TCHAR>());
#else
		wsprintf(buf, TEXT("%s %s"), err.what(), err.where<TCHAR>());
#endif
		MessageBox(NULL, buf, TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	
	xml_node<TCHAR> *node = xml_doc.first_node();
	/* decla */

	node = node->next_sibling();
	if (lstrcmpi(node->name(), TEXT("menulaunch")) != 0) {
		MessageBox(NULL, TEXT("XMLファイルに<menulaunch>が見当たりません"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	node = node->first_node();
	if (lstrcmpi(node->name(), TEXT("setting")) != 0) {
		MessageBox(NULL, TEXT("XMLファイルに<setting>が見当たりません"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	LoadSettings(node, mlm);
	node = node->next_sibling();
	if (lstrcmpi(node->name(), TEXT("menu")) != 0) {
		MessageBox(NULL, TEXT("XMLファイルに<menu>が見当たりません"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	mlm.hMenu = CreatePopupMenu();
	LoadMenu(node, mlm);

	return TRUE;
}

BOOL WINAPI GetAttributes(xml_node<TCHAR> *node, AttributeMap &attr_map) {
	for (xml_attribute<TCHAR> *attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
		attr_map.insert(std::make_pair(attr->name(), attr->value()));
	}
	return TRUE;
}

BOOL WINAPI LoadSettings(xml_node<TCHAR>* node, MLMenu& mlm) {
	mlm.bShowIcon = TRUE;
	mlm.MenuPos.type = CURSOR;
	mlm.MenuPos.x = 0;
	mlm.MenuPos.y = 0;

	AttributeMap attr_map;
	GetAttributes(node, attr_map);
	mlm.bShowIcon = StrToBOOL(attr_map[TEXT("icon")].c_str());
	AttributeMap::iterator menupos = attr_map.find(TEXT("menupos"));
	if (menupos != attr_map.end()) {
		if (menupos->second == TEXT("マウスカーソル"))
			mlm.MenuPos.type = CURSOR;
		else if (menupos->second == TEXT("中央"))
			mlm.MenuPos.type = CENTER;
		else if (menupos->second == TEXT("左上"))
			mlm.MenuPos.type = TOPLEFT;
		else if (menupos->second == TEXT("右上"))
			mlm.MenuPos.type = TOPRIGHT;
		else if (menupos->second == TEXT("左下"))
			mlm.MenuPos.type = BOTTOMLEFT;
		else if (menupos->second == TEXT("右下"))
			mlm.MenuPos.type = BOTTOMRIGHT;
		else {
			int x = 0, y = 0;
			if ((_stscanf_s(menupos->second.c_str(), TEXT("%d,%d"), &x, &y)) == 2) {
				mlm.MenuPos.type = COORDINATE;
				mlm.MenuPos.x = x;
				mlm.MenuPos.y = y;
			}
		}
	}
	return TRUE;
}

