//this file is part of Hex Edit Plugin for Notepad++
//Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "hex.h"
#include "ModifyMenu.h"


/* notepad menus */
BOOL			isMenuHex		= FALSE;
vector<tMenu>	vMenuInfoFile;
vector<tMenu>	vMenuInfoEdit;
vector<tMenu>	vMenuInfoSearch;
vector<tMenu>	vMenuInfoView;

tShortCut g_scList[] = {
    {TRUE, IDM_EDIT_SELECTALL, {0}},
    {TRUE, IDM_EDIT_DELETE, {0}},
    {TRUE, IDM_EDIT_COPY, {0}},
    {TRUE, IDM_EDIT_CUT, {0}},
    {TRUE, IDM_EDIT_PASTE, {0}},
    {TRUE, IDM_EDIT_REDO, {0}},
    {TRUE, IDM_EDIT_UNDO, {0}},
    {TRUE, IDM_EDIT_INS_TAB, {0}},
    {TRUE, IDM_VIEW_ZOOMIN, {0}},
    {TRUE, IDM_VIEW_ZOOMOUT, {0}},
    {TRUE, IDM_VIEW_ZOOMRESTORE, {0}}
};


void ChangeNppMenu(HWND hWnd, BOOL toHexStyle, HWND hSci)
{
    extern HWND g_HSource;
	if ((toHexStyle == isMenuHex) || (hSci != g_HSource))
		return;

	TCHAR	text[64];
    HMENU hMenu = (HMENU)::SendMessage(hWnd, NPPM_INTERNAL_GETMENU, 0, 0);

	if (hMenu == NULL)
		return;

	/* store if menu will be modified */
	isMenuHex = toHexStyle;

	if (vMenuInfoFile.size() == 0) {
		StoreNppMenuInfo(::GetSubMenu(hMenu, MENUINDEX_FILE), vMenuInfoFile);
	} else {
		UpdateNppMenuInfo(::GetSubMenu(hMenu, MENUINDEX_FILE), vMenuInfoFile);
	}
	if (vMenuInfoEdit.size() == 0) {
		StoreNppMenuInfo(::GetSubMenu(hMenu, MENUINDEX_EDIT), vMenuInfoEdit);
	} else {
		UpdateNppMenuInfo(::GetSubMenu(hMenu, MENUINDEX_EDIT), vMenuInfoEdit);
	}
	if (vMenuInfoSearch.size() == 0) {
		StoreNppMenuInfo(::GetSubMenu(hMenu, MENUINDEX_SEARCH), vMenuInfoSearch);
	} else {
		UpdateNppMenuInfo(::GetSubMenu(hMenu, MENUINDEX_SEARCH), vMenuInfoSearch);
	}
	if (vMenuInfoView.size() == 0) {
		StoreNppMenuInfo(::GetSubMenu(hMenu, MENUINDEX_VIEW), vMenuInfoView);
	} else {
		UpdateNppMenuInfo(::GetSubMenu(hMenu, MENUINDEX_VIEW), vMenuInfoView);
	}

	/* create own menu for FILE */
	if (vMenuInfoFile.size() != 0)
	{
		HMENU	hMenuTemp = ::CreatePopupMenu();

		if (isMenuHex == TRUE)
		{
			BOOL	lastSep = FALSE;
			for (size_t nPos = 0; nPos < vMenuInfoFile.size(); nPos++) {
				switch (vMenuInfoFile[nPos].uID) {
					case 0:
					{
						if (vMenuInfoFile[nPos].uFlags & MF_SEPARATOR) {
							lastSep = TRUE;
						} else {
							lastSep = FALSE;
						}
						break;
					}
					case IDM_FILE_NEW:
					case IDM_FILE_OPEN:
					case IDM_FILE_CLOSE:
					case IDM_FILE_CLOSEALL:
					case IDM_FILE_CLOSEALL_BUT_CURRENT:
					case IDM_FILE_SAVE:
					case IDM_FILE_SAVEALL:
					case IDM_FILE_SAVEAS:
					case IDM_FILE_EXIT:
					case IDM_FILE_LOADSESSION:
					case IDM_FILE_SAVESESSION:
					case IDM_FILE_RELOAD:
					case IDM_FILE_SAVECOPYAS:
					case IDM_FILE_DELETE:
					case IDM_FILE_RENAME:
					{
						if (lastSep == TRUE) {
							::AppendMenu(hMenuTemp, MF_BYPOSITION | MF_SEPARATOR, nPos, NULL);
						}
						::AppendMenu(hMenuTemp, 
							vMenuInfoFile[nPos].uFlags | MF_STRING,
							vMenuInfoFile[nPos].uID, vMenuInfoFile[nPos].szName);
						lastSep = FALSE;
						break;
					}
					default:
						break;
				}
			}
		}
		else
		{
            CreateNppMenu(hMenuTemp, vMenuInfoFile);
		}

		/* exchange menus */
		::GetMenuString(hMenu, MENUINDEX_FILE, text, 64, MF_BYPOSITION);
		::ModifyMenu(hMenu, MENUINDEX_FILE, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hMenuTemp, text);
		DestroyNppMenuHndl(::GetSubMenu(hMenu, MENUINDEX_FILE));
	}

	/* create own menu for EDIT */
	if (vMenuInfoEdit.size() != 0)
	{
		HMENU	hMenuTemp = ::CreatePopupMenu();

		if (isMenuHex == TRUE)
		{
			BOOL	lastSep = FALSE;
			for (size_t nPos = 0; nPos < vMenuInfoEdit.size(); nPos++) {
				switch (vMenuInfoEdit[nPos].uID) {
					case 0:
					{
						if (vMenuInfoEdit[nPos].uFlags & MF_SEPARATOR) {
							lastSep = TRUE;
						} else {
							lastSep = FALSE;
						}
						break;
					}
					case IDM_EDIT_CUT:
					case IDM_EDIT_COPY:
					case IDM_EDIT_UNDO:
					case IDM_EDIT_REDO:
					case IDM_EDIT_PASTE:
					case IDM_EDIT_DELETE:
					case IDM_EDIT_SELECTALL:
					case IDM_EDIT_SETREADONLY:
					case IDM_EDIT_FULLPATHTOCLIP:
					case IDM_EDIT_FILENAMETOCLIP:
					case IDM_EDIT_CURRENTDIRTOCLIP:
					case IDM_EDIT_CLEARREADONLY:
					case IDM_OPEN_ALL_RECENT_FILE:
					case IDM_CLEAN_RECENT_FILE_LIST:
					{
						if (lastSep == TRUE) {
							::AppendMenu(hMenuTemp, MF_BYPOSITION | MF_SEPARATOR, nPos, NULL);
						}
						::AppendMenu(hMenuTemp, 
							vMenuInfoEdit[nPos].uFlags | MF_STRING,
							vMenuInfoEdit[nPos].uID, vMenuInfoEdit[nPos].szName);
						lastSep = FALSE;
						break;
					}
					default:
						break;
				}
			}
		}
		else
		{
            CreateNppMenu(hMenuTemp, vMenuInfoEdit);
		}

		/* exchange menus */
		::GetMenuString(hMenu, MENUINDEX_EDIT, text, 64, MF_BYPOSITION);
		::ModifyMenu(hMenu, MENUINDEX_EDIT, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hMenuTemp, text);
		DestroyNppMenuHndl(::GetSubMenu(hMenu, MENUINDEX_EDIT));
	}

	/* create own menu for SEARCH */
	if (vMenuInfoSearch.size() != 0)
	{
		HMENU	hMenuTemp = ::CreatePopupMenu();

		if (isMenuHex == TRUE)
		{
			BOOL	lastSep = FALSE;
			for (size_t nPos = 0; nPos < vMenuInfoSearch.size(); nPos++) {
				switch (vMenuInfoSearch[nPos].uID) {
					case 0:
					{
						if (vMenuInfoSearch[nPos].uFlags & MF_SEPARATOR) {
							lastSep = TRUE;
						} else {
							lastSep = FALSE;
						}
						break;
					}
					case IDM_SEARCH_FIND:
					case IDM_SEARCH_FINDNEXT:
					case IDM_SEARCH_REPLACE:
					case IDM_SEARCH_GOTOLINE:
					case IDM_SEARCH_TOGGLE_BOOKMARK:
					case IDM_SEARCH_NEXT_BOOKMARK:
					case IDM_SEARCH_PREV_BOOKMARK:
					case IDM_SEARCH_CLEAR_BOOKMARKS:
					case IDM_SEARCH_FINDPREV:
					case IDM_SEARCH_FINDINFILES:
					case IDM_SEARCH_VOLATILE_FINDNEXT:
					case IDM_SEARCH_VOLATILE_FINDPREV:
					case IDM_SEARCH_CUTMARKEDLINES:
					case IDM_SEARCH_COPYMARKEDLINES:
					case IDM_SEARCH_PASTEMARKEDLINES:
					case IDM_SEARCH_DELETEMARKEDLINES:
					{
						if (lastSep == TRUE) {
							::AppendMenu(hMenuTemp, MF_BYPOSITION | MF_SEPARATOR, nPos, NULL);
						}
						::AppendMenu(hMenuTemp, 
							vMenuInfoSearch[nPos].uFlags | MF_STRING,
							vMenuInfoSearch[nPos].uID, vMenuInfoSearch[nPos].szName);
						lastSep = FALSE;
						break;
					}
					default:
						break;
				}
			}
		}
		else
		{
            CreateNppMenu(hMenuTemp, vMenuInfoSearch);
        }

		/* exchange menus */
		::GetMenuString(hMenu, MENUINDEX_SEARCH, text, 64, MF_BYPOSITION);
		::ModifyMenu(hMenu, MENUINDEX_SEARCH, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hMenuTemp, text);
		DestroyNppMenuHndl(::GetSubMenu(hMenu, MENUINDEX_SEARCH));
	}

	/* create own menu for VIEW */
	if (vMenuInfoView.size() != 0)
	{
		HMENU	hMenuTemp = ::CreatePopupMenu();

		if (isMenuHex == TRUE)
		{
            CreateNppMenu(hMenuTemp, vMenuInfoView, pSuppViewIds);
		}
		else
		{
            CreateNppMenu(hMenuTemp, vMenuInfoView);
		}

		/* exchange menus */
		::GetMenuString(hMenu, MENUINDEX_VIEW, text, 64, MF_BYPOSITION);
		::ModifyMenu(hMenu, MENUINDEX_VIEW, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hMenuTemp, text);
		DestroyNppMenuHndl(::GetSubMenu(hMenu, MENUINDEX_VIEW));
	}

	/* activate/deactive menu entries */
	::EnableMenuItem(hMenu, MENUINDEX_FORMAT, MF_BYPOSITION | (toHexStyle?MF_GRAYED:MF_ENABLED));
	::EnableMenuItem(hMenu, MENUINDEX_LANGUAGE, MF_BYPOSITION | (toHexStyle?MF_GRAYED:MF_ENABLED));
	::EnableMenuItem(hMenu, MENUINDEX_MACRO, MF_BYPOSITION | (toHexStyle?MF_GRAYED:MF_ENABLED));
	::EnableMenuItem(hMenu, MENUINDEX_RUN, MF_BYPOSITION | (toHexStyle?MF_GRAYED:MF_ENABLED));
	::DrawMenuBar(hWnd);
}

