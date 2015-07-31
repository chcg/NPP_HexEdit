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


#ifndef _MODIFYMENU_H_
#define _MODIFYMENU_H_

#include "keys.h"


const UINT pSuppFileIds[] = {
    IDM_FILE_NEW,
    IDM_FILE_OPEN,
    IDM_FILE_CLOSE,
    IDM_FILE_CLOSEALL,
    IDM_FILE_CLOSEALL_BUT_CURRENT,
    IDM_FILE_SAVE,
    IDM_FILE_SAVEALL,
    IDM_FILE_SAVEAS,
    IDM_FILE_EXIT,
    IDM_FILE_LOADSESSION,
    IDM_FILE_SAVESESSION,
    IDM_FILE_RELOAD,
    IDM_FILE_SAVECOPYAS,
    IDM_FILE_DELETE,
    IDM_FILE_RENAME,
    0
};

const UINT pSuppEditIds[] = {
    IDM_EDIT_CUT,
    IDM_EDIT_COPY,
    IDM_EDIT_UNDO,
    IDM_EDIT_REDO,
    IDM_EDIT_PASTE,
    IDM_EDIT_DELETE,
    IDM_EDIT_SELECTALL,
    IDM_EDIT_SETREADONLY,
    IDM_EDIT_FULLPATHTOCLIP,
    IDM_EDIT_FILENAMETOCLIP,
    IDM_EDIT_CURRENTDIRTOCLIP,
    IDM_EDIT_CLEARREADONLY,
    IDM_OPEN_ALL_RECENT_FILE,
    IDM_CLEAN_RECENT_FILE_LIST,
    0
};

const UINT pSuppSearchIds[] = {
    IDM_SEARCH_FIND,
    IDM_SEARCH_FINDNEXT,
    IDM_SEARCH_REPLACE,
    IDM_SEARCH_GOTOLINE,
    IDM_SEARCH_TOGGLE_BOOKMARK,
    IDM_SEARCH_NEXT_BOOKMARK,
    IDM_SEARCH_PREV_BOOKMARK,
    IDM_SEARCH_CLEAR_BOOKMARKS,
    IDM_SEARCH_FINDPREV,
    IDM_SEARCH_FINDINFILES,
    IDM_SEARCH_VOLATILE_FINDNEXT,
    IDM_SEARCH_VOLATILE_FINDPREV,
    IDM_SEARCH_CUTMARKEDLINES,
    IDM_SEARCH_COPYMARKEDLINES,
    IDM_SEARCH_PASTEMARKEDLINES,
    IDM_SEARCH_DELETEMARKEDLINES,
    0
};

const UINT pSuppViewIds[] = {
    IDM_VIEW_FULLSCREENTOGGLE,
    IDM_VIEW_ALWAYSONTOP,
    IDM_VIEW_ZOOMIN,
    IDM_VIEW_ZOOMOUT,
    IDM_VIEW_ZOOMRESTORE,
    IDM_VIEW_GOTO_ANOTHER_VIEW,
    IDM_VIEW_CLONE_TO_ANOTHER_VIEW,
    IDM_VIEW_SWITCHTO_OTHER_VIEW,
    0
};


void ChangeNppMenu(HWND hWnd, BOOL toHexStyle, HWND hSci);
void StoreNppMenuInfo(HMENU hMenuItem, vector<tMenu> & vMenuInfo);
void UpdateNppMenuInfo(HMENU hMenuItem, vector<tMenu> & vMenuInfo);
UINT CreateNppMenu(HMENU & hMenuItem, vector<tMenu> & vMenuInfo, const UINT* idArray = NULL);
void DestroyNppMenuHndl(HMENU hMenuItem);
void ClearMenuStructures(void);


const int KEY_STR_LEN = 16;

struct KeyIDNAME {
	const TCHAR * name;
	UCHAR id;
};

