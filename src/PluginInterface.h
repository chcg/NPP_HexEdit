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


/**	Version Management for Notepad++ **/
#define		_DEBUG_
/** End **/


#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H



#define TITLETIP_CLASSNAME "MyToolTip"

#include "PluginInterface.h"
#include <windows.h>
#include <TCHAR.h>


/* notepad definitions */


const TCHAR dlgEditor[]		= _T("HEX-Editor");
const TCHAR columns[]		= _T("Columns");
const TCHAR bits[]			= _T("Bits");
const TCHAR bin[]			= _T("Binary");
const TCHAR little[]		= _T("Little");
const TCHAR capital[]		= _T("Capitel");
const TCHAR gotoProp[]		= _T("GotoIsHex");
const TCHAR extensions[]	= _T("Extensions");


const TCHAR HEXEDIT_INI[]	= _T("\\HexEditor.ini");
const TCHAR CONFIG_PATH[]	= _T("\\plugins\\Config");
const TCHAR NPP[]			= _T("\\Notepad++");
const TCHAR NPP_LOCAL_XML[]	= _T("\\doLocalConf.xml");




#ifdef	_DEBUG_
    static char cDBG[256];

	#define DEBUG(x)            ::MessageBox(NULL, x, "DEBUG", MB_OK)
	#define DEBUG_VAL(x)        itoa(x,cDBG,10);DEBUG(cDBG)
	#define DEBUG_VAL_INFO(x,y) sprintf(cDBG, "%s: %d", x, y);DEBUG(cDBG)
	#define DEBUG_BRACE(x)      ScintillaMsg(SCI_SETSEL, x, x);DEBUG("Brace on position:")
	#define DEBUG_STRING(x,y)   ScintillaMsg(SCI_SETSEL, x, y);DEBUG("Selection:")
#else
	#define DEBUG(x)
	#define DEBUG_VAL(x)
	#define DEBUG_VAL_INFO(x,y)
	#define DEBUG_BRACE(x)
#endif




#define WIN32_LEAN_AND_MEAN
#include "Scintilla.h"
#include "rcNotepad.h"