void StoreNppMenuInfo(HMENU hMenuItem, vector<tMenu> & vMenuInfo)
{
	tMenu	menuItem;
	UINT	elemCnt	= ::GetMenuItemCount(hMenuItem);

	vMenuInfo.clear();

	for (size_t nPos = 0; nPos < elemCnt; nPos++)
	{
		menuItem.uID = ::GetMenuItemID(hMenuItem, (int)nPos);
		menuItem.uFlags	= ::GetMenuState(hMenuItem, (UINT)nPos, MF_BYPOSITION);

		::GetMenuString(hMenuItem, (UINT)nPos, menuItem.szName, sizeof(menuItem.szName) / sizeof(TCHAR), MF_BYPOSITION);
		if (menuItem.uFlags & MF_POPUP)
		{
			HMENU	hSubMenu = ::GetSubMenu(hMenuItem, (int)nPos);
			if (hSubMenu != NULL) {
				StoreNppMenuInfo(hSubMenu, menuItem.vSubMenu);
			}
		}
		vMenuInfo.push_back(menuItem);
	}
}

void UpdateNppMenuInfo(HMENU hMenuItem, vector<tMenu> & vMenuInfo)
{
	UINT	nPos	= 0;
	UINT	elemCnt = ::GetMenuItemCount(hMenuItem);

	for (size_t i = 0; i < vMenuInfo.size() && ((vMenuInfo[i].uFlags != 0) || (vMenuInfo[i].uID != 0)); i++)
	{
		if (vMenuInfo[i].uID == ::GetMenuItemID(hMenuItem, nPos)) {
			vMenuInfo[i].uFlags = ::GetMenuState(hMenuItem, nPos, MF_BYPOSITION);
			nPos++;
		} else if (::GetMenuState(hMenuItem, nPos, MF_BYPOSITION) & MF_SEPARATOR) {
			nPos++;
		}
	}
}


