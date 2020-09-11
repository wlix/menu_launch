#include <comutil.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <fstream>
#include <codecvt>
#include "ml_xml.hpp"

XML_TEXT_ENCODING g_encoding = XML_TEXT_ENCODING::SHIFT_JIS;

std::wstring multi_to_wide(std::string const& src) {
	auto const dest_size = ::MultiByteToWideChar(CP_ACP, 0U, src.data(), -1, nullptr, 0U);
	std::vector<wchar_t> dest(dest_size, L'\0');
	if (::MultiByteToWideChar(CP_ACP, 0U, src.data(), -1, dest.data(), dest.size()) == 0) {
		throw std::system_error{ static_cast<int>(::GetLastError()), std::system_category() };
	}
	dest.resize(std::char_traits<wchar_t>::length(dest.data()));
	dest.shrink_to_fit();
	return std::wstring(dest.begin(), dest.end());
}

std::wstring utf8_to_wide(std::string const& src) {
	auto const dest_size = ::MultiByteToWideChar(CP_UTF8, 0U, src.data(), -1, nullptr, 0U);
	std::vector<wchar_t> dest(dest_size, L'\0');
	if (::MultiByteToWideChar(CP_UTF8, 0U, src.data(), -1, dest.data(), dest.size()) == 0) {
		throw std::system_error{ static_cast<int>(::GetLastError()), std::system_category() };
	}
	dest.resize(std::char_traits<wchar_t>::length(dest.data()));
	dest.shrink_to_fit();
	return std::wstring(dest.begin(), dest.end());
}

std::string utf16_to_utf8(std::u16string const& src) {
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
    return converter.to_bytes(src);
}

BOOL StrToBOOL(LPCTSTR lpstr) {
	return (StrCmpI(lpstr, TEXT("true")) == 0 || StrCmpI(lpstr, TEXT("yes")) == 0) ?
		TRUE : FALSE;
}

