//this file is part of Hex Edit Plugin for Notepad++
//Copyright (C)2006-2008 Jens Lorenz <jens.plugin.npp@gmx.de>
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


/* include files */
#include "stdafx.h"
#include "Hex.h"
#include "HexDialog.h"
#include "CompareDialog.h"
#include <OptionDialog.h>
#include "FindReplaceDialog.h"
#include "GotoDialog.h"
#include "PatternDialog.h"
#include "HelpDialog.h"
#include "ToolTip.h"
#include "tables.h"
#include "SysMsg.h"
#include <stdlib.h>

#include <shlwapi.h>
#include <shlobj.h>
#include <assert.h>

/* menu entry count */
const
INT				nbFunc	= 9;

/* for subclassing */
WNDPROC	wndProcNotepad = NULL;

/* informations for notepad */
const char  PLUGIN_NAME[] = "HEX-Editor";

/* current used file */
TCHAR			currentPath[MAX_PATH];
TCHAR			configPath[MAX_PATH];
TCHAR			iniFilePath[MAX_PATH];
UINT			currentSC	= MAIN_VIEW;
INT				openDoc1	= -1;
INT				openDoc2	= -1;

/* global values */
NppData			nppData;
HANDLE			g_hModule;
HWND			g_HSource;
HWND			g_hFindRepDlg;
FuncItem		funcItem[nbFunc];
toolbarIcons	g_TBHex;


/* create classes */
HexEdit			hexEdit1;
HexEdit			hexEdit2;
FindReplaceDlg	findRepDlg;
OptionDlg		propDlg;
GotoDlg			gotoDlg;
PatternDlg		patDlg;
HelpDlg			helpDlg;
tClipboard		g_clipboard;

/* main properties for lists */
tProp			prop;
char			hexMask[256][3];

/* handle of current used edit */
HexEdit*		pCurHexEdit = NULL;

/* get system information */
BOOL	isNotepadCreated	= FALSE;

/* notepad menus */
BOOL			isMenuHex	= FALSE;
HMENU			hMenuFile	= NULL;
HMENU			hMenuEdit	= NULL;
HMENU			hMenuSearch	= NULL;
HMENU			hMenuView	= NULL;


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
	g_hModule = hModule;

    switch (reasonForCall)
    {
		case DLL_PROCESS_ATTACH:
		{
			/* Set function pointers */
			funcItem[0]._pFunc = toggleHexEdit;
			funcItem[1]._pFunc = compareHex;
			funcItem[2]._pFunc = clearCompare;
			funcItem[3]._pFunc = NULL;
			funcItem[4]._pFunc = insertColumnsDlg;
			funcItem[5]._pFunc = replacePatternDlg;
			funcItem[6]._pFunc = NULL;
			funcItem[7]._pFunc = openPropDlg;
			funcItem[8]._pFunc = openHelpDlg;
			
			/* Fill menu names */
			strcpy(funcItem[0]._itemName, "View in &HEX");
			strcpy(funcItem[1]._itemName, "&Compare HEX");
			strcpy(funcItem[2]._itemName, "Clear Compare &Result");
			strcpy(funcItem[4]._itemName, "&Insert Columns...");
			strcpy(funcItem[5]._itemName, "&Pattern Replace...");
			strcpy(funcItem[7]._itemName, "&Options...");
			strcpy(funcItem[8]._itemName, "&Help...");

			/* Set shortcuts */
			funcItem[0]._pShKey = new ShortcutKey;
			funcItem[0]._pShKey->_isAlt		= true;
			funcItem[0]._pShKey->_isCtrl	= true;
			funcItem[0]._pShKey->_isShift	= true;
			funcItem[0]._pShKey->_key		= 0x48;
			funcItem[1]._pShKey				= NULL;
			funcItem[2]._pShKey				= NULL;
			funcItem[3]._pShKey				= NULL;
			funcItem[4]._pShKey				= NULL;
			funcItem[5]._pShKey				= NULL;
			funcItem[6]._pShKey				= NULL;
			funcItem[7]._pShKey				= NULL;
			funcItem[8]._pShKey				= NULL;

			g_hFindRepDlg     = NULL;
			memset(&g_clipboard, 0, sizeof(tClipboard));			break;
		}	
		case DLL_PROCESS_DETACH:
		{
			/* save settings */
			saveSettings();

			hexEdit1.destroy();
			hexEdit2.destroy();
			propDlg.destroy();
			delete funcItem[0]._pShKey;

			if (g_TBHex.hToolbarBmp)
				::DeleteObject(g_TBHex.hToolbarBmp);
			if (g_TBHex.hToolbarIcon)
				::DestroyIcon(g_TBHex.hToolbarIcon);

			/* Remove subclaasing */
			SetWindowLong(nppData._nppHandle, GWL_WNDPROC, (LONG)wndProcNotepad);
			break;
		}
		case DLL_THREAD_ATTACH:
			break;
			
		case DLL_THREAD_DETACH:
			break;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	/* stores notepad data */
	nppData = notpadPlusData;

	/* load data */
	loadSettings();

	/* initial dialogs */
	hexEdit1.init((HINSTANCE)g_hModule, nppData, iniFilePath);
	hexEdit2.init((HINSTANCE)g_hModule, nppData, iniFilePath);
	hexEdit1.SetParentNppHandle(nppData._scintillaMainHandle, MAIN_VIEW);
	hexEdit2.SetParentNppHandle(nppData._scintillaSecondHandle, SUB_VIEW);
	findRepDlg.init((HINSTANCE)g_hModule, nppData);
	propDlg.init((HINSTANCE)g_hModule, nppData);
	gotoDlg.init((HINSTANCE)g_hModule, nppData, iniFilePath);
	patDlg.init((HINSTANCE)g_hModule, nppData);
	helpDlg.init((HINSTANCE)g_hModule, nppData);

	/* Subclassing for Notepad */
	wndProcNotepad = (WNDPROC)SetWindowLong(nppData._nppHandle, GWL_WNDPROC, (LPARAM)SubWndProcNotepad);

	pCurHexEdit = &hexEdit1;
	setHexMask();
}

