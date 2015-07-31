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


#define NPPMSG   (WM_USER + 1000)
	#define NPPM_GETCURRENTSCINTILLA		(NPPMSG + 4)
	#define NPPM_GETCURRENTLANGTYPE			(NPPMSG + 5)
	#define NPPM_SETCURRENTLANGTYPE			(NPPMSG + 6)

	#define NPPM_NBOPENFILES				(NPPMSG + 7)
		#define ALL_OPEN_FILES				0
		#define PRIMARY_VIEW				1
		#define SECOND_VIEW					2

	#define NPPM_GETOPENFILENAMES			(NPPMSG + 8)
	#define NPPM_CANCEL_SCINTILLAKEY		(NPPMSG + 9)
	#define NPPM_BIND_SCINTILLAKEY			(NPPMSG + 10)
	#define NPPM_SCINTILLAKEY_MODIFIED		(NPPMSG + 11)

	#define NPPM_MODELESSDIALOG				(NPPMSG + 12)
		#define MODELESSDIALOGADD			0
		#define MODELESSDIALOGREMOVE		1

	#define NPPM_NBSESSIONFILES				(NPPMSG + 13)
	#define NPPM_GETSESSIONFILES			(NPPMSG + 14)
	#define NPPM_SAVESESSION				(NPPMSG + 15)
	#define NPPM_SAVECURRENTSESSION			(NPPMSG + 16)

		struct sessionInfo {
			char* sessionFilePathName;
			int nbFile;
			char** files;
		};

	#define NPPM_GETOPENFILENAMES_PRIMARY	(NPPMSG + 17)
	#define NPPM_GETOPENFILENAMES_SECOND	(NPPMSG + 18)
	#define NPPM_GETPARENTOF				(NPPMSG + 19)
	#define NPPM_CREATESCINTILLAHANDLE		(NPPMSG + 20)
	#define NPPM_DESTROYSCINTILLAHANDLE		(NPPMSG + 21)
	#define NPPM_GETNBUSERLANG				(NPPMSG + 22)

	#define NPPM_GETCURRENTDOCINDEX			(NPPMSG + 23)
		#define MAIN_VIEW 0
		#define SUB_VIEW 1

	#define NPPM_SETSTATUSBAR				(NPPMSG + 24)
		#define STATUSBAR_DOC_TYPE 0
		#define STATUSBAR_DOC_SIZE 1
		#define STATUSBAR_CUR_POS 2
		#define STATUSBAR_EOF_FORMAT 3
		#define STATUSBAR_UNICODE_TYPE 4
		#define STATUSBAR_TYPING_MODE 5

	#define NPPM_GETMENUHANDLE				(NPPMSG + 25)
		#define NPPPLUGINMENU 0

	#define NPPM_ENCODE_SCI					(NPPMSG + 26)
	//ascii file to unicode
	//int NPPM_ENCODE_SCI(MAIN_VIEW/SUB_VIEW, 0)
	//return new unicodeMode

	#define NPPM_DECODE_SCI					(NPPMSG + 27)
	//unicode file to ascii
	//int NPPM_DECODE_SCI(MAIN_VIEW/SUB_VIEW, 0)
	//return old unicodeMode

	#define NPPM_ACTIVATE_DOC				(NPPMSG + 28)
	//void NPPM_ACTIVATE_DOC(int index2Activate, int view)

	#define NPPM_LAUNCH_FINDINFILESDLG		(NPPMSG + 29)
	//void NPPM_LAUNCH_FINDINFILESDLG(char * dir2Search, char * filtre)

	#define NPPM_DMM_SHOW					(NPPMSG + 30)
	#define NPPM_DMM_HIDE					(NPPMSG + 31)
	#define NPPM_DMM_UPDATEDISPINFO			(NPPMSG + 32)
	//void NPPM_DMM_xxx(0, tTbData->hClient)

	#define NPPM_DMM_REGASDCKDLG			(NPPMSG + 33)
	//void NPPM_DMM_REGASDCKDLG(0, &tTbData)

	#define NPPM_LOADSESSION				(NPPMSG + 34)
	//void NPPM_LOADSESSION(0, const char* file name)

	#define NPPM_DMM_VIEWOTHERTAB			(NPPMSG + 35)
	//void NPPM_DMM_VIEWOTHERTAB(0, tTbData->hClient)

	#define NPPM_RELOADFILE					(NPPMSG + 36)
	//BOOL NPPM_RELOADFILE(BOOL withAlert, char *filePathName2Reload)

	#define NPPM_SWITCHTOFILE				(NPPMSG + 37)
	//BOOL NPPM_SWITCHTOFILE(0, char *filePathName2switch)

	#define NPPM_SAVECURRENTFILE			(NPPMSG + 38)
	//BOOL NPPM_SWITCHCURRENTFILE(0, 0)

	#define NPPM_SAVEALLFILES				(NPPMSG + 39)
	//BOOL NPPM_SAVEALLFILES(0, 0)

	#define NPPM_PIMENU_CHECK				(NPPMSG + 40)
	//void NPPM_PIMENU_CHECK(UINT	funcItem[X]._cmdID, TRUE/FALSE)

	#define NPPM_ADDTOOLBARICON				(NPPMSG + 41)
	//void NPPM_ADDTOOLBARICON(UINT funcItem[X]._cmdID, toolbarIcons icon)
		struct toolbarIcons {
			HBITMAP	hToolbarBmp;
			HICON	hToolbarIcon;
		};

	#define NPPM_GETWINDOWSVERSION			(NPPMSG + 42)
	//winVer NPPM_GETWINDOWSVERSION(0, 0)

	#define NPPM_DMMGETPLUGINHWNDBYNAME		(NPPMSG + 43)
	//HWND NPPM_DMM_GETPLUGINHWNDBYNAME(const char *windowName, const char *moduleName)
	// if moduleName is NULL, then return value is NULL
	// if windowName is NULL, then the first found window handle which matches with the moduleName will be returned
	
	#define NPPM_MAKECURRENTBUFFERDIRTY		(NPPMSG + 44)
	//BOOL NPPM_MAKECURRENTBUFFERDIRTY(0, 0)

	#define NPPM_GETENABLETHEMETEXTUREFUNC	(NPPMSG + 45)
	//BOOL NPPM_GETENABLETHEMETEXTUREFUNC(0, 0)