UINT CreateNppMenu(HMENU & hMenuItem, vector<tMenu> & vMenuInfo, const UINT* idArray)
{
    UINT    cnt     = 0;
	BOOL	lastSep = FALSE;

	for (size_t nPos = 0; nPos < vMenuInfo.size(); nPos++)
    {
        if (vMenuInfo[nPos].uFlags & MF_POPUP)
        {
            HMENU   hSubMenu = ::CreatePopupMenu();
            if (CreateNppMenu(hSubMenu, vMenuInfo[nPos].vSubMenu, idArray) == 0) {
                ::DestroyMenu(hSubMenu);
            } else {
                /* add a seperator in case there is one to add */
		        if (lastSep == TRUE) {
			        ::AppendMenu(hMenuItem, MF_BYPOSITION | MF_SEPARATOR, nPos, NULL);
               		lastSep = FALSE;
                    cnt++;
		        }
	            ::AppendMenu(hMenuItem, MF_POPUP | MF_STRING, (UINT_PTR)hSubMenu, vMenuInfo[nPos].szName);
            }
        }
        else if (vMenuInfo[nPos].uFlags & MF_SEPARATOR)
        {
			lastSep = TRUE;
        }
        else
        {
            UINT    uID  = 0;

            if (idArray != NULL)
            {
                /* search for supported menu id */
                for (UINT i = 0; idArray[i] != 0; i++) {
                    if (idArray[i] == vMenuInfo[nPos].uID) {
                        uID = vMenuInfo[nPos].uID;
                        break;
                    }
                }
            }
            else
            {
                uID = vMenuInfo[nPos].uID;
            }

            if (uID != 0)
            {
                /* add a seperator in case there is one to add */
		        if (lastSep == TRUE) {
			        ::AppendMenu(hMenuItem, MF_BYPOSITION | MF_SEPARATOR, nPos, NULL);
    			    lastSep = FALSE;
		        }

	            ::AppendMenu(hMenuItem, 
			        vMenuInfo[nPos].uFlags | (vMenuInfo[nPos].uFlags & MF_SEPARATOR ? 0 : MF_STRING),
			        uID, vMenuInfo[nPos].szName);
                cnt++;
            }
        }
	}

    return cnt;
}


