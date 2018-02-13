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
#include "Common.h"
#include "ModifyMenu.h"
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
TCHAR			cmparePath[MAX_PATH];
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

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID )
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
			memset(&g_clipboard, 0, sizeof(tClipboard));
			break;
		}	
		case DLL_PROCESS_DETACH:
		{
			/* save settings */
			saveSettings();

			hexEdit1.destroy();
			hexEdit2.destroy();
			propDlg.destroy();
			delete funcItem[0]._pShKey;

            ClearMenuStructures();

			if (g_TBHex.hToolbarBmp)
				::DeleteObject(g_TBHex.hToolbarBmp);
			if (g_TBHex.hToolbarIcon)
				::DestroyIcon(g_TBHex.hToolbarIcon);

			/* Remove subclaasing */
			SetWindowLongPtr(nppData._nppHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndProcNotepad));
			break;
		}
		case DLL_THREAD_ATTACH:
			break;
			
		case DLL_THREAD_DETACH:
			hexEdit1.ClearAllCompareResults();
			hexEdit2.ClearAllCompareResults();
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
	wndProcNotepad = reinterpret_cast<WNDPROC>(SetWindowLongPtr(nppData._nppHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SubWndProcNotepad)));

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
//		SystemUpdate();

		switch (notifyCode->nmhdr.code)
		{
			case SCN_MODIFIED:
				if (notifyCode->modificationType & SC_MOD_INSERTTEXT ||
					notifyCode->modificationType & SC_MOD_DELETETEXT) 
				{
					tHexProp	hexProp1	= hexEdit1.GetHexProp();
					tHexProp	hexProp2	= hexEdit2.GetHexProp();

					if ((hexProp1.szFileName != NULL) && (hexProp2.szFileName != NULL) &&
						(_tcscmp(hexProp1.szFileName, hexProp2.szFileName) == 0))
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
				SystemUpdate();
                GetShortCuts(nppData._nppHandle);
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
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM , LPARAM )
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
	::PathRemoveBackslash(configPath);

	/* Test if config path exist, if not create */
	if (::PathFileExists(configPath) == FALSE)
	{
		vector<string>  vPaths;

		*_tcsrchr(configPath, '\\') = NULL;
		do {
			vPaths.push_back(configPath);
			*_tcsrchr(configPath, NULL) = NULL;
		} while (::PathFileExists(configPath) == FALSE);

		for (size_t i = vPaths.size(); i > 0; --i)
		{
			_tcscpy(configPath, vPaths[i-1].c_str());
			::CreateDirectory(configPath, NULL);
		}
	}

	/* init compare file path */
	_tcscpy(cmparePath, configPath);
	*_tcsrchr(cmparePath, '\\') = NULL;
	::PathAppend(cmparePath, COMPARE_PATH);
	if (::PathFileExists(cmparePath) == FALSE)
	{
		::CreateDirectory(cmparePath, NULL);
	}

	/* init INI file path */
	_tcscpy(iniFilePath, configPath);
	_tcscat(iniFilePath, HEXEDIT_INI);
	if (PathFileExists(iniFilePath) == FALSE)
	{
		HANDLE	hFile			= NULL;
#ifdef UNICODE
		UCHAR	szBOM[]			= {(UCHAR)0xFF, (UCHAR)0xFE};
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
	prop.colorProp.rgbBkMkTxt	= ::GetPrivateProfileInt(dlgEditor, rgbBkMkTxt, RGB(0xFF,0xFF,0xFF), iniFilePath);
	prop.colorProp.rgbBkMkBk	= ::GetPrivateProfileInt(dlgEditor, rgbBkMkBk, RGB(0xFF,0x00,0x00), iniFilePath);
	prop.colorProp.rgbCurLine	= ::GetPrivateProfileInt(dlgEditor, rgbCurLine, RGB(0xDF,0xDF,0xDF), iniFilePath);
	::GetPrivateProfileString(dlgEditor, fontname, _T("Courier New"), prop.fontProp.szFontName, 256, iniFilePath);
	prop.fontProp.iFontSizeElem	= ::GetPrivateProfileInt(dlgEditor, fontsize, FONTSIZE_DEFAULT, iniFilePath);
	prop.fontProp.isCapital		= ::GetPrivateProfileInt(dlgEditor, capital, FALSE, iniFilePath);
	prop.fontProp.isBold		= ::GetPrivateProfileInt(dlgEditor, bold, FALSE, iniFilePath);
	prop.fontProp.isItalic		= ::GetPrivateProfileInt(dlgEditor, italic, FALSE, iniFilePath);
	prop.fontProp.isUnderline	= ::GetPrivateProfileInt(dlgEditor, underline, FALSE, iniFilePath);
	prop.fontProp.isFocusRect	= ::GetPrivateProfileInt(dlgEditor, focusRect, FALSE, iniFilePath);
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
	::WritePrivateProfileString(dlgEditor, rgbBkMkTxt, _itot(prop.colorProp.rgbBkMkTxt, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbBkMkBk, _itot(prop.colorProp.rgbBkMkBk, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, rgbCurLine, _itot(prop.colorProp.rgbCurLine, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, fontname, prop.fontProp.szFontName, iniFilePath);
	::WritePrivateProfileString(dlgEditor, fontsize, _itot(prop.fontProp.iFontSizeElem, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, capital, _itot(prop.fontProp.isCapital, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, bold, _itot(prop.fontProp.isBold, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, italic, _itot(prop.fontProp.isItalic, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, underline, _itot(prop.fontProp.isUnderline, temp, 10), iniFilePath);
	::WritePrivateProfileString(dlgEditor, focusRect, _itot(prop.fontProp.isFocusRect, temp, 10), iniFilePath);
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
	HMENU		hMenu	= (HMENU)::SendMessage(nppData._nppHandle, NPPM_INTERNAL_GETMENU, 0, 0);
	tHexProp	hexProp	= pCurHexEdit->GetHexProp();

	::EnableMenuItem(hMenu, funcItem[4]._cmdID, MF_BYCOMMAND | (hexProp.isVisible?0:MF_GRAYED));
	::EnableMenuItem(hMenu, funcItem[5]._cmdID, MF_BYCOMMAND | (hexProp.isVisible?0:MF_GRAYED));

	tHexProp hexProp1	= hexEdit1.GetHexProp();
	tHexProp hexProp2	= hexEdit2.GetHexProp();
	if ((hexProp1.isVisible == TRUE) && (hexProp2.isVisible == TRUE) &&
		(_tcsicmp(hexProp1.szFileName, hexProp2.szFileName) != 0)) {
		::EnableMenuItem(hMenu, funcItem[1]._cmdID, MF_BYCOMMAND);
	} else {
		::EnableMenuItem(hMenu, funcItem[1]._cmdID, MF_BYCOMMAND | MF_GRAYED);
	}

	if (hexProp.pCmpResult != NULL) {
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

BOOL isFocusRect(void)
{
	return prop.fontProp.isFocusRect;
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
		case HEX_COLOR_BKMK_TXT:
			return prop.colorProp.rgbBkMkTxt;
		case HEX_COLOR_BKMK_BK:
			return prop.colorProp.rgbBkMkBk;
		case HEX_COLOR_CUR_LINE:
			return prop.colorProp.rgbCurLine;
	}
	return 0;
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
    GetShortCuts(nppData._nppHandle);
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
	if (pCurHexEdit->GetHexProp().pCmpResult->hFile != NULL) {
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
        case WM_COPYDATA:
        {
			ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
			OutputDebugString(_T("WM_COPYDATA\n"));
			SystemUpdate();
            pCurHexEdit->SetStatusBar();
            break;
        }
		case WM_COMMAND:
		{
			/* necessary for focus change between main and second SCI handle */
			if (HIWORD(wParam) == SCEN_SETFOCUS)
			{
				ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
				OutputDebugString(_T("SCEN_SETFOCUS\n"));
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
					case IDM_EDIT_DELETE:
						pCurHexEdit->Delete();
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
					TCHAR oldPath[MAX_PATH];
					TCHAR newPath[MAX_PATH];

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
					OutputDebugString(_T("IDC_PREV/NEXT_DOC\n"));
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
					OutputDebugString(_T("TO_ANOTHER_VIEW\n"));
					SystemUpdate();
					pCurHexEdit->SetHexProp(hexProp);
					pCurHexEdit->doDialog();
					pCurHexEdit->SetStatusBar();
					break;
				}
				case IDM_VIEW_SWITCHTO_OTHER_VIEW:
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
			OutputDebugString(_T("DOOPEN\n"));
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
				OutputDebugString(_T("SAVEPOINTREACHED\n"));
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
					OutputDebugString(_T("TCN_SELCHANGED\n"));
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
					OutputDebugString(_T("TCN_DROPPED\n"));
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

	OutputDebugString(_T("SystemUpdate\n"));

	UINT		oldSC		= currentSC;
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
				fileNames1[i] = (LPTSTR)new TCHAR[MAX_PATH];
				if (fileNames1[i] == NULL)
					isAllocOk = FALSE;
			}
			for (i = 0; (i < docCnt2) && (isAllocOk == TRUE); i++) {
				fileNames2[i] = (LPTSTR)new TCHAR[MAX_PATH];
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
BOOL IsExtensionRegistered(LPCTSTR file)
{
	BOOL	bRet	= FALSE;

	LPTSTR	TEMP	= (LPTSTR) new TCHAR[MAX_PATH];
	LPTSTR	ptr		= NULL;

	if (TEMP != NULL)
	{
		_tcscpy(TEMP, prop.autoProp.szExtensions);

		ptr = _tcstok(TEMP, _T(" "));
		while (ptr != NULL)
		{
			if (_tcsicmp(&file[_tcslen(file) - _tcslen(ptr)], ptr) == 0)
			{
				bRet = TRUE;
				break;
			}
			ptr = _tcstok(NULL, _T(" "));
		}

		delete [] TEMP;
	}

	return bRet;
}

BOOL IsPercentReached(LPCTSTR file)
{
	BOOL	bRet		= FALSE;
	DWORD	dwPercent	= (DWORD)_ttoi(prop.autoProp.szPercent);

	/* return if value is not between 1 - 99 */
	if ((dwPercent < 1) || (dwPercent >= 100))
		return bRet;

	/* open file if exists */
	HANDLE	hFile	= ::CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

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
	if (posEnd > (INT)lenSrc) {
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

				UINT offsetValue = (lenCpy) % hexProp.bits;
				UINT max	= (lenCpy) / hexProp.bits + 1;

				for (UINT i = 1; i <= max; i++)
				{
					if (i == max)
					{
						for (UINT j = 1; j <= offsetValue; j++)
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
	TCHAR		szFile[MAX_PATH];
	BOOL		doMatch		= TRUE;
	tHexProp	hexProp1	= hexEdit1.GetHexProp();
	tHexProp	hexProp2	= hexEdit2.GetHexProp();
	tCmpResult	cmpResult	= {0};

	if ((hexProp1.bits != hexProp2.bits) || (hexProp1.columns != hexProp2.columns) || (hexProp1.isBin != hexProp2.isBin))
	{
		/* ask which hex edits should be used to have same settings in both */
		CompareDlg	dlg;
		dlg.init((HINSTANCE)g_hModule, nppData);
		if (dlg.doDialog(&hexEdit1, &hexEdit2, currentSC) == IDCANCEL) {
			return;
		}
	}

	/* create file for compare results */
	_tcscpy(cmpResult.szFileName, cmparePath);
	_tcscpy(szFile, ::PathFindFileName(hexEdit1.GetHexProp().szFileName));
	::PathRemoveExtension(szFile);
	::PathAppend(cmpResult.szFileName, szFile);
	_tcscat(cmpResult.szFileName, _T("_"));
	_tcscpy(szFile, ::PathFindFileName(hexEdit2.GetHexProp().szFileName));
	::PathRemoveExtension(szFile);
	_tcscat(cmpResult.szFileName, szFile);
	_tcscat(cmpResult.szFileName, _T(".cmp"));
	cmpResult.hFile = ::CreateFile(cmpResult.szFileName, 
		GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		
	if (cmpResult.hFile != INVALID_HANDLE_VALUE)
	{
		LPSTR	buffer1 = (LPSTR)new CHAR[COMP_BLOCK+1];
		LPSTR	buffer2 = (LPSTR)new CHAR[COMP_BLOCK+1];

		/* get text size to comapre */
		INT		maxLength1 = ScintillaMsg(nppData._scintillaMainHandle, SCI_GETTEXTLENGTH) + 1;
		INT		maxLength2 = ScintillaMsg(nppData._scintillaSecondHandle, SCI_GETTEXTLENGTH) + 1;

		/* get max length */
		INT		curPos		= 0;
		INT		maxLength	= maxLength1;
		INT		minLength	= maxLength1;
        if (maxLength2 > maxLength1) {
			maxLength = maxLength2;
        } else {
			minLength = maxLength2;
        }

		while (curPos < minLength)
		{
		    CHAR	val	    = FALSE;
			INT	    posSrc	= 0;
			INT	    length1	= ((maxLength1 - curPos) > COMP_BLOCK ? COMP_BLOCK : (maxLength1 % COMP_BLOCK));
			INT	    length2	= ((maxLength2 - curPos) > COMP_BLOCK ? COMP_BLOCK : (maxLength2 % COMP_BLOCK));

		    ScintillaGetText(nppData._scintillaMainHandle, buffer1, curPos, length1 - 1);
            ScintillaGetText(nppData._scintillaSecondHandle, buffer2, curPos, length2 - 1);

			while ((posSrc < length1) && (posSrc < length2))
			{
				DWORD	hasWritten	= 0;

				if (buffer1[posSrc] != buffer2[posSrc])
				{
					val		= TRUE;
					doMatch	= FALSE;
				}

				/* increment source buffer */
				posSrc++;

				/* write to file */
				if (hexProp1.bits == 1) {
					::WriteFile(cmpResult.hFile, &val, sizeof(val), &hasWritten, NULL);
                    val	= FALSE;
				} else if ((posSrc % hexProp1.bits) == 0) {
					::WriteFile(cmpResult.hFile, &val, sizeof(val), &hasWritten, NULL);
                    val	= FALSE;
				}
			}

			/* increment file position */
			curPos += posSrc;
		}

		if (doMatch == TRUE)
		{
			if (NLMessageBox((HINSTANCE)g_hModule, nppData._nppHandle, _T("MsgBox CompMatch"), MB_OK) == FALSE)
				::MessageBox(nppData._nppHandle, _T("Files Match."), _T("Hex-Editor Compare"), MB_OK);
			::CloseHandle(cmpResult.hFile);
			::DeleteFile(cmpResult.szFileName);
		}
		else
		{
			DWORD	hasWritten	= 0;
			CHAR    val		    = TRUE;

            for (INT i = (minLength / hexProp1.bits); i < (maxLength / hexProp1.bits); i++) {
			    ::WriteFile(cmpResult.hFile, &val, sizeof(val), &hasWritten, NULL);
            }

            /* create two structures for each view */
			tCmpResult* pCmpResult1 = (tCmpResult*)new tCmpResult;
			tCmpResult* pCmpResult2 = (tCmpResult*)new tCmpResult;

            if ((pCmpResult1 != NULL) && (pCmpResult2 != NULL))
            {
                /* set data */
			    *pCmpResult1 = cmpResult;
			    *pCmpResult2 = cmpResult;

                hexEdit1.SetCompareResult(pCmpResult1, pCmpResult2);
                hexEdit2.SetCompareResult(pCmpResult2, pCmpResult1);
            }
            else
            {
		        delete pCmpResult1;
		        delete pCmpResult2;
            }
		}

		delete [] buffer1;
		delete [] buffer2;
	}
}


/**************************************************************************
 *	Other functions
 */
eNppCoding GetNppEncoding(void)
{
	eNppCoding	ret		= HEX_CODE_NPP_ASCI;
	HMENU		hMenu	= (HMENU)::SendMessage(nppData._nppHandle, NPPM_INTERNAL_GETMENU, 0, 0);

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