#define NOTEPADPLUS_USER   (WM_USER + 1000)
	#define WM_GETCURRENTSCINTILLA		(NOTEPADPLUS_USER + 4)
	#define WM_GETCURRENTLANGTYPE		(NOTEPADPLUS_USER + 5)
	#define WM_SETCURRENTLANGTYPE		(NOTEPADPLUS_USER + 6)

	#define WM_NBOPENFILES				(NOTEPADPLUS_USER + 7)
		#define ALL_OPEN_FILES			0
		#define PRIMARY_VIEW			1
		#define SECOND_VIEW				2

	#define WM_GETOPENFILENAMES			(NOTEPADPLUS_USER + 8)
	#define WM_CANCEL_SCINTILLAKEY		(NOTEPADPLUS_USER + 9)
	#define WM_BIND_SCINTILLAKEY		(NOTEPADPLUS_USER + 10)
	#define WM_SCINTILLAKEY_MODIFIED	(NOTEPADPLUS_USER + 11)

	#define WM_MODELESSDIALOG			(NOTEPADPLUS_USER + 12)
		#define MODELESSDIALOGADD		0
		#define MODELESSDIALOGREMOVE	1

	#define WM_NBSESSIONFILES			(NOTEPADPLUS_USER + 13)
	#define WM_GETSESSIONFILES			(NOTEPADPLUS_USER + 14)
	#define WM_SAVESESSION				(NOTEPADPLUS_USER + 15)
	#define WM_SAVECURRENTSESSION		(NOTEPADPLUS_USER + 16)

		struct sessionInfo {
			char* sessionFilePathName;
			int nbFile;
			char** files;
		};

	#define WM_GETOPENFILENAMES_PRIMARY (NOTEPADPLUS_USER + 17)
	#define WM_GETOPENFILENAMES_SECOND	(NOTEPADPLUS_USER + 18)
	#define WM_GETPARENTOF				(NOTEPADPLUS_USER + 19)
	#define WM_CREATESCINTILLAHANDLE	(NOTEPADPLUS_USER + 20)
	#define WM_DESTROYSCINTILLAHANDLE	(NOTEPADPLUS_USER + 21)
	#define WM_GETNBUSERLANG			(NOTEPADPLUS_USER + 22)

	#define WM_GETCURRENTDOCINDEX		(NOTEPADPLUS_USER + 23)
		#define MAIN_VIEW 0
		#define SUB_VIEW 1

	#define WM_SETSTATUSBAR				(NOTEPADPLUS_USER + 24)
		#define STATUSBAR_DOC_TYPE 0
		#define STATUSBAR_DOC_SIZE 1
		#define STATUSBAR_CUR_POS 2
		#define STATUSBAR_EOF_FORMAT 3
		#define STATUSBAR_UNICODE_TYPE 4
		#define STATUSBAR_TYPING_MODE 5

	#define WM_GETMENUHANDLE			(NOTEPADPLUS_USER + 25)
		#define NPPPLUGINMENU 0

	#define WM_ENCODE_SCI				(NOTEPADPLUS_USER + 26)
	//ascii file to unicode
	//int WM_ENCODE_SCI(MAIN_VIEW/SUB_VIEW, 0)
	//return new unicodeMode

	#define WM_DECODE_SCI				(NOTEPADPLUS_USER + 27)
	//unicode file to ascii
	//int WM_DECODE_SCI(MAIN_VIEW/SUB_VIEW, 0)
	//return old unicodeMode

	#define WM_ACTIVATE_DOC				(NOTEPADPLUS_USER + 28)
	//void WM_ACTIVATE_DOC(int index2Activate, int view)

	#define WM_LAUNCH_FINDINFILESDLG	(NOTEPADPLUS_USER + 29)
	//void WM_LAUNCH_FINDINFILESDLG(char * dir2Search, char * filtre)

	#define WM_DMM_SHOW					(NOTEPADPLUS_USER + 30)
	#define WM_DMM_HIDE					(NOTEPADPLUS_USER + 31)
	#define WM_DMM_UPDATEDISPINFO		(NOTEPADPLUS_USER + 32)
	//void WM_DMM_xxx(0, tTbData->hClient)

	#define WM_DMM_REGASDCKDLG			(NOTEPADPLUS_USER + 33)
	//void WM_DMM_REGASDCKDLG(0, &tTbData)

	#define WM_LOADSESSION				(NOTEPADPLUS_USER + 34)
	//void WM_LOADSESSION(0, const char* file name)

	#define WM_DMM_VIEWOTHERTAB			(NOTEPADPLUS_USER + 35)
	//void WM_DMM_VIEWOTHERTAB(0, tTbData->hClient)

	#define WM_RELOADFILE				(NOTEPADPLUS_USER + 36)
	//BOOL WM_RELOADFILE(BOOL withAlert, char *filePathName2Reload)

	#define WM_SWITCHTOFILE				(NOTEPADPLUS_USER + 37)
	//BOOL WM_SWITCHTOFILE(0, char *filePathName2switch)

	#define WM_SAVECURRENTFILE			(NOTEPADPLUS_USER + 38)
	//BOOL WM_SWITCHCURRENTFILE(0, 0)

	#define WM_SAVEALLFILES				(NOTEPADPLUS_USER + 39)
	//BOOL WM_SAVEALLFILES(0, 0)

	#define WM_PIMENU_CHECK				(NOTEPADPLUS_USER + 40)
	//void WM_PIMENU_CHECK(UINT	funcItem[X]._cmdID, TRUE/FALSE)

	#define WM_ADDTOOLBARICON			(NOTEPADPLUS_USER + 41)
	//void WM_ADDTOOLBARICON(UINT funcItem[X]._cmdID, toolbarIcons icon)
		struct toolbarIcons {
			HBITMAP	hToolbarBmp;
			HICON	hToolbarIcon;
		};