void DestroyNppMenuHndl(HMENU hMenuItem)
{
	UINT	elemCnt = ::GetMenuItemCount(hMenuItem);

	for (UINT nPos = 0; nPos < elemCnt; nPos++)
	{
        if (::GetMenuState(hMenuItem, nPos, MF_BYPOSITION) & MF_POPUP)
        {
            DestroyNppMenuHndl(::GetSubMenu(hMenuItem, nPos));
		}
	}
    ::DestroyMenu(hMenuItem);
}


void ClearMenuStructures(void)
{
	vMenuInfoFile.clear();
	vMenuInfoEdit.clear();
	vMenuInfoSearch.clear();
	vMenuInfoView.clear();
}

void GetShortCuts(HWND hWnd)
{
    TCHAR   text[64];
    LPTSTR  pSc     = NULL;
    LPTSTR  pScKey  = NULL;
    UINT    max     = sizeof(g_scList) / sizeof(tShortCut);
    HMENU   hMenu   = (HMENU)::SendMessage(hWnd, NPPM_INTERNAL_GETMENU, 0, 0);
 
    if (hMenu == NULL)
        return;

    for (UINT i = 0; i < max; i++)
    {
        g_scList[i].isEnable = FALSE;
        if (::GetMenuString(hMenu, g_scList[i].uID, text, 64, MF_BYCOMMAND) != 0)
        {
            pSc = &(_tcsstr(text, _T("\t")))[1];
            if (pSc != NULL) {
                g_scList[i].isEnable = TRUE;
                pScKey = _tcsstr(pSc, _T("Ctrl+"));
                if (pScKey != NULL) {
                    g_scList[i].scKey._isCtrl = TRUE;
                    pSc = pScKey + _tcslen(_T("Ctrl+"));
                }
                pScKey = _tcsstr(pSc, _T("Alt+"));
                if (pScKey != NULL) {
                    g_scList[i].scKey._isAlt = TRUE;
                    pSc = pScKey + _tcslen(_T("Alt+"));
                }
                pScKey = _tcsstr(pSc, _T("Shift+"));
                if (pScKey != NULL) {
                    g_scList[i].scKey._isShift = TRUE;
                    pSc = pScKey + _tcslen(_T("Shift+"));
                }
                for (UINT j = 0; j < nrKeys; j++) {
                    if (_tcscmp(pSc, namedKeyArray[j].name) == NULL) {
                        g_scList[i].scKey._key = namedKeyArray[j].id;
                        break;
                    }
                }
            }
        }
    }
}