static KeyIDNAME namedKeyArray[] = {
    {_T("None"), VK_NULL},

    {_T("Backspace"), VK_BACK},
    {_T("Tab"), VK_TAB},
    {_T("Enter"), VK_RETURN},
    {_T("Esc"), VK_ESCAPE},
    {_T("Spacebar"), VK_SPACE},

    {_T("Page up"), VK_PRIOR},
    {_T("Page down"), VK_NEXT},
    {_T("End"), VK_END},
    {_T("Home"), VK_HOME},
    {_T("Left"), VK_LEFT},
    {_T("Up"), VK_UP},
    {_T("Right"), VK_RIGHT},
    {_T("Down"), VK_DOWN},

    {_T("INS"), VK_INSERT},
    {_T("DEL"), VK_DELETE},

    {_T("0"), VK_0},
    {_T("1"), VK_1},
    {_T("2"), VK_2},
    {_T("3"), VK_3},
    {_T("4"), VK_4},
    {_T("5"), VK_5},
    {_T("6"), VK_6},
    {_T("7"), VK_7},
    {_T("8"), VK_8},
    {_T("9"), VK_9},
    {_T("A"), VK_A},
    {_T("B"), VK_B},
    {_T("C"), VK_C},
    {_T("D"), VK_D},
    {_T("E"), VK_E},
    {_T("F"), VK_F},
    {_T("G"), VK_G},
    {_T("H"), VK_H},
    {_T("I"), VK_I},
    {_T("J"), VK_J},
    {_T("K"), VK_K},
    {_T("L"), VK_L},
    {_T("M"), VK_M},
    {_T("N"), VK_N},
    {_T("O"), VK_O},
    {_T("P"), VK_P},
    {_T("Q"), VK_Q},
    {_T("R"), VK_R},
    {_T("S"), VK_S},
    {_T("T"), VK_T},
    {_T("U"), VK_U},
    {_T("V"), VK_V},
    {_T("W"), VK_W},
    {_T("X"), VK_X},
    {_T("Y"), VK_Y},
    {_T("Z"), VK_Z},

    {_T("Numpad 0"), VK_NUMPAD0},
    {_T("Numpad 1"), VK_NUMPAD1},
    {_T("Numpad 2"), VK_NUMPAD2},
    {_T("Numpad 3"), VK_NUMPAD3},
    {_T("Numpad 4"), VK_NUMPAD4},
    {_T("Numpad 5"), VK_NUMPAD5},
    {_T("Numpad 6"), VK_NUMPAD6},
    {_T("Numpad 7"), VK_NUMPAD7},
    {_T("Numpad 8"), VK_NUMPAD8},
    {_T("Numpad 9"), VK_NUMPAD9},
    {_T("Num *"), VK_MULTIPLY},
    {_T("Num +"), VK_ADD},
    //{_T("Num Enter"), VK_SEPARATOR},	//this one doesnt seem to work
    {_T("Num -"), VK_SUBTRACT},
    {_T("Num ."), VK_DECIMAL},
    {_T("Num /"), VK_DIVIDE},
    {_T("F1"), VK_F1},
    {_T("F2"), VK_F2},
    {_T("F3"), VK_F3},
    {_T("F4"), VK_F4},
    {_T("F5"), VK_F5},
    {_T("F6"), VK_F6},
    {_T("F7"), VK_F7},
    {_T("F8"), VK_F8},
    {_T("F9"), VK_F9},
    {_T("F10"), VK_F10},
    {_T("F11"), VK_F11},
    {_T("F12"), VK_F12},

    {_T("~"), VK_OEM_3},
    {_T("-"), VK_OEM_MINUS},
    {_T("="), VK_OEM_PLUS},
    {_T("["), VK_OEM_4},
    {_T("]"), VK_OEM_6},
    {_T(";"), VK_OEM_1},
    {_T("'"), VK_OEM_7},
    {_T("\\"), VK_OEM_5},
    {_T(","), VK_OEM_COMMA},
    {_T("."), VK_OEM_PERIOD},
    {_T("/"), VK_OEM_2},

    {_T("<>"), VK_OEM_102},
};

#define nrKeys sizeof(namedKeyArray)/sizeof(struct KeyIDNAME)

void GetShortCuts(HWND hWnd);
void UpdateShortCut(NotifyHeader *nmhdr);
UINT MapShortCutToMenuId(BYTE uChar);


#endif //_MODIFYMENU_H_