// Notification code
#define NPPN_FIRST 1000
	#define NPPN_READY					(NPPN_FIRST + 1)
	//scnNotification->nmhdr.code = NPPN_READY;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;

	#define NPPN_TB_MODIFICATION		(NPPN_FIRST + 2)
	//scnNotification->nmhdr.code = NPPN_TB_MODIFICATION;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;


#define	RUNCOMMAND_USER    (WM_USER + 3000)
	#define WM_GET_FULLCURRENTPATH		(RUNCOMMAND_USER + FULL_CURRENT_PATH)
	#define WM_GET_CURRENTDIRECTORY		(RUNCOMMAND_USER + CURRENT_DIRECTORY)
	#define WM_GET_FILENAME				(RUNCOMMAND_USER + FILE_NAME)
	#define WM_GET_NAMEPART				(RUNCOMMAND_USER + NAME_PART)
	#define WM_GET_EXTPART				(RUNCOMMAND_USER + EXT_PART)
		#define VAR_NOT_RECOGNIZED		0
		#define FULL_CURRENT_PATH		1
		#define CURRENT_DIRECTORY		2
		#define FILE_NAME				3
		#define NAME_PART				4
		#define EXT_PART				5


#define WM_DOOPEN						(SCINTILLA_USER   + 8)


#define	SC_MAINHANDLE	0
#define SC_SECHANDLE	1


enum LangType {	
	L_TXT,
	L_PHP, 
	L_C, 
	L_CPP, 
	L_CS, 
	L_OBJC, 
	L_JAVA, 
	L_RC, 
	L_HTML, 
	L_XML, 
	L_MAKEFILE, 
	L_PASCAL, 
	L_BATCH, 
	L_INI, 
	L_NFO,
	L_USER, 
	L_ASP, 
	L_SQL, 
	L_VB,
	L_JS, 
	L_CSS, 
	L_PERL, 
	L_PYTHON, 
	L_LUA, 
	L_TEX, 
	L_FORTRAN, 
	L_BASH, 
	L_FLASH, 
	L_NSIS, 
	L_TCL,
    L_LISP,
	L_SCHEME, 
	L_ASM, 
	L_DIFF, 
	L_PROPS, 
	L_PS, 
	L_RUBY, 
	L_SMALLTALK, 
	L_VHDL
};

struct NppData {
	HWND _nppHandle;
	HWND _scintillaMainHandle;
	HWND _scintillaSecondHandle;
};

struct ShortcutKey { 
	bool _isCtrl; 
	bool _isAlt; 
	bool _isShift; 
	unsigned char _key; 
};

const int itemNameMaxLen = 64;
typedef void (__cdecl * PFUNCPLUGINCMD)();
typedef struct {
	char _itemName[itemNameMaxLen];
	PFUNCPLUGINCMD _pFunc;
	int _cmdID;
	bool _init2Check;
	ShortcutKey *_pShKey;
} FuncItem;




/* hex edit global definitions */


#define HEX_BYTE		1
#define HEX_WORD		2
#define HEX_DWORD		4
#define HEX_LONG		8

typedef enum 
{
	HEX_EDIT_HEX,
	HEX_EDIT_ASCII
} eEdit;

typedef enum 
{
	HEX_SEL_NORM,
	HEX_SEL_VERTICAL,
	HEX_SEL_HORIZONTAL,
	HEX_SEL_BLOCK
} eSel;

typedef enum
{
	HEX_LINE_FIRST,
	HEX_LINE_MIDDLE,
	HEX_LINE_LAST
} eLineVis;


#define		COMBO_STR_MAX	1024

typedef struct
{
	INT 	length;
	TCHAR	text[COMBO_STR_MAX];
} tComboInfo;