void UpdateShortCut(NotifyHeader *nmhdr)
{
    UINT    max = sizeof(g_scList) / sizeof(tShortCut);

    for (UINT i = 0; i < max; i++)
    {
        if (g_scList[i].uID == nmhdr->idFrom)
        {
            if (((ShortcutKey*)nmhdr->hwndFrom)->_key == 0) {
                g_scList[i].isEnable = FALSE;
            } else {
                g_scList[i].scKey = *((ShortcutKey*)nmhdr->hwndFrom);
                g_scList[i].isEnable = TRUE;
            }
            break;
        }
    }
}

UINT MapShortCutToMenuId(BYTE uChar)
{
    UINT    max = sizeof(g_scList) / sizeof(tShortCut);

    for (UINT i = 0; i < max; i++)
    {
        if (g_scList[i].scKey._key == uChar)
        {
            if (g_scList[i].isEnable == TRUE)
            {
                if ((g_scList[i].scKey._isAlt   == TRUE) != ((bool)(0x80 & ::GetKeyState(VK_MENU))))    return 0;
                if ((g_scList[i].scKey._isCtrl  == TRUE) != ((bool)(0x80 & ::GetKeyState(VK_CONTROL)))) return 0;
                if ((g_scList[i].scKey._isShift == TRUE) != ((bool)(0x80 & ::GetKeyState(VK_SHIFT))))   return 0;
                return g_scList[i].uID;
            }
            return 0;
        }
    }
    return 0;
}