extern "C" __declspec(dllexport) const char * getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(INT *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

/***
 *	beNotification()
 *
 *	This function is called, if a notification in Scantilla/Notepad++ occurs
 */
extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	/* test for friday */
	if (((notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle) ||
		 (notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle)))
	{
		SystemUpdate();

		switch (notifyCode->nmhdr.code)
		{
			case SCN_MODIFIED:
				if (notifyCode->modificationType & SC_MOD_INSERTTEXT ||
					notifyCode->modificationType & SC_MOD_DELETETEXT) 
				{
					const tHexProp*	p_prop1	= hexEdit1.GetHexProp();
					const tHexProp*	p_prop2	= hexEdit2.GetHexProp();
					INT				length	= notifyCode->length;

					if ((p_prop1->pszFileName != NULL) && (p_prop2->pszFileName != NULL) &&
						(strcmp(p_prop1->pszFileName, p_prop2->pszFileName) == 0))
					{
						/* test for line lengths */
						hexEdit1.TestLineLength();
						hexEdit2.TestLineLength();

						/* redo undo the code */
						if (notifyCode->modificationType & SC_PERFORMED_UNDO ||
							notifyCode->modificationType & SC_PERFORMED_REDO)
						{
							hexEdit1.RedoUndo(notifyCode->position, notifyCode->length, notifyCode->modificationType);
							hexEdit2.RedoUndo(notifyCode->position, notifyCode->length, notifyCode->modificationType);
						}
					}
					else
					{
						/* test for line length */
						pCurHexEdit->TestLineLength();

						/* redo undo the code */
						if (notifyCode->modificationType & SC_PERFORMED_UNDO ||
							notifyCode->modificationType & SC_PERFORMED_REDO)
						{
							pCurHexEdit->RedoUndo(notifyCode->position, notifyCode->length, notifyCode->modificationType);
						}
					}
				}
				break;
			default:
				break;
		}
	}
	if (notifyCode->nmhdr.hwndFrom == nppData._nppHandle)
	{
		switch (notifyCode->nmhdr.code)
		{
			case NPPN_TBMODIFICATION:
			{
				/* change menu language */
				NLChangeNppMenu((HINSTANCE)g_hModule, nppData._nppHandle, PLUGIN_NAME, funcItem, nbFunc);

				g_TBHex.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_TB_HEX), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
				::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[0]._cmdID, (LPARAM)&g_TBHex);
				break;
			}
			case NPPN_READY:
			{
				isNotepadCreated = TRUE;
				break;
			}
			case NPPN_FILEOPENED:
			case NPPN_FILECLOSED:
			{
				SystemUpdate();
				pCurHexEdit->doDialog();
				break;
			}
			default:
				break;
		}
	}
}

/***
 *	messageProc()
 *
 *	This function is called, if a notification from Notepad occurs
 */
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_CREATE)
	{
		setMenu();
	}
	return TRUE;
}

/***
 *	loadSettings()
 *
 *	Load the parameters for plugin
 */
