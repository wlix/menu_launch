#include <WebServices.h>
#include <comutil.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <fstream>
#include "ml_xml.hpp"

#pragma comment(lib, "WebServices.lib")

BOOL StrToBOOL(LPCTSTR lpstr) {
	return (StrCmpI(lpstr, TEXT("true")) == 0 || StrCmpI(lpstr, TEXT("yes")) == 0) ?
		TRUE : FALSE;
}

tstring WINAPI xml_ExpandEnvironmentStrings(const tstring& str) {
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
			ExtractIconEx(xml_ExpandEnvironmentStrings(icon->second).c_str(),
				StrToInt(index->second.c_str()), NULL, &hIcon, 1);
		}
		else {
			if (SHGetFileInfo(xml_ExpandEnvironmentStrings(icon->second).c_str(),
				0, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON))
				hIcon = sfi.hIcon;
		}
	}
	else {
		AttributeMap::const_iterator path = attr.find(TEXT("path"));
		if (path != attr.end()) {
			if (SHGetFileInfo(xml_ExpandEnvironmentStrings(path->second).c_str(),
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

BOOL WINAPI LoadMenuItems(IXMLDOMNodePtr node, HMENU hMenu, MLMenu& mlm) {
	IXMLDOMNodeListPtr children{ nullptr };
	node->get_childNodes(&children);
	if (children == NULL)
		return TRUE;
	while (true) {
		IXMLDOMNodePtr child;
		children->nextNode(&child);
		if (child == NULL)
			break;
		DOMNodeType type;
		child->get_nodeType(&type);
		if (type == NODE_ELEMENT) {
			_bstr_t name;
			child->get_nodeName(name.GetAddress());
			AttributeMap attrmap;
			GetAttributes(child, attrmap);

			if (lstrcmp((LPTSTR)name, TEXT("item")) == 0 ||
				lstrcmp((LPTSTR)name, TEXT("execute")) == 0) {
				MenuItem mi = { MenuItem::EXECUTE };
				mi.execute.Path = xml_ExpandEnvironmentStrings(attrmap[TEXT("path")]);
				mi.execute.Param = xml_ExpandEnvironmentStrings(attrmap[TEXT("param")]);
				mlm.MenuItems.push_back(mi);

				HBITMAP hBitmap = mlm.bShowIcon ? GetIconBitmap(attrmap) : NULL;
				if (hBitmap != NULL)
					mlm.hBitmaps.push_back(hBitmap);
				int ID = mlm.MenuItems.size();
				AppendMenuItem(hMenu, attrmap[TEXT("title")].c_str(), hBitmap, NULL, ID);
			}
			else if (lstrcmp((LPTSTR)name, TEXT("command")) == 0) {
				MenuItem mi = { MenuItem::PLUGINCOMMAND };
				mi.command.PluginFilename = xml_ExpandEnvironmentStrings(attrmap[TEXT("filename")]);
				mi.command.CommandID = StrToInt(attrmap[TEXT("id")].c_str());
				mlm.MenuItems.push_back(mi);

				HBITMAP hBitmap = NULL;
				if (attrmap.find(TEXT("icon")) != attrmap.end()) {
					hBitmap = mlm.bShowIcon ? GetIconBitmap(attrmap) : NULL;
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
				AppendMenuItem(hMenu, attrmap[TEXT("title")].c_str(), hBitmap, NULL, ID);
			}
			else if (lstrcmp((LPTSTR)name, TEXT("submenu")) == 0) {
				HBITMAP hBitmap = mlm.bShowIcon ? GetIconBitmap(attrmap) : NULL;
				if (hBitmap != NULL)
					mlm.hBitmaps.push_back(hBitmap);
				HMENU hSubMenu = CreatePopupMenu();
				AppendMenuItem(hMenu, attrmap[TEXT("title")].c_str(), hBitmap, hSubMenu, 0);

				LoadMenuItems(child, hSubMenu, mlm);

				if (attrmap.find(TEXT("path")) != attrmap.end()) {
					MenuItem mi = { MenuItem::EXECUTE };
					mi.execute.Path = xml_ExpandEnvironmentStrings(attrmap[TEXT("path")]);
					mi.execute.Param = xml_ExpandEnvironmentStrings(attrmap[TEXT("param")]);
					mlm.MenuItems.push_back(mi);
					int ID = mlm.MenuItems.size();
					AppendMenu(hSubMenu, MF_STRING | MF_OWNERDRAW, ID, TEXT(" "));
					SetMenuDefaultItem(hSubMenu, ID, FALSE);
				}
			}
			else if (lstrcmp((LPTSTR)name, TEXT("separator")) == 0) {
				AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
			}
			else if (lstrcmp((LPTSTR)name, TEXT("folder")) == 0) {
				HBITMAP hBitmap = mlm.bShowIcon ? (attrmap.find(TEXT("path")) == attrmap.end() ?
					GetDesktopIcon() : GetIconBitmap(attrmap)) : NULL;
				if (hBitmap != NULL)
					mlm.hBitmaps.push_back(hBitmap);

				HMENU hSubMenu = CreatePopupMenu();

				FolderMenu fm = { xml_ExpandEnvironmentStrings(attrmap[TEXT("path")]),
					attrmap.find(TEXT("showicon")) == attrmap.end() ?
						mlm.bShowIcon : StrToBOOL(attrmap[TEXT("showicon")].c_str()),
					attrmap.find(TEXT("folderonly")) == attrmap.end() ?
						FALSE : StrToBOOL(attrmap[TEXT("folderonly")].c_str()) };
				mlm.FolderMenus.insert(std::make_pair(hSubMenu, fm));
				AppendMenuItem(hMenu, attrmap[TEXT("title")].c_str(), hBitmap, hSubMenu, 0);
			}
			child->Release();
		}
	}
	children->Release();
	return TRUE;
}

BOOL WINAPI xml_LoadMenu(IXMLDOMNodePtr node, MLMenu& mlm) {
	LoadMenuItems(node, mlm.hMenu, mlm);

	MENUINFO mi = { 0 };
	mi.cbSize = sizeof mi;
	mi.fMask = MIM_APPLYTOSUBMENUS | MIM_STYLE;
	mi.dwStyle = MNS_CHECKORBMP;
	SetMenuInfo(mlm.hMenu, &mi);

	return TRUE;
}

BOOL WINAPI LoadXML(LPCTSTR szFileName, MLMenu& mlm) {
	WS_XML_STRING               xml;
	WS_CHARSET					xml_charset;
	WS_ERROR*                   xml_error;
	WS_XML_READER*              xml_reader;
	WS_XML_READER_BUFFER_INPUT  xml_buf_input;
	WS_XML_READER_TEXT_ENCODING xml_txt_encoding;

	ZeroMemory(&xml, sizeof(xml));
	std::basic_ifstream<BYTE> ifs(szFileName, std::ios::binary);
	if (!ifs) {
		MessageBox(NULL, TEXT("XMLファイルが開けませんでした"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	ifs.unsetf(std::ios::skipws);
	ifs.seekg(0, std::ios::end);
	xml.length = ifs.tellg();
	ifs.seekg(0);
	ifs.read(xml.bytes, xml.length);
	ifs.close();

	if (FAILED(WsCreateError(NULL, 0, &xml_error))) {
		MessageBox(NULL, TEXT("WebServicesErrorの作成に失敗しました"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	if (FAILED(WsCreateReader(NULL, 0, &xml_reader, xml_error))) {
		MessageBox(NULL, TEXT("XML Readerの作成に失敗しました"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	ZeroMemory(&xml_buf_input, sizeof(xml_buf_input));
	xml_buf_input.input.inputType = WS_XML_READER_INPUT_TYPE_BUFFER;
	xml_buf_input.encodedData = xml.bytes;
	xml_buf_input.encodedDataSize = xml.length;

	ZeroMemory(&xml_txt_encoding, sizeof(xml_txt_encoding));
	xml_txt_encoding.encoding.encodingType = WS_XML_READER_ENCODING_TYPE_TEXT;
	xml_txt_encoding.charSet = WS_CHARSET_AUTO;

	if (FAILED(WsSetInput(xml_reader, &xml_txt_encoding.encoding, &xml_buf_input.input, NULL, 0, xml_error))) {
		MessageBox(NULL, TEXT("XMLのロード先のバッファの作成に失敗しました"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	if (FAILED(WsGetReaderProperty(xml_reader, WS_XML_READER_PROPERTY_CHARSET, &xml_charset, sizeof(xml_charset), xml_error))) {
		MessageBox(NULL, TEXT("XMLの文字コード取得に失敗しました"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	int depth = 0;
	for (;;) {
		const WS_XML_NODE *node;
		if (FAILED(WsGetReaderNode(xml_reader, &node, xml_error))) {
			MessageBox(NULL, TEXT("XML要素の取得に失敗しました"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
			return FALSE;
		}
		switch (node->nodeType) {
		case WS_XML_NODE_TYPE_ELEMENT:
			const WS_XML_ELEMENT_NODE *element_node = (const WS_XML_ELEMENT_NODE *)element_node;
			switch (depth) {
			case 0:
				Match(element_node->prefix, TEXT("menulaunch"));
			}
			depth++;
			break;
		case WS_XML_NODE_TYPE_END_ELEMENT:
			depth--;
			break;
		}
	}
	else {
		LoadSettings(xmldom, mlm);
		IXMLDOMNodePtr menunode = NULL;
		if (xmldom->selectSingleNode(_bstr_t("/menulaunch/menu"), &menunode) == S_OK) {
			mlm.hMenu = CreatePopupMenu();
			xml_LoadMenu(menunode, mlm);
		}
		else {
			MessageBox(NULL, TEXT("menuエレメントがありません"), TEXT("設定ファイルの読み込みエラー"), MB_OK | MB_ICONERROR);
			bRes = FALSE;
		}

		if (menunode != NULL) {
			menunode->Release();
		}
	}

	return TRUE;
}

BOOL WINAPI Match(const WS_XML_STRING *xml_str, LPCTSTR tchar) {
	LPTSTR xml_tchar = nullptr;
	int    xml_tchar_num = 0;
#ifdef UNICODE || _UNICODE
	if ((xml_tchar_num = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (char*)xml_str->bytes, xml_str->length, NULL, 0)) == 0) {
		if (lstrlen(tchar) == 0) { return TRUE; }
		else { return FALSE; }
	}
	xml_tchar = (LPTSTR)HeapAlloc(GetProcessHeap(), 0, xml_tchar_num * sizeof(TCHAR));

	if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (char*)xml_str->bytes, xml_str->length, xml_tchar, xml_tchar_num) != xml_tchar_num) {
		if (xml_tchar != nullptr) { HeapFree(GetProcessHeap(), 0, xml_tchar); }
		return FALSE;
	}
#else 

#endif
}

BOOL WINAPI GetAttributes(IXMLDOMNodePtr node, AttributeMap& attr) {
	IXMLDOMNamedNodeMapPtr attrmap{ nullptr };
	node->get_attributes(&attrmap);
	if (attrmap != NULL) {
		while (true) {
			IXMLDOMNodePtr attrnode = NULL;
			attrmap->nextNode(&attrnode);
			if (attrnode == NULL)
				break;
			_bstr_t name;
			_variant_t value;
			attrnode->get_nodeName(name.GetAddress());
			attrnode->get_nodeValue(value.GetAddress());
			attr.insert(std::make_pair(tstring(name), _bstr_t(value.bstrVal)));
			attrnode->Release();
		}
		attrmap->Release();
	}
	return TRUE;
}

BOOL WINAPI LoadSettings(IXMLDOMNodePtr node, MLMenu& mlm) {
	IXMLDOMNodePtr settingnode{ nullptr };
	mlm.bShowIcon = TRUE;
	mlm.MenuPos.type = CURSOR;
	mlm.MenuPos.x = 0;
	mlm.MenuPos.y = 0;

	if (node->selectSingleNode(_bstr_t(TEXT("/menulaunch/setting")), &settingnode) == S_OK) {
		AttributeMap attr;
		GetAttributes(settingnode, attr);
		mlm.bShowIcon = StrToBOOL(attr[TEXT("icon")].c_str());
		AttributeMap::iterator menupos = attr.find(TEXT("menupos"));
		if (menupos != attr.end()) {
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
		settingnode->Release();
	}
	return TRUE;
}