tstring WINAPI ml_ExpandEnvironmentStrings(const std::wstring& str) {
	int size = ExpandEnvironmentStrings(str.c_str(), NULL, 0);
	if (size > 0) {
		// std::vector<char> buff(size); 
		// これだと，str に 日本語が含まれていない場合は動作するが，
		// そうでない場合に 次の ExpandEnvironmentStrings で
		// GetLastError() == 234 (データがさらにあります。)
		// となる．
		// 一回目の ExpandEnvironmentStrings の戻り値がおかしいみたい？
		// 対策として，バッファを大きめに確保する

		std::vector<wchar_t> buff((INT32)size * 3);
		ExpandEnvironmentStrings(str.c_str(), &buff[0], buff.size());
		return std::wstring(&buff[0]);
	}
	else {
		return std::wstring(str);
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

	AttributeMap::const_iterator icon = attr.find("icon");
	if (icon != attr.end()) {
		AttributeMap::const_iterator index = attr.find("index");
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
		AttributeMap::const_iterator path = attr.find("path");
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

BOOL WINAPI LoadMenuItems(xml_node<>* parent_node, HMENU hMenu, MLMenu& mlm) {
	for(xml_node<> *node = parent_node->first_node(); node; node = node->next_sibling()) {
		AttributeMap attr_map;
		GetAttributes(node, attr_map);

		if (_strcmpi(node->name(), "item") == 0 ||
			_strcmpi(node->name(), "execute") == 0) {
			MenuItem mi = { MenuItem::EXECUTE };
			mi.execute.Path = ml_ExpandEnvironmentStrings(attr_map["path"]);
			mi.execute.Param = ml_ExpandEnvironmentStrings(attr_map["param"]);
			mlm.MenuItems.push_back(mi);

			HBITMAP hBitmap = mlm.bShowIcon ? GetIconBitmap(attr_map) : NULL;
			if (hBitmap != NULL)
				mlm.hBitmaps.push_back(hBitmap);
			int ID = mlm.MenuItems.size();
			AppendMenuItem(hMenu, attr_map["title"].c_str(), hBitmap, NULL, ID);
		}
		else if (_strcmpi(node->name(), "command") == 0) {
			MenuItem mi = { MenuItem::PLUGINCOMMAND };
			mi.command.PluginFilename = ml_ExpandEnvironmentStrings(attr_map["filename"]);
			mi.command.CommandID = StrToInt(attr_map["id"].c_str());
			mlm.MenuItems.push_back(mi);

			HBITMAP hBitmap = NULL;
			if (attr_map.find("icon") != attr_map.end()) {
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
			AppendMenuItem(hMenu, attr_map["title"].c_str(), hBitmap, NULL, ID);
		}
		else if (_strcmpi(node->name(), "submenu") == 0) {
			HBITMAP hBitmap = mlm.bShowIcon ? GetIconBitmap(attr_map) : NULL;
			if (hBitmap != NULL)
				mlm.hBitmaps.push_back(hBitmap);
			HMENU hSubMenu = CreatePopupMenu();
			AppendMenuItem(hMenu, attr_map["title"].c_str(), hBitmap, hSubMenu, 0);

			LoadMenuItems(node, hSubMenu, mlm);

			if (attr_map.find("path") != attr_map.end()) {
				MenuItem mi = { MenuItem::EXECUTE };
				mi.execute.Path = ml_ExpandEnvironmentStrings(attr_map["path"]);
				mi.execute.Param = ml_ExpandEnvironmentStrings(attr_map["param"]);
				mlm.MenuItems.push_back(mi);
				int ID = mlm.MenuItems.size();
				AppendMenu(hSubMenu, MF_STRING | MF_OWNERDRAW, ID, TEXT(" "));
				SetMenuDefaultItem(hSubMenu, ID, FALSE);
			}
		}
		else if (_strcmpi(node->name(), "separator") == 0) {
			AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
		}
		else if (_strcmpi(node->name(), "folder") == 0) {
			HBITMAP hBitmap = mlm.bShowIcon ? (attr_map.find("path") == attr_map.end() ?
				GetDesktopIcon() : GetIconBitmap(attr_map)) : NULL;
			if (hBitmap != NULL)
				mlm.hBitmaps.push_back(hBitmap);

			HMENU hSubMenu = CreatePopupMenu();

			FolderMenu fm = { ml_ExpandEnvironmentStrings(attr_map["path"]),
				attr_map.find("showicon") == attr_map.end() ?
					mlm.bShowIcon : StrToBOOL(attr_map["showicon"].c_str()),
				attr_map.find("folderonly") == attr_map.end() ?
					FALSE : StrToBOOL(attr_map["folderonly"].c_str()) };
			mlm.FolderMenus.insert(std::make_pair(hSubMenu, fm));
			AppendMenuItem(hMenu, attr_map["title"].c_str(), hBitmap, hSubMenu, 0);
		}
	}
	return TRUE;
}

BOOL WINAPI LoadMenu(xml_node<>* node, MLMenu& mlm) {
	LoadMenuItems(node, mlm.hMenu, mlm);

	MENUINFO mi = { 0 };
	mi.cbSize = sizeof mi;
	mi.fMask = MIM_APPLYTOSUBMENUS | MIM_STYLE;
	mi.dwStyle = MNS_CHECKORBMP;
	SetMenuInfo(mlm.hMenu, &mi);

	return TRUE;
}

BOOL WINAPI LoadXML(LPCTSTR szFileName, MLMenu& mlm) {
	std::vector<char> xml;
	size_t xml_size;
	std::ifstream ifs(szFileName, std::ios::binary);
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

	xml_document<> xml_doc;
	try {
		xml_doc.parse<parse_declaration_node>(&xml.front());
	} catch (parse_error &err) {
		LPTSTR buf = nullptr;
#if UNICODE || _UNICODE
		std::wstring wch_err_what = multi_to_wide(std::string(err.what()));
		wsprintf(buf, TEXT("%s %s"), wch_err_what.c_str(), err.where<TCHAR>());
#else
		wsprintf(buf, TEXT("%s %s"), err.what(), err.where<TCHAR>());
#endif
		MessageBox(NULL, buf, TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	
	xml_node<> *node = xml_doc.first_node();
	/* declaration */
	if (_strcmpi(node->first_attribute("encoding")->value(), "Shift_JIS") == 0) {
		g_encoding = XML_TEXT_ENCODING::SHIFT_JIS;
	} else if (_strcmpi(node->first_attribute("encoding")->value(), "UTF-8") == 0
		|| _strcmpi(node->first_attribute("encoding")->value(), "UTF8") == 0) {
		g_encoding = XML_TEXT_ENCODING::UTF_8;
	} else if (_strcmpi(node->first_attribute("encoding")->value(), "UTF-16") == 0
		|| _strcmpi(node->first_attribute("encoding")->value(), "UTF16") == 0) {
		g_encoding = XML_TEXT_ENCODING::UTF_16;
	}
	TCHAR buff[512];
	wsprintf(buff, TEXT("%d"), g_encoding);
	MessageBox(NULL, buff, TEXT("g_encoding"), MB_OK);

	node = node->next_sibling();
	if (_strcmpi(node->name(), "menulaunch") != 0) {
		MessageBox(NULL, TEXT("XMLファイルに<menulaunch>が見当たりません"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	node = node->first_node();
	if (_strcmpi(node->name(), "setting") != 0) {
		MessageBox(NULL, TEXT("XMLファイルに<setting>が見当たりません"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	LoadSettings(node, mlm);
	node = node->next_sibling();
	if (_strcmpi(node->name(), "menu") != 0) {
		MessageBox(NULL, TEXT("XMLファイルに<menu>が見当たりません"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	mlm.hMenu = CreatePopupMenu();
	LoadMenu(node, mlm);

	return TRUE;
}

BOOL WINAPI GetAttributes(xml_node<> *node, AttributeMap &attr_map) {
	for (xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
		std::wstring buf;
		if (g_encoding == XML_TEXT_ENCODING::SHIFT_JIS) {
			buf = multi_to_wide(attr->value());
		} else if (g_encoding == XML_TEXT_ENCODING::UTF_8) {
			buf = utf8_to_wide(attr->value());
		} else if (g_encoding == XML_TEXT_ENCODING::UTF_16) {
			buf = utf8_to_wide(utf16_to_utf8((char16_t *)attr->value()));
		}
		attr_map.insert(std::make_pair(attr->name(), buf));
	}
	return TRUE;
}

BOOL WINAPI LoadSettings(xml_node<>* node, MLMenu& mlm) {
	mlm.bShowIcon = TRUE;
	mlm.MenuPos.type = CURSOR;
	mlm.MenuPos.x = 0;
	mlm.MenuPos.y = 0;

	AttributeMap attr_map;
	GetAttributes(node, attr_map);
	mlm.bShowIcon = StrToBOOL(attr_map["icon"].c_str());
	AttributeMap::iterator menupos = attr_map.find("menupos");
	if (menupos != attr_map.end()) {
		if (menupos->second == L"マウスカーソル")
			mlm.MenuPos.type = CURSOR;
		else if (menupos->second == L"中央")
			mlm.MenuPos.type = CENTER;
		else if (menupos->second == L"左上")
			mlm.MenuPos.type = TOPLEFT;
		else if (menupos->second == L"右上")
			mlm.MenuPos.type = TOPRIGHT;
		else if (menupos->second == L"左下")
			mlm.MenuPos.type = BOTTOMLEFT;
		else if (menupos->second == L"右下")
			mlm.MenuPos.type = BOTTOMRIGHT;
		else {
			int x = 0, y = 0;
			if ((swscanf_s(menupos->second.c_str(), L"%d,%d", &x, &y)) == 2) {
				mlm.MenuPos.type = COORDINATE;
				mlm.MenuPos.x = x;
				mlm.MenuPos.y = y;
			}
		}
	}
	return TRUE;
}

