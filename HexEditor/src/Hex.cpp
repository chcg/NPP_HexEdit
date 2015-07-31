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
CONST TCHAR  PLUGIN_NAME[] = _T("HEX-Editor");

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
HexEdit*		pCurHexEdit		= NULL;

/* get system information */
BOOL	isNotepadCreated		= FALSE;

/* notepad menus */
BOOL			isMenuHex		= FALSE;
vector<tMenu>	vMenuInfoFile;
vector<tMenu>	vMenuInfoEdit;
vector<tMenu>	vMenuInfoSearch;
vector<tMenu>	vMenuInfoView;


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
			_tcscpy(funcItem[0]._itemName, _T("View in &HEX"));
			_tcscpy(funcItem[1]._itemName, _T("&Compare HEX"));
			_tcscpy(funcItem[2]._itemName, _T("Clear Compare &Result"));
			_tcscpy(funcItem[4]._itemName, _T("&Insert Columns..."));
			_tcscpy(funcItem[5]._itemName, _T("&Pattern Replace..."));
			_tcscpy(funcItem[7]._itemName, _T("&Options..."));
			_tcscpy(funcItem[8]._itemName, _T("&Help..."));

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

			vMenuInfoFile.clear();
			vMenuInfoEdit.clear();
			vMenuInfoSearch.clear();
			vMenuInfoView.clear();

			if (g_TBHex.hToolbarBmp)
				::DeleteObject(g_TBHex.hToolbarBmp);
			if (g_TBHex.hToolbarIcon)
				::DestroyIcon(g_TBHex.hToolbarIcon);

			/* Remove subclaasing */
			SetWindowLongPtr(nppData._nppHandle, GWL_WNDPROC, (LONG)wndProcNotepad);
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
	wndProcNotepad = (WNDPROC)SetWindowLongPtr(nppData._nppHandle, GWL_WNDPROC, (LPARAM)SubWndProcNotepad);

	pCurHexEdit = &hexEdit1;
	setHexMask();
}

extern "C" __declspec(dllexport) LPCTSTR getName()
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
					tHexProp	hexProp1	= hexEdit1.GetHexProp();
					tHexProp	hexProp2	= hexEdit2.GetHexProp();
					INT			length		= notifyCode->length;

					if ((hexProp1.pszFileName != NULL) && (hexProp2.pszFileName != NULL) &&
						(wcscmp(hexProp1.pszFileName, hexProp2.pszFileName) == 0))
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


#ifdef UNICODE
/***
 *	isUnicode()
 *
 *	This function notificates Notepad++ the unicode compability
 */
extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
#endif

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
			_tcscpy(configPath, vPaths[i].c_str());
			::CreateDirectory(configPath, NULL);
		}
	}

	_tcscpy(iniFilePath, configPath);
	_tcscat(iniFilePath, HEXEDIT_INI);
	if (PathFileExists(iniFilePath) == FALSE)
	{
		HANDLE	hFile			= NULL;
#ifdef UNICODE
		CHAR	szBOM[]			= {0xFF, 0xFE};
		DWORD	dwByteWritten	= 0;
#endif
			
		if (hFile != INVALID_HANDLE_VALUE)
		{
			hFile = ::CreateFile(iniFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#ifdef UNICODE
			::WriteFile(hFile, szBOM, sizeof(szBOM), &dwByteWritten, NULL);
#endif
			::CloseHandle(hFile);
		}
	}

	prop.hexProp.addWidth		= ::GetPrivateProfileInt(dlgEditor, addWidth, 8, iniFilePath);
	prop.hexProp.columns		= ::GetPrivateProfileInt(dlgEditor, columns, 16, iniFilePath);
	prop.hexProp.bits			= ::GetPrivateProfileInt(dlgEditor, bits, HEX_BYTE, iniFilePath);
	prop.hexProp.isBin			= ::GetPrivateProfileInt(dlgEditor, bin, FALSE, iniFilePath);
	prop.hexProp.isLittle		= ::GetPrivateProfileInt(dlgEditor, little, FALSE, iniFilePath);
	::GetPrivateProfileString(dlgEditor, extensions, _T(""), prop.autoProp.szExtensions, MAX_PATH, iniFilePath);
	::GetPrivateProfileString(dlgEditor, percent, _T(""), prop.autoProp.szPercent, 4, iniFilePath);
	prop.colorProp.rgbRegTxt	= ::GetPrivateProfileInt(dlgEditor, rgbRegTxt, RGB(0x00,0x00,0x00), iniFilePath);
	prop.colorProp.rgbRegBk		= ::GetPrivateProfileInt(dlgEditor, rgbRegBk, RGB(0xFF,0xFF,0xFF), iniFilePath);
	prop.colorProp.rgbSelTxt	= ::GetPrivateProfileInt(dlgEditor, rgbSelTxt, RGB(0xFF,0xFF,0xFF), iniFilePath);
	prop.colorProp.rgbSelBk		= ::GetPrivateProfileInt(dlgEditor, rgbSelBk, RGB(0x88,0x88,0xff), iniFilePath);
	prop.colorProp.rgbDiffTxt	= ::GetPrivateProfileInt(dlgEditor, rgbDiffTxt, RGB(0xFF,0xFF,0xFF), iniFilePath);
	prop.colorProp.rgbDiffBk	= ::GetPrivateProfileInt(dlgEditor, rgbDiffBk, RGB(0xff,0x88,0x88), iniFilePath);
	prop.colorProp.rgbBkMk		= ::GetPrivateProfileInt(dlgEditor, rgbBkMk, RGB(0xFF,0x00,0x00), iniFilePath);
	::GetPrivateProfileString(dlgEditor, fontname, _T("Courier New"), prop.fontProp.szFontName, 256, iniFilePath);
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
	TCHAR	temp[64];

	::WritePrivateProfileString(dlgEditor, addWidth, _itot(prop.hexProp.addWidth, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, columns, _itot(prop.hexProp.columns, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, bits, _itot(prop.hexProp.bits, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, bin, _itot(prop.hexProp.isBin, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, little, _itot(prop.hexProp.isLittle, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, extensions, prop.autoProp.szExtensions, iniFilePath);
	::WritePrivateProfileString(dlgEditor, percent, prop.autoProp.szPercent, iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbRegTxt, _itot(prop.colorProp.rgbRegTxt, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbRegBk, _itot(prop.colorProp.rgbRegBk, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbSelTxt, _itot(prop.colorProp.rgbSelTxt, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbSelBk, _itot(prop.colorProp.rgbSelBk, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbDiffTxt, _itot(prop.colorProp.rgbDiffTxt, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbDiffBk, _itot(prop.colorProp.rgbDiffBk, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbBkMk, _itot(prop.colorProp.rgbBkMk, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, fontname, prop.fontProp.szFontName, iniFilePath);
	::WritePrivateProfileString(dlgEditor, fontsize, _itot(prop.fontProp.iFontSizeElem, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, capital, _itot(prop.fontProp.isCapital, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, bold, _itot(prop.fontProp.isBold, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, italic, _itot(prop.fontProp.isItalic, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, underline, _itot(prop.fontProp.isUnderline, temp, 10), iniFilePath);
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
	HMENU		hMenu	= ::GetMenu(nppData._nppHandle);
	tHexProp	hexProp	= pCurHexEdit->GetHexProp();

	::EnableMenuItem(hMenu, funcItem[4]._cmdID, MF_BYCOMMAND | (hexProp.isVisible?0:MF_GRAYED));
	::EnableMenuItem(hMenu, funcItem[5]._cmdID, MF_BYCOMMAND | (hexProp.isVisible?0:MF_GRAYED));

	tHexProp hexProp1	= hexEdit1.GetHexProp();
	tHexProp hexProp2	= hexEdit2.GetHexProp();
	if ((hexProp1.isVisible == TRUE) && (hexProp2.isVisible == TRUE) &&
		(wcsicmp(hexProp1.pszFileName, hexProp2.pszFileName) != 0)) {
		::EnableMenuItem(hMenu, funcItem[1]._cmdID, MF_BYCOMMAND);
	} else {
		::EnableMenuItem(hMenu, funcItem[1]._cmdID, MF_BYCOMMAND | MF_GRAYED);
	}

	if (hexProp.pCompareData != NULL) {
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

LPCTSTR getFontName(void)
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
UINT ScintillaGetText(char *text, INT start, INT end) 
{
	TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText  = text;
	return (UINT)ScintillaMsg(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}

UINT ScintillaGetText(HWND hWnd, char *text, INT start, INT end)
{
	TextRange tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText  = text;
	return (UINT)::SendMessage(hWnd, SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
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
	if (pCurHexEdit->GetHexProp().pCompareData != NULL) {
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
		hexEdit1.UpdateFont();
		hexEdit2.UpdateFont();
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
					case IDM_EDIT_SELECTALL:
						pCurHexEdit->SelectAll();
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
					case IDM_SEARCH_CLEAR_BOOKMARKS:
						pCurHexEdit->ClearBookmarks();
						return TRUE;
					case IDM_SEARCH_CUTMARKEDLINES:
						pCurHexEdit->CutBookmarkLines();
						return TRUE;
					case IDM_SEARCH_COPYMARKEDLINES:
						pCurHexEdit->CopyBookmarkLines();
						return TRUE;
					case IDM_SEARCH_PASTEMARKEDLINES:
						pCurHexEdit->PasteBookmarkLines();
						return TRUE;
					case IDM_SEARCH_DELETEMARKEDLINES:
						pCurHexEdit->DeleteBookmarkLines();
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
				case IDM_FILE_RENAME:
				{
					WCHAR oldPath[MAX_PATH];
					WCHAR newPath[MAX_PATH];

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
					pCurHexEdit->SetStatusBar();
					break;
				}
				case IDM_VIEW_GOTO_ANOTHER_VIEW:
				case IDM_VIEW_CLONE_TO_ANOTHER_VIEW:
				{
					tHexProp hexProp = pCurHexEdit->GetHexProp();

					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					SystemUpdate();

					pCurHexEdit->SetHexProp(hexProp);
					pCurHexEdit->doDialog();
					pCurHexEdit->SetStatusBar();
					break;
				}
				case IDM_VIEW_SWITCHTO_MAIN:
				case IDM_VIEW_SWITCHTO_SUB:
				{
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					pCurHexEdit->SetStatusBar();
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
					pCurHexEdit->SetStatusBar();
					break;
				}
				case TCN_TABDROPPED:
				case TCN_TABDROPPEDOUTSIDE:
				{
					HexEdit* pOldHexEdit = pCurHexEdit;
					tHexProp hexProp	 = pCurHexEdit->GetHexProp();

					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					SystemUpdate();

					if (pOldHexEdit != pCurHexEdit)
					{
						if (pCurHexEdit == &hexEdit1)
						{
							hexEdit2.doDialog();
						}
						else
						{
							hexEdit1.doDialog();
						}
						pCurHexEdit->SetHexProp(hexProp);
						pCurHexEdit->doDialog();
						pCurHexEdit->SetStatusBar();
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
	TCHAR		pszNewPath[MAX_PATH];

	/* update open files */
	UpdateCurrentHScintilla();
	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)pszNewPath);
	INT newOpenDoc1 = (INT)::SendMessage(nppData._nppHandle, NPPM_GETCURRENTDOCINDEX, 0, MAIN_VIEW);
	INT newOpenDoc2 = (INT)::SendMessage(nppData._nppHandle, NPPM_GETCURRENTDOCINDEX, 0, SUB_VIEW);

	if ((newOpenDoc1 != openDoc1) || (newOpenDoc2 != openDoc2) || 
		(_tcscmp(pszNewPath, currentPath) != 0) || (oldSC != currentSC))
	{
		/* set new file */
		_tcscpy(currentPath, pszNewPath);
		openDoc1 = newOpenDoc1;
		openDoc2 = newOpenDoc2;

		INT			i = 0;
		INT			docCnt1;
		INT			docCnt2;
		LPCTSTR		*fileNames1;
		LPCTSTR		*fileNames2;
		BOOL		isAllocOk = TRUE;
		
		/* update doc information */
		docCnt1		= (INT)::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, (LPARAM)PRIMARY_VIEW);
		docCnt2		= (INT)::SendMessage(nppData._nppHandle, NPPM_GETNBOPENFILES, 0, (LPARAM)SECOND_VIEW);
		fileNames1	= (LPCTSTR*)new LPTSTR[docCnt1];
		fileNames2	= (LPCTSTR*)new LPTSTR[docCnt2];

		if ((fileNames1 != NULL) && (fileNames2 != NULL))
		{
			for (i = 0; (i < docCnt1) && (isAllocOk == TRUE); i++) {
				fileNames1[i] = (LPWSTR)new TCHAR[MAX_PATH];
				if (fileNames1[i] == NULL)
					isAllocOk = FALSE;
			}
			for (i = 0; (i < docCnt2) && (isAllocOk == TRUE); i++) {
				fileNames2[i] = (LPWSTR)new TCHAR[MAX_PATH];
				if (fileNames2[i] == NULL)
					isAllocOk = FALSE;
			}

			if (isAllocOk == TRUE)
			{
				::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMESPRIMARY, (WPARAM)fileNames1, (LPARAM)docCnt1);
				hexEdit1.UpdateDocs(fileNames1, docCnt1, openDoc1);

				::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMESSECOND, (WPARAM)fileNames2, (LPARAM)docCnt2);
				hexEdit2.UpdateDocs(fileNames2, docCnt2, openDoc2);

				/* update edit */
				if (currentSC == MAIN_VIEW)
					pCurHexEdit = &hexEdit1;
				else
					pCurHexEdit = &hexEdit2;

				ActivateWindow();
				setMenu();
			}

			if (fileNames1 != NULL)
			{
				for (i = 0; i < docCnt1; i++)
					if (fileNames1[i] != NULL)
						delete [] fileNames1[i];
				delete [] fileNames1;
			}
			if (fileNames2 != NULL)
			{
				for (i = 0; i < docCnt2; i++)
					if (fileNames2[i] != NULL)
						delete [] fileNames2[i];
				delete [] fileNames2;
			}
		}
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
			TCHAR	text[5];
			::GetWindowText(g_hFindRepDlg, text, 5);
			findRepDlg.doDialog(pCurHexEdit->getHSelf(), (_tcscmp(text, _T("Find")) != 0)?TRUE:FALSE);
			::SendMessage(g_hFindRepDlg, WM_COMMAND, IDCANCEL, 0);
		}
	}
}





/**************************************************************************
 *	Global Hex-Edit-Functions
 */
BOOL IsExtensionRegistered(LPCWSTR file)
{
	BOOL	bRet	= FALSE;

	LPWSTR	TEMP	= (LPWSTR) new WCHAR[MAX_PATH];
	LPWSTR	ptr		= NULL;

	if (TEMP != NULL)
	{
		wcscpy(TEMP, prop.autoProp.szExtensions);

		ptr = wcstok(TEMP, L" ");
		while (ptr != NULL)
		{
			if (wcsicmp(&file[wcslen(file) - wcslen(ptr)], ptr) == 0)
			{
				bRet = TRUE;
				break;
			}
			ptr = wcstok(NULL, L" ");
		}

		delete [] TEMP;
	}

	return bRet;
}

BOOL IsPercentReached(LPCWSTR file)
{
	BOOL	bRet		= FALSE;
	DWORD	dwPercent	= (DWORD)_ttoi(prop.autoProp.szPercent);

	/* return if value is not between 1 - 99 */
	if ((dwPercent < 1) || (dwPercent >= 100))
		return bRet;

	/* open file if exists */
	HANDLE	hFile	= ::CreateFileW(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
		return bRet;

	/* calculate the count of necessary zeros */
	DWORD	dwMax		= (AUTOSTART_MAX * dwPercent) / 100;

	/* create buffer and read from file */
	DWORD	dwBytesRead	= 0;
	LPBYTE	pReadBuffer	= (LPBYTE)new BYTE[AUTOSTART_MAX];

	if (pReadBuffer != NULL)
	{
		::ZeroMemory(pReadBuffer, AUTOSTART_MAX);

		if(::ReadFile(hFile, pReadBuffer, AUTOSTART_MAX, &dwBytesRead, NULL) != FALSE)
		{
			/* check for coding */
			if (((pReadBuffer[0] != 0xFF) && (pReadBuffer[1] != 0xFE)) &&
				((pReadBuffer[0] != 0xFE) && (pReadBuffer[1] != 0xFF)) &&
				((pReadBuffer[0] != 0xEF) && (pReadBuffer[1] != 0xBB) && (pReadBuffer[2] != 0xBF)))
			{
				DWORD	dwCount	= 0;

				for (DWORD i = 0; i < dwBytesRead; i++)
				{
					if ((pReadBuffer[i] < 0x20) && (pReadBuffer[i] != '\t') &&
						(pReadBuffer[i] != '\r') && (pReadBuffer[i] != '\n')) {
						dwCount++;
					}
					if (dwCount >= dwMax) {
						bRet = TRUE;
						break;
					}
				}
			}
		}
		delete [] pReadBuffer;
	}
	::CloseHandle(hFile);

	return bRet;
}

void ChangeClipboardDataToHex(tClipboard *clipboard)
{
	char*	text	= clipboard->text;
	INT		length	= clipboard->length;

	clipboard->length	= length * 3;
	clipboard->text		= (char*) new char[clipboard->length+1];

	if (clipboard->text != NULL)
	{
		strcpy(clipboard->text, hexMask[(UCHAR)text[0]]);
		for (INT i = 1; i < length; i++)
		{
			strcat(clipboard->text, " ");
			strcat(clipboard->text, hexMask[(UCHAR)text[i]]);
		}
		clipboard->text[clipboard->length] = 0;
	}
}

BOOL LittleEndianChange(HWND hTarget, HWND hSource, LPINT offset, LPINT length)
{
	if ((hTarget == NULL) || (hSource == NULL) || (offset == NULL) || (length == NULL))
		return FALSE;

	tHexProp	hexProp	= pCurHexEdit->GetHexProp();
	BOOL		bRet	= FALSE;
	UINT		lenSrc  = 0;
	UINT		lenCpy	= 0;
	LPSTR		buffer  = NULL;
	INT			posBeg	= *offset;
	INT			posEnd	= *offset + *length;

	/* get source length of scintilla context */
	lenSrc = ScintillaMsg(hSource, SCI_GETTEXTLENGTH);

	if (posBeg < 0)
		posBeg = 0;

	/* calculate positions alligned */
	posBeg -= (posBeg % hexProp.bits);
	if (posEnd % hexProp.bits) {
		posEnd += hexProp.bits - (posEnd % hexProp.bits);
	}
	if (posEnd > lenSrc) {
		posEnd = lenSrc;
	}

	if (*length <= FIND_BLOCK)
	{
		/* create a buffer to copy data */
		buffer = (LPSTR)new CHAR[(posEnd - posBeg) + 1];

		if (buffer != NULL)
		{
			/* to clear the content of context begin UNDO */
			::SendMessage(hTarget, SCI_BEGINUNDOACTION, 0, 0);
			::SendMessage(hTarget, SCI_CLEARALL, 0, 0);

			/* copy text into the buffer */
			lenCpy = ScintillaGetText(hSource, buffer, posBeg, posEnd);

			/* convert when property is little */
			if (hexProp.isLittle == TRUE)
			{
				LPSTR temp  = (LPSTR)new CHAR[lenCpy + 1];
				LPSTR pText	= buffer;

				/* it must be unsigned */
				for (UINT i = 0; i < lenCpy; i++) {
					temp[i] = buffer[i];
				}

				UINT offset = (lenCpy) % hexProp.bits;
				UINT max	= (lenCpy) / hexProp.bits + 1;

				for (i = 1; i <= max; i++)
				{
					if (i == max)
					{
						for (UINT j = 1; j <= offset; j++)
						{
							*pText = temp[lenCpy-j];
							pText++;
						}
					}
					else
					{
						for (SHORT j = 1; j <= hexProp.bits; j++)
						{
							*pText = temp[hexProp.bits*i-j];
							pText++;
						}
					}
				}
				*pText = NULL;
				delete [] temp;
			}

			/* add text to target */
			::SendMessage(hTarget, SCI_ADDTEXT, lenCpy, (LPARAM)buffer);
			::SendMessage(hTarget, SCI_ENDUNDOACTION, 0, 0);

			/* everything is fine, set return values */
			*offset = posBeg;
			*length = posEnd - posBeg;
			bRet = TRUE;
		}
		delete [] buffer;
	}
	return bRet;
}

eError replaceLittleToBig(HWND hTarget, HWND hSource, INT startSrc, INT startTgt, INT lengthOld, INT lengthNew)
{
	tHexProp hexProp;

	if (currentSC == MAIN_VIEW)
		hexProp = hexEdit1.GetHexProp();
	else
		hexProp = hexEdit2.GetHexProp();

	if (hexProp.isLittle == TRUE)
	{
		if (startSrc % hexProp.bits)
		{
			return E_START;
		}
		if ((lengthOld % hexProp.bits) || (lengthNew % hexProp.bits))
		{
			return E_STRIDE;
		}
	}

	char*	text = (char*)new char[lengthNew+1];

	if (text != NULL)
	{
		if (hSource)
		{
			/* get new text */
			::SendMessage(hSource, SCI_SETSELECTIONSTART, startSrc, 0);
			::SendMessage(hSource, SCI_SETSELECTIONEND, startSrc + lengthNew, 0);
			::SendMessage(hSource, SCI_GETSELTEXT, 0, (LPARAM)text);
		}

		/* set in target */
		if (hexProp.isLittle == FALSE)
		{
			ScintillaMsg(hTarget, SCI_SETTARGETSTART, startTgt);
			ScintillaMsg(hTarget, SCI_SETTARGETEND, startTgt + lengthOld);
			ScintillaMsg(hTarget, SCI_REPLACETARGET, lengthNew, (LPARAM)text);
		}
		else
		{
			INT		length	  = (lengthOld < lengthNew ? lengthNew:lengthOld);
			INT		posSource = startSrc;
			INT		posTarget = 0;

			ScintillaMsg(hTarget, SCI_BEGINUNDOACTION);

			for (INT i = 0; i < length; i++)
			{
				/* set position of change */
				posTarget = posSource - (posSource % hexProp.bits) + ((hexProp.bits-1) - (posSource % hexProp.bits)) + startTgt;

				if ((i < lengthOld) && (i < lengthNew))
				{
					ScintillaMsg(hTarget, SCI_SETTARGETSTART, posTarget);
					ScintillaMsg(hTarget, SCI_SETTARGETEND, posTarget + 1);
					ScintillaMsg(hTarget, SCI_REPLACETARGET, 1, (LPARAM)&text[i]);
				}
				else if (i < lengthOld)
				{
					/* old string is longer as the new one */
					ScintillaMsg(hTarget, SCI_SETTARGETSTART, posTarget);
					ScintillaMsg(hTarget, SCI_SETTARGETEND, posTarget + 1);
					ScintillaMsg(hTarget, SCI_REPLACETARGET, 0, (LPARAM)'\0');

					if (!((i+1) % hexProp.bits))
						posSource = startSrc + lengthNew - 1;
				}
				else if (i < lengthNew)
				{
					/* new string is longer as the old one */
					ScintillaMsg(hTarget, SCI_SETCURRENTPOS, posTarget - hexProp.bits + 1);
					ScintillaMsg(hTarget, SCI_ADDTEXT, 1, (LPARAM)&text[i]);
					if (!((i+1) % hexProp.bits))
						posSource += hexProp.bits;
					posSource--;
				}

				posSource++;
			}

			ScintillaMsg(hTarget, SCI_ENDUNDOACTION);
		}
		delete [] text;

		return E_OK;
	}

	return E_MEMORY;
}


void DoCompare(void)
{
	BOOL		doMatch		= TRUE;
	UINT		posSrc		= 0;
	UINT		posTgt		= 0;
	LPSTR		compare1	= NULL;
	LPSTR		compare2	= NULL;
	tHexProp	hexProp1	= hexEdit1.GetHexProp();
	tHexProp	hexProp2	= hexEdit2.GetHexProp();

	if ((hexProp1.bits != hexProp2.bits) || (hexProp1.columns != hexProp2.columns) || (hexProp1.isBin != hexProp2.isBin))
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
	if ((length1 >= 1024000 * 40) || (length2 >= 1024000 * 40)) {
		::MessageBox(nppData._nppHandle, _T("Currently only files up to 40 MB supported."), _T("Hex-Editor Compare Error"), MB_OK|MB_ICONERROR);
		return;
	}

	LPSTR	buffer1 = (LPSTR)new CHAR[length1];
	LPSTR	buffer2 = (LPSTR)new CHAR[length2];
	::SendMessage(nppData._scintillaMainHandle, SCI_GETTEXT, length1, (LPARAM)buffer1);
	::SendMessage(nppData._scintillaSecondHandle, SCI_GETTEXT, length2, (LPARAM)buffer2);

	/* create memory to store comopare results */
	compare1 = (LPSTR)new CHAR[length1 / hexProp1.bits + hexProp1.columns];
	::ZeroMemory(compare1, length1 / hexProp1.bits + hexProp1.columns);
	compare2 = (LPSTR)new CHAR[length2 / hexProp2.bits + hexProp1.columns];
	::ZeroMemory(compare2, length2 / hexProp2.bits + hexProp1.columns);

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
		if (hexProp1.bits != 1) {
			posTgt = posSrc / hexProp1.bits;
		} else {
			posTgt++;
		}
	}

	if (doMatch == TRUE)
	{
		if (NLMessageBox((HINSTANCE)g_hModule, nppData._nppHandle, _T("MsgBox CompMatch"), MB_OK) == FALSE)
			::MessageBox(nppData._nppHandle, _T("Files Match."), _T("Hex-Editor Compare"), MB_OK);
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
	HMENU	hMenu			= ::GetMenu(nppData._nppHandle);

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
			for (INT nPos = 0; nPos < vMenuInfoFile.size(); nPos++) {
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
			for (INT nPos = 0; nPos < vMenuInfoFile.size(); nPos++) {
				::AppendMenu(hMenuTemp, 
					vMenuInfoFile[nPos].uFlags | (vMenuInfoFile[nPos].uFlags & MF_SEPARATOR ? 0 : MF_STRING),
					vMenuInfoFile[nPos].uID, vMenuInfoFile[nPos].szName);
			}
		}

		/* exchange menus */
		::GetMenuString(hMenu, MENUINDEX_FILE, text, 64, MF_BYPOSITION);
		::DestroyMenu(::GetSubMenu(hMenu, MENUINDEX_FILE));
		::ModifyMenu(hMenu, MENUINDEX_FILE, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hMenuTemp, text);
	}

	/* create own menu for EDIT */
	if (vMenuInfoEdit.size() != 0)
	{
		HMENU	hMenuTemp = ::CreatePopupMenu();

		if (isMenuHex == TRUE)
		{
			BOOL	lastSep = FALSE;
			for (INT nPos = 0; nPos < vMenuInfoEdit.size(); nPos++) {
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
			for (INT nPos = 0; nPos < vMenuInfoEdit.size(); nPos++) {
				::AppendMenu(hMenuTemp, 
					vMenuInfoEdit[nPos].uFlags | (vMenuInfoEdit[nPos].uFlags & MF_SEPARATOR ? 0 : MF_STRING),
					vMenuInfoEdit[nPos].uID, vMenuInfoEdit[nPos].szName);
			}
		}

		/* exchange menus */
		::GetMenuString(hMenu, MENUINDEX_EDIT, text, 64, MF_BYPOSITION);
		::DestroyMenu(::GetSubMenu(hMenu, MENUINDEX_EDIT));
		::ModifyMenu(hMenu, MENUINDEX_EDIT, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hMenuTemp, text);
	}

	/* create own menu for SEARCH */
	if (vMenuInfoSearch.size() != 0)
	{
		HMENU	hMenuTemp = ::CreatePopupMenu();

		if (isMenuHex == TRUE)
		{
			BOOL	lastSep = FALSE;
			for (INT nPos = 0; nPos < vMenuInfoSearch.size(); nPos++) {
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
			for (INT nPos = 0; nPos < vMenuInfoSearch.size(); nPos++) {
				::AppendMenu(hMenuTemp, 
					vMenuInfoSearch[nPos].uFlags | (vMenuInfoSearch[nPos].uFlags & MF_SEPARATOR ? 0 : MF_STRING),
					vMenuInfoSearch[nPos].uID, vMenuInfoSearch[nPos].szName);
			}
		}

		/* exchange menus */
		::GetMenuString(hMenu, MENUINDEX_SEARCH, text, 64, MF_BYPOSITION);
		::DestroyMenu(::GetSubMenu(hMenu, MENUINDEX_SEARCH));
		::ModifyMenu(hMenu, MENUINDEX_SEARCH, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hMenuTemp, text);
	}

	/* create own menu for VIEW */
	if (vMenuInfoView.size() != 0)
	{
		HMENU	hMenuTemp = ::CreatePopupMenu();

		if (isMenuHex == TRUE)
		{
			BOOL	lastSep = FALSE;
			for (INT nPos = 0; nPos < vMenuInfoView.size(); nPos++) {
				switch (vMenuInfoView[nPos].uID) {
					case 0:
					{
						if (vMenuInfoView[nPos].uFlags & MF_SEPARATOR) {
							lastSep = TRUE;
						} else {
							lastSep = FALSE;
						}
						break;
					}
					case IDM_VIEW_FULLSCREENTOGGLE:
					case IDM_VIEW_ALWAYSONTOP:
					case IDM_VIEW_ZOOMIN:
					case IDM_VIEW_ZOOMOUT:
					case IDM_VIEW_ZOOMRESTORE:
					case IDM_VIEW_GOTO_ANOTHER_VIEW:
					case IDM_VIEW_CLONE_TO_ANOTHER_VIEW:
					case IDM_VIEW_SWITCHTO_MAIN:
					case IDM_VIEW_SWITCHTO_SUB:
					{
						if (lastSep == TRUE) {
							::AppendMenu(hMenuTemp, MF_SEPARATOR, nPos, NULL);
						}
						::AppendMenu(hMenuTemp, 
							vMenuInfoView[nPos].uFlags | MF_STRING,
							vMenuInfoView[nPos].uID, vMenuInfoView[nPos].szName);
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
			for (INT nPos = 0; nPos < vMenuInfoView.size(); nPos++) {
				HMENU	hSubMenu = NULL;
				tMenu	testMenu = vMenuInfoView[nPos];
				if (vMenuInfoView[nPos].uFlags & MF_POPUP) {
					hSubMenu = ::CreatePopupMenu();
					for (INT nPosSub = 0; nPosSub < vMenuInfoView[nPos].vSubMenu.size(); nPosSub++) {
						::AppendMenu(hSubMenu, vMenuInfoView[nPos].vSubMenu[nPosSub].uFlags | MF_STRING,
							vMenuInfoView[nPos].vSubMenu[nPosSub].uID, vMenuInfoView[nPos].vSubMenu[nPosSub].szName);
					}
					::AppendMenu(hMenuTemp, MF_POPUP | MF_STRING, (UINT_PTR)hSubMenu, vMenuInfoView[nPos].szName);
				} else {
					::AppendMenu(hMenuTemp, 
						vMenuInfoView[nPos].uFlags | (vMenuInfoView[nPos].uFlags & MF_SEPARATOR ? 0 : MF_STRING),
						vMenuInfoView[nPos].uID, vMenuInfoView[nPos].szName);
				}
			}
		}

		/* exchange menus */
		::GetMenuString(hMenu, MENUINDEX_VIEW, text, 64, MF_BYPOSITION);
		::DestroyMenu(::GetSubMenu(hMenu, MENUINDEX_VIEW));
		::ModifyMenu(hMenu, MENUINDEX_VIEW, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR)hMenuTemp, text);
	}

	/* activate/deactive menu entries */
	::EnableMenuItem(hMenu, MENUINDEX_FORMAT, MF_BYPOSITION | (toHexStyle?MF_GRAYED:MF_ENABLED));
	::EnableMenuItem(hMenu, MENUINDEX_LANGUAGE, MF_BYPOSITION | (toHexStyle?MF_GRAYED:MF_ENABLED));
	::EnableMenuItem(hMenu, MENUINDEX_MACRO, MF_BYPOSITION | (toHexStyle?MF_GRAYED:MF_ENABLED));
	::EnableMenuItem(hMenu, MENUINDEX_RUN, MF_BYPOSITION | (toHexStyle?MF_GRAYED:MF_ENABLED));
	::DrawMenuBar(nppData._nppHandle);
}

void StoreNppMenuInfo(HMENU hMenuItem, vector<tMenu> & vMenuInfo)
{
	tMenu	menuItem;
	UINT	elemCnt	= ::GetMenuItemCount(hMenuItem);

	vMenuInfo.clear();

	for (INT nPos = 0; nPos < elemCnt; nPos++)
	{
		menuItem.uID = ::GetMenuItemID(hMenuItem, nPos);
		menuItem.uFlags	= ::GetMenuState(hMenuItem, nPos, MF_BYPOSITION);

		::GetMenuString(hMenuItem, nPos, menuItem.szName, sizeof(menuItem.szName) / sizeof(TCHAR), MF_BYPOSITION);
		if ((menuItem.uID == 0) || (menuItem.uID == -1))
		{
			HMENU	hSubMenu = ::GetSubMenu(hMenuItem, nPos);
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

	for (INT i = 0; i < vMenuInfo.size() && ((vMenuInfo[i].uFlags != 0) || (vMenuInfo[i].uID != 0)); i++)
	{
		if (vMenuInfo[i].uID == ::GetMenuItemID(hMenuItem, nPos)) {
			vMenuInfo[i].uFlags = ::GetMenuState(hMenuItem, nPos, MF_BYPOSITION);
			nPos++;
		} else if (::GetMenuState(hMenuItem, nPos, MF_BYPOSITION) & MF_SEPARATOR) {
			nPos++;
		}
	}
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