// Notification code
#define NPPN_FIRST		1000
	// To notify plugins that all the procedures of launchment of notepad++ are done.
	#define NPPN_READY						(NPPN_FIRST + 1)
	//scnNotification->nmhdr.code = NPPN_READY;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;

	// To notify plugins that toolbar icons can be registered
	#define NPPN_TB_MODIFICATION			(NPPN_FIRST + 2)
	//scnNotification->nmhdr.code = NPPN_TB_MODIFICATION;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;

	// To notify plugins that the current file is about to be closed
	#define NPPN_FILEBEFORECLOSE			(NPPN_FIRST + 3)
	//scnNotification->nmhdr.code = NPPN_FILEBEFORECLOSE;
	//scnNotification->nmhdr.hwndFrom = hwndNpp;
	//scnNotification->nmhdr.idFrom = 0;


#define	RUNCOMMAND_USER    (WM_USER + 3000)
	#define NPPM_GETFULLCURRENTPATH		(RUNCOMMAND_USER + FULL_CURRENT_PATH)
	#define NPPM_GETCURRENTDIRECTORY	(RUNCOMMAND_USER + CURRENT_DIRECTORY)
	#define NPPM_GETFILENAME			(RUNCOMMAND_USER + FILE_NAME)
	#define NPPM_GETNAMEPART			(RUNCOMMAND_USER + NAME_PART)
	#define NPPM_GETEXTPART				(RUNCOMMAND_USER + EXT_PART)
	#define NPPM_GETCURRENTWORD			(RUNCOMMAND_USER + CURRENT_WORD)
	#define NPPM_GETNPPDIRECTORY		(RUNCOMMAND_USER + NPP_DIRECTORY)

		#define VAR_NOT_RECOGNIZED 0
		#define FULL_CURRENT_PATH 1
		#define CURRENT_DIRECTORY 2
		#define FILE_NAME 3
		#define NAME_PART 4
		#define EXT_PART 5
		#define CURRENT_WORD 6
		#define NPP_DIRECTORY 7


#define WM_DOOPEN						(SCINTILLA_USER   + 8)


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