typedef struct
{	
	char	pszFileName[MAX_PATH];	// identifier of struct
	BOOL	isModified;				// stores the modification state
	BOOL	isVisible;				// is current file visible
	SHORT	columns;				// number of columns
	SHORT	bits;					// number of bits used
	BOOL	isBin;					// shows in binary
	BOOL	isLittle;				// shows in little endian
	eEdit	editType;				// edit in hex or in ascii
	UINT	firstVisRow;			// last selected scroll position

	BOOL	isSel;					// is text selected...
	eSel	selection;				// selection type
	UINT	cursorItem;				// (selection end) / (pos) item
	UINT	cursorSubItem;			// (selection end) / (pos) sub item
	UINT	cursorPos;				// cursor position
	UINT	anchorItem;				// selection start item
	UINT	anchorSubItem;			// selection start sub item
	UINT	anchorPos;				// start position edit position
} tHexProp;


typedef struct
{	
	tHexProp	hex;
	BOOL		isCapital;
} tProp;


typedef struct
{
	char*		text;
	UINT		length;
	eSel		selection;
	UINT		stride;
	UINT		items;
} tClipboard;


typedef enum
{
	HEX_CODE_NPP_ASCI = 0,
	HEX_CODE_NPP_UTF8,
	HEX_CODE_NPP_USCBE,
	HEX_CODE_NPP_USCLE
} eNppCoding;


typedef enum
{
	E_OK		= 0,
	E_START		= -1,
	E_STRIDE	= -2
} eError;

#define LITTLE_REPLEACE_ERROR 																 \
	::MessageBox(_hSelf, "In Little-Endian-Mode the replacing values could only be column\n" \
						 "wise. For example in 16-bit mode the find length could be 2 and\n" \
						 "replace length 8 or other way round.",							 \
						 "HexEdit Error",													 \
						 MB_OK | MB_ICONERROR)

#define LITTLE_DELETE_ERROR 																   \
	::MessageBox(_hSelf, "In Little-Endian-Mode deleting of values could only be column wise.",\
						 "HexEdit Error",													   \
						 MB_OK | MB_ICONERROR)



UINT ScintillaMsg(HWND hWnd, UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
void ScintillaGetText(HWND hWnd, char* text, INT start, INT end);
UINT ScintillaMsg(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
void ScintillaGetText(char* text, INT start, INT end);
void UpdateCurrentHScintilla(void);
HWND getCurrentHScintilla(void);

void setHexMask(void);
void initMenu(void);
void checkMenu(BOOL state);
tHexProp getProp(void);
BOOL getCLM(void);

void toggleHexEdit(void);
void openPropDlg(void);
void insertColumnsDlg(void);
void replacePatternDlg(void);
void openHelpDlg(void);
LRESULT CALLBACK SubWndProcNotepad(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void setMenu(void);
void ActivateWindow(void);
void SystemUpdate(void);
void GetSecondFileName(void);
void DialogUpdate(void);

/* Global Function of HexEdit */
BOOL IsExtensionRegistered(LPCTSTR file);
void ChangeClipboardDataToHex(tClipboard *clipboard);
void LittleEndianChange(HWND hTarget, HWND hSource);
eError replaceLittleToBig(HWND hSource, INT startPos, INT lengthOld, INT lengthNew);

/* Extended Window Funcions */
eNppCoding GetNppEncoding(void);
void ClientToScreen(HWND hWnd, RECT* rect);
void ScreenToClient(HWND hWnd, RECT* rect);


// 4 mandatory functions for a plugins
extern "C" __declspec(dllexport) void setInfo(NppData);
extern "C" __declspec(dllexport) const char * getName();
extern "C" __declspec(dllexport) void beNotified(SCNotification *);
extern "C" __declspec(dllexport) void messageProc(UINT Message, WPARAM wParam, LPARAM lParam);
extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *);


#endif //PLUGININTERFACE_H