void loadSettings(void)
{
	/* initialize the config directory */
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);

	/* Test if config path exist, if not create */
	if (::PathFileExists(configPath) == FALSE)
	{
		vector<string>    vPaths;
		do {
			vPaths.push_back(configPath);
			::PathRemoveFileSpec(configPath);
		} while (::PathFileExists(configPath) == FALSE);

		for (INT i = vPaths.size()-1; i >= 0; i--)
		{
			strcpy(configPath, vPaths[i].c_str());
			::CreateDirectory(configPath, NULL);
		}
	}

	strcpy(iniFilePath, configPath);
	strcat(iniFilePath, HEXEDIT_INI);
	if (PathFileExists(iniFilePath) == FALSE)
	{
		::CloseHandle(::CreateFile(iniFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
	}

	prop.hexProp.addWidth		= ::GetPrivateProfileInt(dlgEditor, addWidth, 8, iniFilePath);
	prop.hexProp.columns		= ::GetPrivateProfileInt(dlgEditor, columns, 16, iniFilePath);
	prop.hexProp.bits			= ::GetPrivateProfileInt(dlgEditor, bits, HEX_BYTE, iniFilePath);
	prop.hexProp.isBin			= ::GetPrivateProfileInt(dlgEditor, bin, FALSE, iniFilePath);
	prop.hexProp.isLittle		= ::GetPrivateProfileInt(dlgEditor, little, FALSE, iniFilePath);
	::GetPrivateProfileString(dlgEditor, extensions, "", prop.szExtensions, 256, iniFilePath);
	prop.colorProp.rgbRegTxt	= ::GetPrivateProfileInt(dlgEditor, rgbRegTxt, RGB(0x00,0x00,0x00), iniFilePath);
	prop.colorProp.rgbRegBk		= ::GetPrivateProfileInt(dlgEditor, rgbRegBk, RGB(0xFF,0xFF,0xFF), iniFilePath);
	prop.colorProp.rgbSelTxt	= ::GetPrivateProfileInt(dlgEditor, rgbSelTxt, RGB(0xFF,0xFF,0xFF), iniFilePath);
	prop.colorProp.rgbSelBk		= ::GetPrivateProfileInt(dlgEditor, rgbSelBk, RGB(0x88,0x88,0xff), iniFilePath);
	prop.colorProp.rgbDiffTxt	= ::GetPrivateProfileInt(dlgEditor, rgbDiffTxt, RGB(0xFF,0xFF,0xFF), iniFilePath);
	prop.colorProp.rgbDiffBk	= ::GetPrivateProfileInt(dlgEditor, rgbDiffBk, RGB(0xff,0x88,0x88), iniFilePath);
	prop.colorProp.rgbBkMk		= ::GetPrivateProfileInt(dlgEditor, rgbBkMk, RGB(0xFF,0x00,0x00), iniFilePath);
	::GetPrivateProfileString(dlgEditor, fontname, "Courier New", prop.fontProp.szFontName, 256, iniFilePath);
	prop.fontProp.iFontSizeElem	= ::GetPrivateProfileInt(dlgEditor, fontsize, FONTSIZE_DEFAULT, iniFilePath);
	prop.fontProp.isCapital		= ::GetPrivateProfileInt(dlgEditor, capital, FALSE, iniFilePath);
	prop.fontProp.isBold		= ::GetPrivateProfileInt(dlgEditor, bold, FALSE, iniFilePath);
	prop.fontProp.isItalic		= ::GetPrivateProfileInt(dlgEditor, italic, FALSE, iniFilePath);
	prop.fontProp.isUnderline	= ::GetPrivateProfileInt(dlgEditor, underline, FALSE, iniFilePath);
}

/***
 *	saveSettings()
 *
 *	Saves the parameters for plugin
 */
void saveSettings(void)
{
	char	temp[64];

	::WritePrivateProfileString(dlgEditor, addWidth, itoa(prop.hexProp.addWidth, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, columns, itoa(prop.hexProp.columns, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, bits, itoa(prop.hexProp.bits, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, bin, itoa(prop.hexProp.isBin, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, little, itoa(prop.hexProp.isLittle, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, extensions, prop.szExtensions, iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbRegTxt, itoa(prop.colorProp.rgbRegTxt, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbRegBk, itoa(prop.colorProp.rgbRegBk, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbSelTxt, itoa(prop.colorProp.rgbSelTxt, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbSelBk, itoa(prop.colorProp.rgbSelBk, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbDiffTxt, itoa(prop.colorProp.rgbDiffTxt, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbDiffBk, itoa(prop.colorProp.rgbDiffBk, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbBkMk, itoa(prop.colorProp.rgbBkMk, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, fontname, prop.fontProp.szFontName, iniFilePath);
	::WritePrivateProfileString(dlgEditor, fontsize, itoa(prop.fontProp.iFontSizeElem, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, capital, itoa(prop.fontProp.isCapital, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, bold, itoa(prop.fontProp.isBold, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, italic, itoa(prop.fontProp.isItalic, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, underline, itoa(prop.fontProp.isUnderline, temp, 10), iniFilePath);
}

/***
 *	setHexMask()
 *
 *	Set correct hexMask table
 */
void setHexMask(void)
{
	memcpy(&hexMask, &hexMaskNorm, 256 * 3);

	if (prop.fontProp.isCapital == TRUE)
	{
		memcpy(&hexMask, &hexMaskCap, 256 * 3);
	}
}

/***
 *	setMenu()
 *
 *	Initialize the menu
 */
void setMenu(void)
{
	HMENU			hMenu	= ::GetMenu(nppData._nppHandle);
	const tHexProp*	p_prop	= pCurHexEdit->GetHexProp();

	::EnableMenuItem(hMenu, funcItem[4]._cmdID, MF_BYCOMMAND | (p_prop->isVisible?0:MF_GRAYED));
	::EnableMenuItem(hMenu, funcItem[5]._cmdID, MF_BYCOMMAND | (p_prop->isVisible?0:MF_GRAYED));

	const tHexProp* p_prop1	= hexEdit1.GetHexProp();
	const tHexProp*	p_prop2	= hexEdit2.GetHexProp();
	if ((p_prop1->isVisible == TRUE) && (p_prop2->isVisible == TRUE) &&
		(stricmp(p_prop1->pszFileName, p_prop2->pszFileName) != 0)) {
		::EnableMenuItem(hMenu, funcItem[1]._cmdID, MF_BYCOMMAND);
	} else {
		::EnableMenuItem(hMenu, funcItem[1]._cmdID, MF_BYCOMMAND | MF_GRAYED);
	}

	if (p_prop->pCompareData != NULL) {
		::EnableMenuItem(hMenu, funcItem[2]._cmdID, MF_BYCOMMAND);
	} else {
		::EnableMenuItem(hMenu, funcItem[2]._cmdID, MF_BYCOMMAND | MF_GRAYED);
	}
}

/***
 *	checkMenu()
 *
 *	Check menu item of HEX
 */
void checkMenu(BOOL state)
{
	::SendMessage(nppData._nppHandle, NPPM_SETMENUITEMCHECK, (WPARAM)funcItem[0]._cmdID, (LPARAM)state);
}

/***
  *	API to get the current properties
 */
tHexProp getProp(void)
{
	return prop.hexProp;
}

BOOL getCLM(void)
{
	return prop.fontProp.isCapital;
}

LPCSTR getFontName(void)
{
	return prop.fontProp.szFontName;
}

UINT getFontSize(void)
{
	return g_iFontSize[prop.fontProp.iFontSizeElem];
}

UINT getFontSizeElem(void)
{
	return prop.fontProp.iFontSizeElem;
}

void setFontSizeElem(UINT iElem)
{
	prop.fontProp.iFontSizeElem = iElem;
}

BOOL isFontBold(void)
{
	return prop.fontProp.isBold;
}

BOOL isFontItalic(void)
{
	return prop.fontProp.isItalic;
}

BOOL isFontUnderline(void)
{
	return prop.fontProp.isUnderline;
}

COLORREF getColor(eColorType type)
{
	switch (type)
	{
		case HEX_COLOR_REG_TXT:
			return prop.colorProp.rgbRegTxt;
		case HEX_COLOR_REG_BK:
			return prop.colorProp.rgbRegBk;
		case HEX_COLOR_SEL_TXT:
			return prop.colorProp.rgbSelTxt;
		case HEX_COLOR_SEL_BK:
			return prop.colorProp.rgbSelBk;
		case HEX_COLOR_DIFF_TXT:
			return prop.colorProp.rgbDiffTxt;
		case HEX_COLOR_DIFF_BK:
			return prop.colorProp.rgbDiffBk;
		case HEX_COLOR_BKMK:
			return prop.colorProp.rgbBkMk;
	}
}

/***
 *	ScintillaMsg()
 *
 *	API-Wrapper
 */
UINT ScintillaMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
	return (UINT)::SendMessage(g_HSource, message, wParam, lParam);
}
UINT ScintillaMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	g_HSource = hWnd;
	return (UINT)::SendMessage(hWnd, message, wParam, lParam);
}
void CleanScintillaBuf(HWND hWnd)
{
	do {
		ScintillaMsg(hWnd, SCI_UNDO);
	} while (ScintillaMsg(hWnd, SCI_CANUNDO));
}

/***
 *	ScintillaGetText()
 *
 *	API-Wrapper
 */
void ScintillaGetText(char *text, INT start, INT end) 
{
	TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText  = text;
	ScintillaMsg(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}
void ScintillaGetText(HWND hWnd, char *text, INT start, INT end)
{
	g_HSource = hWnd;
	ScintillaGetText(text, start, end);
}


/***
 *	UpdateCurrentHScintilla()
 *
 *	update the global values (gets the current handle and current used window)
 */
void UpdateCurrentHScintilla(void)
{
	UINT		newSC		= MAIN_VIEW;

	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&newSC);
	g_HSource = (newSC == MAIN_VIEW)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
	currentSC = newSC;
}


/***
 *	getCurrentHScintilla()
 *
 *	Get the handle of the current scintilla
 */
HWND getCurrentHScintilla(void)
{
	UpdateCurrentHScintilla();
	return g_HSource;
}


/**************************************************************************
 *	Interface functions
 */

void toggleHexEdit(void)
{
	pCurHexEdit->doDialog(TRUE);
	DialogUpdate();
	setMenu();
}

void compareHex(void)
{
	DoCompare();
	DialogUpdate();
	setMenu();
}

void clearCompare(void)
{
	if (pCurHexEdit->GetHexProp()->pCompareData != NULL) {
		pCurHexEdit->SetCompareResult(NULL);
		setMenu();
	}
}

void insertColumnsDlg(void)
{
	patDlg.insertColumns(pCurHexEdit->getHSelf());
}

void replacePatternDlg(void)
{
	patDlg.patternReplace(pCurHexEdit->getHSelf());
}

void openPropDlg(void)
{
	if (propDlg.doDialog(&prop) == IDOK)
	{
		hexEdit1.SetFont();
		hexEdit2.SetFont();
		setHexMask();
		SystemUpdate();
	}
}

void openHelpDlg(void)
{
	helpDlg.doDialog();
}


/**************************************************************************
 *	SubWndProcNotepad
 */
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT			ret = 0;

	switch (message)
	{
		case WM_ACTIVATE:
		{
			ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
			if (currentSC == MAIN_VIEW)
			{
				::SendMessage(hexEdit1.getHSelf(), message, wParam, lParam);
				::SendMessage(hexEdit2.getHSelf(), message, ~wParam, lParam);
			}
			else
			{
				::SendMessage(hexEdit1.getHSelf(), message, ~wParam, lParam);
				::SendMessage(hexEdit2.getHSelf(), message, wParam, lParam);
			}
			break;
		}
		case WM_COMMAND:
		{
			/* necessary for focus change between main and second SCI handle */
			if (HIWORD(wParam) == SCEN_SETFOCUS)
			{
				ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
				SystemUpdate();
			}

			if (pCurHexEdit->isVisible() == true)
			{
				switch (LOWORD(wParam))
				{
					case IDM_EDIT_CUT:
						pCurHexEdit->Cut();
						return TRUE;
					case IDM_EDIT_COPY:
						pCurHexEdit->Copy();
						return TRUE;
					case IDM_EDIT_PASTE:
						pCurHexEdit->Paste();
						return TRUE;
					case IDM_VIEW_ZOOMIN:
						pCurHexEdit->ZoomIn();
						return TRUE;
					case IDM_VIEW_ZOOMOUT:
						pCurHexEdit->ZoomOut();
						return TRUE;
					case IDM_VIEW_ZOOMRESTORE:
						pCurHexEdit->ZoomRestore();
						return TRUE;
					case IDM_SEARCH_FIND:
						findRepDlg.doDialog(pCurHexEdit->getHSelf(), FALSE);
						return TRUE;
					case IDM_SEARCH_REPLACE:
						findRepDlg.doDialog(pCurHexEdit->getHSelf(), TRUE);
						return TRUE;
					case IDM_SEARCH_FINDNEXT:
						findRepDlg.findNext(pCurHexEdit->getHSelf());
						return TRUE;
					case IDM_SEARCH_FINDPREV:
						findRepDlg.findPrev(pCurHexEdit->getHSelf());
						return TRUE;
					case IDM_SEARCH_VOLATILE_FINDNEXT:
						findRepDlg.findNext(pCurHexEdit->getHSelf(), TRUE);
						return TRUE;
					case IDM_SEARCH_VOLATILE_FINDPREV:
						findRepDlg.findPrev(pCurHexEdit->getHSelf(), TRUE);
						return TRUE;
					case IDM_SEARCH_GOTOLINE:
						gotoDlg.doDialog(pCurHexEdit->getHSelf());
						return TRUE;
					case IDM_SEARCH_NEXT_BOOKMARK:
						pCurHexEdit->NextBookmark();
						return TRUE;
					case IDM_SEARCH_PREV_BOOKMARK:
						pCurHexEdit->PrevBookmark();
						return TRUE;
					case IDM_SEARCH_TOGGLE_BOOKMARK:
						pCurHexEdit->ToggleBookmark();
						return TRUE;
					/* ignore this messages */
					case IDM_EDIT_LINE_UP:
					case IDM_EDIT_LINE_DOWN:
					case IDM_EDIT_DUP_LINE:
					case IDM_EDIT_JOIN_LINES:
					case IDM_EDIT_SPLIT_LINES:
					case IDM_EDIT_BLOCK_COMMENT:
					case IDM_EDIT_LTR:
					case IDM_EDIT_INS_TAB:
					case IDM_EDIT_RMV_TAB:
					case IDM_EDIT_STREAM_COMMENT:
						return TRUE;
					default:
						break;
				}
			}

			switch (LOWORD(wParam))
			{
				case IDM_FILE_RELOAD:
				{
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					pCurHexEdit->SetCompareResult(NULL);
					break;
				}
				case IDM_SEARCH_FIND:
				case IDM_SEARCH_REPLACE:
				{
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					g_hFindRepDlg = ::GetActiveWindow();
					break;
				}
				case IDM_FILE_SAVE:
				{
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					if (::SendMessage(g_HSource, SCI_GETMODIFY, 0, 0) == 0)
					{
						pCurHexEdit->ResetModificationState();
					}
					break;
				}
				case IDM_FILE_SAVEAS: 
				{
					CHAR oldPath[MAX_PATH];
					CHAR newPath[MAX_PATH];

					/* stop updating of active documents (workaround to keep possible HEX view open) */
					isNotepadCreated = FALSE;

					::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)oldPath);
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)newPath);
					pCurHexEdit->FileNameChanged(newPath);

					/* switch update again on and do the update now self */
					isNotepadCreated = TRUE;
					SystemUpdate();

					if (::SendMessage(g_HSource, SCI_GETMODIFY, 0, 0) == 0)
					{
						pCurHexEdit->ResetModificationState();
					}
					break;
				}
				case IDC_PREV_DOC:
				case IDC_NEXT_DOC:
				case IDM_FILE_CLOSE:
				case IDM_FILE_CLOSEALL:
				case IDM_FILE_CLOSEALL_BUT_CURRENT:
				case IDM_FILE_NEW:
				{
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					SystemUpdate();
					pCurHexEdit->doDialog();
					break;
				}
				case IDM_DOC_GOTO_ANOTHER_VIEW:
				case IDM_DOC_CLONE_TO_ANOTHER_VIEW:
				{
					const tHexProp*	p_prop = pCurHexEdit->GetHexProp();

					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					SystemUpdate();

					if (pCurHexEdit == &hexEdit1)
					{
						hexEdit1.SetHexProp(*p_prop);
						hexEdit1.doDialog();
					}
					else
					{
						hexEdit2.SetHexProp(*p_prop);
						hexEdit2.doDialog();
					}
					break;
				}
				default:
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					break;
			}
			break;
		}
		case NPPM_DOOPEN:
		{
			ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
			SystemUpdate();
			pCurHexEdit->doDialog();
			break;
		}
		case WM_NOTIFY:
		{
			SCNotification *notifyCode = (SCNotification *)lParam;

			if (((notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle) ||
				(notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle)) &&
				(notifyCode->nmhdr.code == SCN_SAVEPOINTREACHED))
			{
				SystemUpdate();
				if (TRUE != pCurHexEdit->GetModificationState())
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
				break;
			}

			switch (notifyCode->nmhdr.code)
			{
				case TCN_TABDELETE:
				case TCN_SELCHANGE:
				{
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					SystemUpdate();
					break;
				}
				case TCN_TABDROPPED:
				case TCN_TABDROPPEDOUTSIDE:
				{
					const tHexProp*	p_prop = pCurHexEdit->GetHexProp();

					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					SystemUpdate();

					if (pCurHexEdit == &hexEdit1)
					{
						hexEdit2.doDialog();
						hexEdit1.SetHexProp(*p_prop);
						hexEdit1.doDialog();
					}
					else
					{
						hexEdit1.doDialog();
						hexEdit2.SetHexProp(*p_prop);
						hexEdit2.doDialog();
					}
					break;
				}
				default:
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					break;
			}
			break;
		}
		default:
			ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
			break;
	}

	return ret;
}


/**************************************************************************
 *	Update functions
 */
void SystemUpdate(void)
{
	if (isNotepadCreated == FALSE)
		return;

	UINT		oldSC		= currentSC;
	UINT		newDocCnt	= 0;
	char		pszNewPath[MAX_PATH];

	/* update open files */
	UpdateCurrentHScintilla();
	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)pszNewPath);
	INT newOpenDoc1 = (INT)::SendMessage(nppData._nppHandle, NPPM_GETCURRENTDOCINDEX, 0, MAIN_VIEW);
	INT newOpenDoc2 = (INT)::SendMessage(nppData._nppHandle, NPPM_GETCURRENTDOCINDEX, 0, SUB_VIEW);

	if ((newOpenDoc1 != openDoc1) || (newOpenDoc2 != openDoc2) || 
		(strcmp(pszNewPath, currentPath) != 0) || (oldSC != currentSC))
	{
		/* set new file */
		strcpy(currentPath, pszNewPath);
		openDoc1 = newOpenDoc1;
		openDoc2 = newOpenDoc2;

		INT			i = 0;
		INT			docCnt1;
		INT			docCnt2;
		const char	**fileNames1;
		const char	**fileNames2;
		
		/* update doc information */
		docCnt1		= (INT)::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, (LPARAM)PRIMARY_VIEW);
		docCnt2		= (INT)::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, (LPARAM)SECOND_VIEW);
		fileNames1	= (const char **)new char*[docCnt1];
		fileNames2	= (const char **)new char*[docCnt2];

		for (i = 0; i < docCnt1; i++)
			fileNames1[i] = (char*)new char[MAX_PATH];
		for (i = 0; i < docCnt2; i++)
			fileNames2[i] = (char*)new char[MAX_PATH];

		::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMESPRIMARY, (WPARAM)fileNames1, (LPARAM)docCnt1);
		hexEdit1.UpdateDocs(fileNames1, docCnt1, openDoc1);

		::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMESSECOND, (WPARAM)fileNames2, (LPARAM)docCnt2);
		hexEdit2.UpdateDocs(fileNames2, docCnt2, openDoc2);

		for (i = 0; i < docCnt1; i++)
			delete [] fileNames1[i];
		for (i = 0; i < docCnt2; i++)
			delete [] fileNames2[i];
		delete [] fileNames1;
		delete [] fileNames2;

		/* update edit */
		if (currentSC == MAIN_VIEW)
			pCurHexEdit = &hexEdit1;
		else
			pCurHexEdit = &hexEdit2;

		ActivateWindow();
		setMenu();
	}
	DialogUpdate();
}

void ActivateWindow(void)
{
	/* activate correct window */
	if (currentSC == MAIN_VIEW)
	{
		::SendMessage(hexEdit1.getHSelf(), WM_ACTIVATE, TRUE, 0);
		::SendMessage(hexEdit2.getHSelf(), WM_ACTIVATE, FALSE, 0);
	}
	else
	{
		::SendMessage(hexEdit1.getHSelf(), WM_ACTIVATE, FALSE, 0);
		::SendMessage(hexEdit2.getHSelf(), WM_ACTIVATE, TRUE, 0);
	}
}

void DialogUpdate(void)
{
	/* Pattern/Replace dialog is visible? */
	if (((hexEdit1.isVisible() == false) && (hexEdit2.isVisible() == false)) &&
		(patDlg.isVisible() == true))
	{
		patDlg.destroy();
	}

	/* find replace dialog change */
	if ((pCurHexEdit->isVisible() == false) && (findRepDlg.isVisible() == true))
	{
		findRepDlg.display(FALSE);
		::SendMessage(nppData._nppHandle, WM_COMMAND, (findRepDlg.isFindReplace() == TRUE)?IDM_SEARCH_REPLACE:IDM_SEARCH_FIND, 0);
	}
	else if (g_hFindRepDlg != NULL)
	{
		if ((pCurHexEdit->isVisible() == true) && (::IsWindowVisible(g_hFindRepDlg) == TRUE))
		{
			char	text[5];
			::GetWindowText(g_hFindRepDlg, text, 5);
			findRepDlg.doDialog(pCurHexEdit->getHSelf(), (strcmp(text, "Find") != 0)?TRUE:FALSE);
			::SendMessage(g_hFindRepDlg, WM_COMMAND, IDCANCEL, 0);
		}
	}
}





/**************************************************************************
 *	Global Hex-Edit-Functions
 */
BOOL IsExtensionRegistered(LPCTSTR file)
{
	BOOL	bRet	= FALSE;

	LPTSTR	ptr		= NULL;
	LPTSTR	TEMP	= (LPTSTR) new TCHAR[MAX_PATH];

	strcpy(TEMP, prop.szExtensions);

	ptr = strtok(TEMP, " ");
	while (ptr != NULL)
	{
		if (stricmp(&file[strlen(file) - strlen(ptr)], ptr) == 0)
		{
			bRet = TRUE;
			break;
		}
		ptr = strtok(NULL, " ");
	}

	delete [] TEMP;

	return bRet;
}

void ChangeClipboardDataToHex(tClipboard *clipboard)
{
	char*	text	= clipboard->text;
	INT		length	= clipboard->length;

	clipboard->length	= length * 3;
	clipboard->text		= (char*) new char[clipboard->length+1];

	strcpy(clipboard->text, hexMask[(UCHAR)text[0]]);
	for (INT i = 1; i < length; i++)
	{
		strcat(clipboard->text, " ");
		strcat(clipboard->text, hexMask[(UCHAR)text[i]]);
	}

	clipboard->text[clipboard->length] = 0;
}

void LittleEndianChange(HWND hTarget, HWND hSource)
{
	const 
	tHexProp*	p_prop	= pCurHexEdit->GetHexProp();
	UINT		length  = 0;
	char*		buffer  = NULL;

	/* for buffer UNDO */
	ScintillaMsg(hTarget, SCI_BEGINUNDOACTION);

	/* get text of current scintilla */
	length = ScintillaMsg(hSource, SCI_GETTEXTLENGTH) + 1;
	buffer = (char*)new char[length];
	::SendMessage(hSource, SCI_GETTEXT, length, (LPARAM)buffer);

	/* convert when property is little */
	if (p_prop->isLittle == TRUE)
	{
		char *temp  = (char*)new char[length];
		char *pText	= buffer;

		/* i must be unsigned */
		for (UINT i = 0; i < length; i++)
		{
			temp[i] = buffer[i];
		}

		UINT offset = (length-1) % p_prop->bits;
		UINT max	= (length-1) / p_prop->bits + 1;

		for (i = 1; i <= max; i++)
		{
			if (i == max)
			{
				for (UINT j = 1; j <= offset; j++)
				{
					*pText = temp[length-1-j];
					pText++;
				}
			}
			else
			{
				for (SHORT j = 1; j <= p_prop->bits; j++)
				{
					*pText = temp[p_prop->bits*i-j];
					pText++;
				}
			}
		}
		*pText = NULL;
		delete [] temp;
	}

	/* set text in target */
	::SendMessage(hTarget, SCI_CLEARALL, 0, 0);
	::SendMessage(hTarget, SCI_ADDTEXT, length, (LPARAM)buffer);
	::SendMessage(hTarget, SCI_ENDUNDOACTION, 0, 0);
	delete [] buffer;
}


eError replaceLittleToBig(HWND hSource, INT startPos, INT lengthOld, INT lengthNew)
{
	const tHexProp*	p_prop	= NULL;

	UpdateCurrentHScintilla();
	if (currentSC == MAIN_VIEW)
		p_prop = hexEdit1.GetHexProp();
	else
		p_prop = hexEdit2.GetHexProp();

	if (p_prop->isLittle == TRUE)
	{
		if (startPos % p_prop->bits)
		{
			return E_START;
		}
		if ((lengthOld % p_prop->bits) || (lengthNew % p_prop->bits))
		{
			return E_STRIDE;
		}
	}

	char*	text = (char*)new char[lengthNew+1];

	/* get new text */
	::SendMessage(hSource, SCI_SETSELECTIONSTART, startPos, 0);
	::SendMessage(hSource, SCI_SETSELECTIONEND, startPos + lengthNew, 0);
	::SendMessage(hSource, SCI_GETSELTEXT, 0, (LPARAM)text);

	/* set in target */
	if (p_prop->isLittle == FALSE)
	{
		ScintillaMsg(SCI_SETTARGETSTART, startPos);
		ScintillaMsg(SCI_SETTARGETEND, startPos + lengthOld);
		ScintillaMsg(SCI_REPLACETARGET, lengthNew, (LPARAM)text);
	}
	else
	{
		INT		length	  = (lengthOld < lengthNew ? lengthNew:lengthOld);
		INT		posSource = startPos;
		INT		posTarget = 0;

		ScintillaMsg(SCI_BEGINUNDOACTION);

		for (INT i = 0; i < length; i++)
		{
			/* set position of change */
			posTarget = posSource - (posSource % p_prop->bits) + ((p_prop->bits-1) - (posSource % p_prop->bits));

			if ((i < lengthOld) && (i < lengthNew))
			{
				ScintillaMsg(SCI_SETTARGETSTART, posTarget);
				ScintillaMsg(SCI_SETTARGETEND, posTarget + 1);
				ScintillaMsg(SCI_REPLACETARGET, 1, (LPARAM)&text[i]);
			}
			else if (i < lengthOld)
			{
				/* old string is longer as the new one */
				ScintillaMsg(SCI_SETTARGETSTART, posTarget);
				ScintillaMsg(SCI_SETTARGETEND, posTarget + 1);
				ScintillaMsg(SCI_REPLACETARGET, 0, (LPARAM)'\0');

				if (!((i+1) % p_prop->bits))
					posSource = startPos + lengthNew - 1;
			}
			else if (i < lengthNew)
			{
				/* new string is longer as the old one */
				ScintillaMsg(SCI_SETCURRENTPOS, posTarget - p_prop->bits + 1);
				ScintillaMsg(SCI_ADDTEXT, 1, (LPARAM)&text[i]);
				if (!((i+1) % p_prop->bits))
					posSource += p_prop->bits;
				posSource--;
			}

			posSource++;
		}

		ScintillaMsg(SCI_ENDUNDOACTION);
	}

	delete [] text;

	return E_OK;
}


void DoCompare(void)
{
	BOOL		doMatch		= TRUE;
	UINT		posSrc		= 0;
	UINT		posTgt		= 0;
	LPSTR		compare1	= NULL;
	LPSTR		compare2	= NULL;
	const tHexProp* p_prop1 = hexEdit1.GetHexProp();
	const tHexProp* p_prop2 = hexEdit2.GetHexProp();

	if ((p_prop1->bits != p_prop2->bits) || (p_prop1->columns != p_prop2->columns) || (p_prop1->isBin != p_prop2->isBin))
	{
		/* ask which hex edits should be used to have same settings in both */
		CompareDlg	dlg;
		dlg.init((HINSTANCE)g_hModule, nppData);
		if (dlg.doDialog(&hexEdit1, &hexEdit2, currentSC) == IDCANCEL) {
			return;
		}
	}

	/* get texts to comapre */
	INT		length1 = ScintillaMsg(nppData._scintillaMainHandle, SCI_GETTEXTLENGTH) + 1;
	INT		length2 = ScintillaMsg(nppData._scintillaSecondHandle, SCI_GETTEXTLENGTH) + 1;
	LPSTR	buffer1 = (LPSTR)new CHAR[length1];
	LPSTR	buffer2 = (LPSTR)new CHAR[length2];
	::SendMessage(nppData._scintillaMainHandle, SCI_GETTEXT, length1, (LPARAM)buffer1);
	::SendMessage(nppData._scintillaSecondHandle, SCI_GETTEXT, length2, (LPARAM)buffer2);

	/* create memory to store comopare results */
	compare1 = (LPSTR)new CHAR[length1 / p_prop1->bits + p_prop1->columns];
	::ZeroMemory(compare1, length1 / p_prop1->bits + p_prop1->columns);
	compare2 = (LPSTR)new CHAR[length2 / p_prop2->bits + p_prop1->columns];
	::ZeroMemory(compare2, length2 / p_prop2->bits + p_prop1->columns);

	while ((posSrc < length1) && (posSrc < length2))
	{
		if (buffer1[posSrc] != buffer2[posSrc])
		{
			doMatch			 = FALSE;
			compare1[posTgt] = TRUE;
			compare2[posTgt] = TRUE;
		}
		/* increment source buffer */
		posSrc++;

		/* increment target buffer */
		if (p_prop1->bits != 1) {
			posTgt = posSrc / p_prop1->bits;
		} else {
			posTgt++;
		}
	}

	if (doMatch == TRUE)
	{
		if (NLMessageBox((HINSTANCE)g_hModule, nppData._nppHandle, "MsgBox CompMatch", MB_OK) == FALSE)
			::MessageBox(nppData._nppHandle, "Files Match.", "Hex-Editor Compare", MB_OK);
		delete [] compare1;
		delete [] compare2;
	}
	else
	{
		hexEdit1.SetCompareResult(compare1);
		hexEdit2.SetCompareResult(compare2);
	}

	delete [] buffer1;
	delete [] buffer2;
}


/**************************************************************************
 *	Other functions
 */
eNppCoding GetNppEncoding(void)
{
	eNppCoding	ret		= HEX_CODE_NPP_ASCI;
	HMENU		hMenu	= ::GetMenu(nppData._nppHandle);

	if ((::GetMenuState(hMenu, IDM_FORMAT_UCS_2BE, MF_BYCOMMAND) & MF_CHECKED) != 0)
	{
		ret = HEX_CODE_NPP_USCBE;
	}
	else if	((::GetMenuState(hMenu, IDM_FORMAT_UCS_2LE, MF_BYCOMMAND) & MF_CHECKED) != 0)
	{
		ret = HEX_CODE_NPP_USCLE;
	}
	else if	((::GetMenuState(hMenu, IDM_FORMAT_UTF_8, MF_BYCOMMAND) & MF_CHECKED) != 0)
	{
		ret = HEX_CODE_NPP_UTF8_BOM;
	}
	else if ((::GetMenuState(hMenu, IDM_FORMAT_AS_UTF_8, MF_BYCOMMAND) & MF_CHECKED) != 0)
	{
		ret = HEX_CODE_NPP_UTF8;
	}

	return ret;
}

void ChangeNppMenu(BOOL toHexStyle, HWND hSci)
{
	if ((toHexStyle == isMenuHex) || (hSci != g_HSource))
		return;

	TCHAR	text[64];
	HMENU	hMenu	 = NULL;
	HMENU	hMenuNpp = ::GetMenu(nppData._nppHandle);

	/* store if menu will be modified */
	isMenuHex = toHexStyle;

	/* activate/deactive menu entries */
	::EnableMenuItem(hMenuNpp, MENUINDEX_FORMAT, MF_BYPOSITION | (toHexStyle?MF_GRAYED:MF_ENABLED));
	::EnableMenuItem(hMenuNpp, MENUINDEX_LANGUAGE, MF_BYPOSITION | (toHexStyle?MF_GRAYED:MF_ENABLED));
	::EnableMenuItem(hMenuNpp, MENUINDEX_MACRO, MF_BYPOSITION | (toHexStyle?MF_GRAYED:MF_ENABLED));
	::DrawMenuBar(nppData._nppHandle);

#ifdef TODO_SUPPORT_HEX_MENU

	/* greate own menu for file */
	if (hMenuFile == NULL)
	{
		hMenuFile = ::CreatePopupMenu();
		AppendNppMenu(hMenuNpp, IDM_FILE_NEW, hMenuFile);
		AppendNppMenu(hMenuNpp, IDM_FILE_OPEN, hMenuFile);
		AppendNppMenu(hMenuNpp, IDM_FILE_RELOAD, hMenuFile);
		AppendNppMenu(hMenuNpp, IDM_FILE_SAVE, hMenuFile);
		AppendNppMenu(hMenuNpp, IDM_FILE_SAVEAS, hMenuFile);
		AppendNppMenu(hMenuNpp, IDM_FILE_SAVECOPYAS, hMenuFile);
		AppendNppMenu(hMenuNpp, IDM_FILE_SAVEALL, hMenuFile);
		AppendNppMenu(hMenuNpp, IDM_FILE_CLOSE, hMenuFile);
		AppendNppMenu(hMenuNpp, IDM_FILE_CLOSEALL, hMenuFile);
		AppendNppMenu(hMenuNpp, IDM_FILE_CLOSEALL_BUT_CURRENT, hMenuFile);
		::AppendMenu(hMenuFile, MF_SEPARATOR, 0, NULL);
		AppendNppMenu(hMenuNpp, IDM_FILE_LOADSESSION, hMenuFile);
		AppendNppMenu(hMenuNpp, IDM_FILE_SAVESESSION, hMenuFile);
		::AppendMenu(hMenuFile, MF_SEPARATOR, 0, NULL);
		AppendNppMenu(hMenuNpp, IDM_FILE_EXIT, hMenuFile);
	}

	/* exchange menus */
	hMenu = ::GetSubMenu(hMenuNpp, MENUINDEX_FILE);
	::GetMenuString(hMenuNpp, MENUINDEX_FILE, text, 64, MF_BYPOSITION);
	::ModifyMenu(hMenuNpp, MENUINDEX_FILE, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hMenuFile, text);
	hMenuFile = hMenu;
#endif
}

void AppendNppMenu(HMENU hMenuNpp, UINT idItem, HMENU & hMenu)
{
	TCHAR	text[64];
	::GetMenuString(hMenuNpp, idItem, text, 64, MF_BYCOMMAND);
	::AppendMenu(hMenu, MF_STRING, idItem, text);
}


void ClientToScreen(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ClientToScreen( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ClientToScreen( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
}


void ScreenToClient(HWND hWnd, RECT* rect)
{
	POINT		pt;

	pt.x		 = rect->left;
	pt.y		 = rect->top;
	::ScreenToClient( hWnd, &pt );
	rect->left   = pt.x;
	rect->top    = pt.y;

	pt.x		 = rect->right;
	pt.y		 = rect->bottom;
	::ScreenToClient( hWnd, &pt );
	rect->right  = pt.x;
	rect->bottom = pt.y;
}
