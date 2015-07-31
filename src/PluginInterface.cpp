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


/* include files */
#include "stdafx.h"
#include "PluginInterface.h"
#include "HexDialog.h"
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


const INT	nbFunc	= 7;


/* for subclassing */
WNDPROC	wndProcNotepad = NULL;

/* informations for notepad */
const char  PLUGIN_NAME[] = "HEX-Editor";

/* current used file */
TCHAR		currentPath[MAX_PATH];
TCHAR		configPath[MAX_PATH];
TCHAR		iniFilePath[MAX_PATH];
UINT		currentSC	= MAIN_VIEW;


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
HexEdit*		_curHexEdit = NULL;


INT				state = 0;

/* get system information */
BOOL	isNotepadCreated	= FALSE;

/* menu params */
bool	viewHex				= false;

/* supported extensions of hexedit */
char	pszExtensions[256];


/* dialog params */
RECT	rcDlg		 = {0, 0, 0, 0};

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
	g_hModule = hModule;

    switch (reasonForCall)
    {
		case DLL_PROCESS_ATTACH:
		{
			TCHAR	nppPath[MAX_PATH];

			GetModuleFileName((HMODULE)hModule, nppPath, sizeof(nppPath));
            // remove the module name : get plugins directory path
			PathRemoveFileSpec(nppPath);
 
			// cd .. : get npp executable path
			PathRemoveFileSpec(nppPath);
 
			// Make localConf.xml path
			TCHAR	localConfPath[MAX_PATH];
			_tcscpy(localConfPath, nppPath);
			PathAppend(localConfPath, NPP_LOCAL_XML);
 
			// Test if localConf.xml exist
			if (PathFileExists(localConfPath) == TRUE)
			{
				/* make ini file path if not exist */
				_tcscpy(configPath, nppPath);
				_tcscat(configPath, CONFIG_PATH);
				if (PathFileExists(configPath) == FALSE)
				{
					::CreateDirectory(configPath, NULL);
				}
			}
			else
			{
				ITEMIDLIST *pidl;
				SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
				SHGetPathFromIDList(pidl, configPath);
 
				PathAppend(configPath, NPP);
			}

			_tcscpy(iniFilePath, configPath);
			_tcscat(iniFilePath, HEXEDIT_INI);
			if (PathFileExists(iniFilePath) == FALSE)
			{
				::CloseHandle(::CreateFile(iniFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
			}

			/* Set function pointers */
			funcItem[0]._pFunc = toggleHexEdit;
			funcItem[1]._pFunc = toggleHexEdit;			/* ------- */
			funcItem[2]._pFunc = insertColumnsDlg;
			funcItem[3]._pFunc = replacePatternDlg;
			funcItem[4]._pFunc = replacePatternDlg;		/* ------- */
			funcItem[5]._pFunc = openPropDlg;
			funcItem[6]._pFunc = openHelpDlg;
			
			/* Fill menu names */
			strcpy(funcItem[0]._itemName, "View in HEX");
			strcpy(funcItem[1]._itemName, "-----------------");
			strcpy(funcItem[2]._itemName, "Insert Columns...");
			strcpy(funcItem[3]._itemName, "Pattern Replace...");
			strcpy(funcItem[4]._itemName, "-----------------");
			strcpy(funcItem[5]._itemName, "Options...");
			strcpy(funcItem[6]._itemName, "Help...");

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

			prop.hex.columns  = ::GetPrivateProfileInt(dlgEditor, columns, 16, iniFilePath);
			prop.hex.bits	  = ::GetPrivateProfileInt(dlgEditor, bits, HEX_BYTE, iniFilePath);
			prop.hex.isBin	  = ::GetPrivateProfileInt(dlgEditor, bin, FALSE, iniFilePath);
			prop.hex.isLittle = ::GetPrivateProfileInt(dlgEditor, little, FALSE, iniFilePath);
			prop.isCapital	  = ::GetPrivateProfileInt(dlgEditor, capital, FALSE, iniFilePath);
			::GetPrivateProfileString(dlgEditor, extensions, "", pszExtensions, 256, iniFilePath);

			g_hFindRepDlg     = NULL;
			memset(&g_clipboard, 0, sizeof(tClipboard));
			break;
		}	
		case DLL_PROCESS_DETACH:
		{
			char	temp[64];

			::WritePrivateProfileString(dlgEditor, columns, itoa(prop.hex.columns, temp, 10), iniFilePath);
			::WritePrivateProfileString(dlgEditor, bits, itoa(prop.hex.bits, temp, 10), iniFilePath);
			::WritePrivateProfileString(dlgEditor, bin, itoa(prop.hex.isBin, temp, 10), iniFilePath);
			::WritePrivateProfileString(dlgEditor, little, itoa(prop.hex.isLittle, temp, 10), iniFilePath);
			::WritePrivateProfileString(dlgEditor, capital, itoa(prop.isCapital, temp, 10), iniFilePath);
			::WritePrivateProfileString(dlgEditor, extensions, pszExtensions, iniFilePath);

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

	/* initial dialogs */
	hexEdit1.init((HINSTANCE)g_hModule, nppData, iniFilePath);
	hexEdit2.init((HINSTANCE)g_hModule, nppData, iniFilePath);
	hexEdit1.SetParentNppHandle(nppData._scintillaMainHandle);
	hexEdit2.SetParentNppHandle(nppData._scintillaSecondHandle);
	findRepDlg.init((HINSTANCE)g_hModule, nppData);
	propDlg.init((HINSTANCE)g_hModule, nppData);
	gotoDlg.init((HINSTANCE)g_hModule, nppData, iniFilePath);
	patDlg.init((HINSTANCE)g_hModule, nppData);
	helpDlg.init((HINSTANCE)g_hModule, nppData);

	/* Subclassing for Notepad */
	wndProcNotepad = (WNDPROC)SetWindowLong(nppData._nppHandle, GWL_WNDPROC, (LPARAM)SubWndProcNotepad);

	_curHexEdit = &hexEdit1;
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
	if ((isNotepadCreated == TRUE) &&
		((notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle) ||
		 (notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle)))
	{
		SystemUpdate();

		switch (notifyCode->nmhdr.code)
		{
			case SCN_MODIFIED:
				if (notifyCode->modificationType & SC_MOD_INSERTTEXT ||
					notifyCode->modificationType & SC_MOD_DELETETEXT) 
				{
					tHexProp	prop1 = hexEdit1.GetHexProp();
					tHexProp	prop2 = hexEdit2.GetHexProp();

					if ((prop1.pszFileName != NULL) && (prop2.pszFileName != NULL) &&
						(strcmp(prop1.pszFileName, prop2.pszFileName) == 0))
					{
						hexEdit1.TestLineLength();
						hexEdit2.TestLineLength();

						/* redo undo the code */
						if (notifyCode->modificationType & SC_PERFORMED_UNDO)
						{
							hexEdit1.RedoUndo(notifyCode->position);
							hexEdit2.RedoUndo(notifyCode->position);
						}
						else if (notifyCode->modificationType & SC_PERFORMED_REDO)
						{
							hexEdit1.RedoUndo(notifyCode->position+notifyCode->length);
							hexEdit2.RedoUndo(notifyCode->position+notifyCode->length);
						}
					}
					else
					{
						/* test for line length */
						_curHexEdit->TestLineLength();

						/* redo undo the code */
						if (notifyCode->modificationType & SC_PERFORMED_UNDO)
						{
							_curHexEdit->RedoUndo(notifyCode->position);
						}
						else if (notifyCode->modificationType & SC_PERFORMED_REDO)
						{
							_curHexEdit->RedoUndo(notifyCode->position+notifyCode->length);
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
		if (notifyCode->nmhdr.code == NPPN_TB_MODIFICATION)
		{
			g_TBHex.hToolbarBmp = (HBITMAP)::LoadImage((HINSTANCE)g_hModule, MAKEINTRESOURCE(IDB_TB_HEX), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS));
			::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[0]._cmdID, (LPARAM)&g_TBHex);
		}
		if (notifyCode->nmhdr.code == NPPN_READY)
		{
			isNotepadCreated = TRUE;
		}
	}
}

/***
 *	messageProc()
 *
 *	This function is called, if a notification from Notepad occurs
 */
extern "C" __declspec(dllexport) void messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_CREATE)
	{
		setMenu();
	}
}

/***
 *	setHexMask()
 *
 *	Set correct hexMask table
 */
void setHexMask(void)
{
	memcpy(&hexMask, &hexMaskNorm, 256 * 3);

	if (prop.isCapital == TRUE)
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
	tHexProp	prop	= _curHexEdit->GetHexProp();

	::ModifyMenu(hMenu, funcItem[1]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
	::ModifyMenu(hMenu, funcItem[4]._cmdID, MF_BYCOMMAND | MF_SEPARATOR, 0, 0);
	::EnableMenuItem(hMenu, funcItem[2]._cmdID, MF_BYCOMMAND | (prop.isVisible?0:MF_GRAYED));
	::EnableMenuItem(hMenu, funcItem[3]._cmdID, MF_BYCOMMAND | (prop.isVisible?0:MF_GRAYED));
}

/***
 *	checkMenu()
 *
 *	Check menu item of HEX
 */
void checkMenu(BOOL state)
{
	::SendMessage(nppData._nppHandle, NPPM_PIMENU_CHECK, (WPARAM)funcItem[0]._cmdID, (LPARAM)state);
}

/***
  * getProp(), getCLM()
  *
  *	API to get the current properties
 */
tHexProp getProp(void)
{
	return prop.hex;
}

BOOL getCLM(void)
{
	return prop.isCapital;
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
	return ScintillaMsg(message, wParam, lParam);
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
	_curHexEdit->doDialog(TRUE);
	DialogUpdate();
	setMenu();
}

void insertColumnsDlg(void)
{
	patDlg.insertColumns(_curHexEdit->getHSelf());
}

void replacePatternDlg(void)
{
	patDlg.patternReplace(_curHexEdit->getHSelf());
}

void openPropDlg(void)
{
	if (propDlg.doDialog(&prop) == IDOK)
	{
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

			if (_curHexEdit->isVisible() == true)
			{
				switch (LOWORD(wParam))
				{
					case IDM_EDIT_CUT:
						_curHexEdit->Cut();
						return TRUE;
					case IDM_EDIT_COPY:
						_curHexEdit->Copy();
						return TRUE;
					case IDM_EDIT_PASTE:
						_curHexEdit->Paste();
						return TRUE;
					case IDM_VIEW_ZOOMIN:
						_curHexEdit->ZoomIn();
						return TRUE;
					case IDM_VIEW_ZOOMOUT:
						_curHexEdit->ZoomOut();
						return TRUE;
					case IDM_VIEW_ZOOMRESTORE:
						_curHexEdit->ZoomRestore();
						return TRUE;
					case IDM_SEARCH_FIND:
					{
						findRepDlg.doDialog(_curHexEdit->getHSelf(), FALSE);
						return TRUE;
					}
					case IDM_SEARCH_REPLACE:
					{
						findRepDlg.doDialog(_curHexEdit->getHSelf(), TRUE);
						return TRUE;
					}
					case IDM_SEARCH_FINDNEXT:
					{
						findRepDlg.findNext(_curHexEdit->getHSelf());
						return TRUE;
					}
					case IDM_SEARCH_FINDPREV:
					{
						findRepDlg.findPrev(_curHexEdit->getHSelf());
						return TRUE;
					}
					case IDM_SEARCH_GOTOLINE:
					{
						gotoDlg.doDialog(_curHexEdit->getHSelf());
						return TRUE;
					}
					default:
						break;
				}
			}

			switch (LOWORD(wParam))
			{
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
						_curHexEdit->ResetModificationState();
					}
					break;
				}
				case IDM_FILE_SAVEAS: 
				{
					char oldPath[MAX_PATH];
					char newPath[MAX_PATH];

					::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)oldPath);
					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)newPath);
					_curHexEdit->FileNameChanged(newPath);
					if (::SendMessage(g_HSource, SCI_GETMODIFY, 0, 0) == 0)
					{
						_curHexEdit->ResetModificationState();
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
					_curHexEdit->doDialog();
					break;
				}
				case IDC_DOC_GOTO_ANOTHER_VIEW:
				case IDC_DOC_CLONE_TO_ANOTHER_VIEW:
				{
					tHexProp	prop = _curHexEdit->GetHexProp();

					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					SystemUpdate();

					if (_curHexEdit == &hexEdit1)
					{
//						hexEdit2.doDialog();
						hexEdit1.SetHexProp(prop);
						hexEdit1.doDialog();
					}
					else
					{
//						hexEdit1.doDialog();
						hexEdit2.SetHexProp(prop);
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
		case WM_NOTIFY:
		{
			SCNotification *notifyCode = (SCNotification *)lParam;

			if (((notifyCode->nmhdr.hwndFrom == nppData._scintillaMainHandle) ||
				(notifyCode->nmhdr.hwndFrom == nppData._scintillaSecondHandle)) &&
				(notifyCode->nmhdr.code == SCN_SAVEPOINTREACHED))
			{
				SystemUpdate();
				if (TRUE != _curHexEdit->GetModificationState())
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
//					_curHexEdit->doDialog();
					break;
				}
				case TCN_TABDROPPED:
				case TCN_TABDROPPEDOUTSIDE:
				{
					tHexProp	prop = _curHexEdit->GetHexProp();

					ret = ::CallWindowProc(wndProcNotepad, hWnd, message, wParam, lParam);
					SystemUpdate();

					if (_curHexEdit == &hexEdit1)
					{
						hexEdit2.doDialog();
						hexEdit1.SetHexProp(prop);
						hexEdit1.doDialog();
					}
					else
					{
						hexEdit1.doDialog();
						hexEdit2.SetHexProp(prop);
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

	UINT		oldSC	= currentSC;
	char		pszNewPath[MAX_PATH];

	/* update open files */
	UpdateCurrentHScintilla();
	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)pszNewPath);

	if ((strcmp(pszNewPath, currentPath) != 0) || (oldSC != currentSC))
	{
		/* set new file */
		strcpy(currentPath, pszNewPath);

		INT			i = 0;
		INT			docCnt1;
		INT			docCnt2;
		const char	**fileNames1;
		const char	**fileNames2;
		
		/* update doc information */
		docCnt1		= (INT)::SendMessage(nppData._nppHandle, NPPM_NBOPENFILES, 0, (LPARAM)PRIMARY_VIEW);
		docCnt2		= (INT)::SendMessage(nppData._nppHandle, NPPM_NBOPENFILES, 0, (LPARAM)SECOND_VIEW);
		fileNames1	= (const char **)new char*[docCnt1];
		fileNames2	= (const char **)new char*[docCnt2];

		for (i = 0; i < docCnt1; i++)
			fileNames1[i] = (char*)new char[MAX_PATH];
		for (i = 0; i < docCnt2; i++)
			fileNames2[i] = (char*)new char[MAX_PATH];

		if (::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMES_PRIMARY, (WPARAM)fileNames1, (LPARAM)docCnt1))
		{
			INT openDoc1 = (INT)::SendMessage(nppData._nppHandle, NPPM_GETCURRENTDOCINDEX, 0, MAIN_VIEW);
//      assert(!((currentSC == MAIN_VIEW) && (openDoc1 == -1)));
			hexEdit1.UpdateDocs(fileNames1, docCnt1, openDoc1);
		}

		if (::SendMessage(nppData._nppHandle, NPPM_GETOPENFILENAMES_SECOND, (WPARAM)fileNames2, (LPARAM)docCnt2))
		{
			INT openDoc2 = (INT)::SendMessage(nppData._nppHandle, NPPM_GETCURRENTDOCINDEX, 0, SUB_VIEW);
//      assert(!((currentSC == SUB_VIEW) && (openDoc2 == -1)));
			hexEdit2.UpdateDocs(fileNames2, docCnt2, openDoc2);
		}

		for (i = 0; i < docCnt1; i++)
			delete [] fileNames1[i];
		for (i = 0; i < docCnt2; i++)
			delete [] fileNames2[i];
		delete [] fileNames1;
		delete [] fileNames2;

		/* update edit */
		if (currentSC == MAIN_VIEW)
			_curHexEdit = &hexEdit1;
		else
			_curHexEdit = &hexEdit2;

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
	if ((_curHexEdit->isVisible() == false) && (findRepDlg.isVisible() == true))
	{
		findRepDlg.display(FALSE);
		::SendMessage(nppData._nppHandle, WM_COMMAND, (findRepDlg.isFindReplace() == TRUE)?IDM_SEARCH_REPLACE:IDM_SEARCH_FIND, 0);
	}
	else if (g_hFindRepDlg != NULL)
	{
		if ((_curHexEdit->isVisible() == true) && (::IsWindowVisible(g_hFindRepDlg) == TRUE))
		{
			char	text[5];
			::GetWindowText(g_hFindRepDlg, text, 5);
			findRepDlg.doDialog(_curHexEdit->getHSelf(), (strcmp(text, "Find") != 0)?TRUE:FALSE);
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

	strcpy(TEMP, pszExtensions);

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
	UINT		length  = 0;
	char*		buffer  = NULL;
	tHexProp	prop	= {0};

	/* for buffer UNDO */
	ScintillaMsg(hTarget, SCI_BEGINUNDOACTION);

	prop = _curHexEdit->GetHexProp();

	/* get text of current scintilla */
	length = ScintillaMsg(hSource, SCI_GETTEXTLENGTH) + 1;
	buffer = (char*)new char[length];
	ScintillaMsg(hSource, SCI_GETTEXT, length, (LPARAM)buffer);

	/* convert when property is little */
	if (prop.isLittle == TRUE)
	{
		char *temp  = (char*)new char[length];
		char *pText	= buffer;

		/* i must be unsigned */
		for (UINT i = 0; i < length; i++)
		{
			temp[i] = buffer[i];
		}

		UINT offset = (length-1) % prop.bits;
		UINT max	= (length-1) / prop.bits + 1;

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
				for (SHORT j = 1; j <= prop.bits; j++)
				{
					*pText = temp[prop.bits*i-j];
					pText++;
				}
			}
		}
		*pText = NULL;
		delete [] temp;
	}

	/* set text in target */
	ScintillaMsg(hTarget, SCI_CLEARALL, 0, 0);
	ScintillaMsg(hTarget, SCI_ADDTEXT, length, (LPARAM)buffer);
	ScintillaMsg(hTarget, SCI_ENDUNDOACTION);
	delete [] buffer;
}


eError replaceLittleToBig(HWND hSource, INT startPos, INT lengthOld, INT lengthNew)
{
	tHexProp	prop	= {0};

	UpdateCurrentHScintilla();
	if (currentSC == MAIN_VIEW)
		prop = hexEdit1.GetHexProp();
	else
		prop = hexEdit2.GetHexProp();

	if (prop.isLittle == TRUE)
	{
		if (startPos % prop.bits)
		{
			return E_START;
		}
		if ((lengthOld % prop.bits) || (lengthNew % prop.bits))
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
	if (prop.isLittle == FALSE)
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
			posTarget = posSource - (posSource % prop.bits) + ((prop.bits-1) - (posSource % prop.bits));

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

				if (!((i+1) % prop.bits))
					posSource = startPos + lengthNew - 1;
			}
			else if (i < lengthNew)
			{
				/* new string is longer as the old one */
				ScintillaMsg(SCI_SETCURRENTPOS, posTarget - prop.bits + 1);
				ScintillaMsg(SCI_ADDTEXT, 1, (LPARAM)&text[i]);
				if (!((i+1) % prop.bits))
					posSource += prop.bits;
				posSource--;
			}

			posSource++;
		}

		ScintillaMsg(SCI_ENDUNDOACTION);
	}

	delete [] text;

	return E_OK;
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
	else if	(((::GetMenuState(hMenu, IDM_FORMAT_UTF_8, MF_BYCOMMAND) & MF_CHECKED) != 0) ||
		     ((::GetMenuState(hMenu, IDM_FORMAT_AS_UTF_8, MF_BYCOMMAND) & MF_CHECKED) != 0))
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

