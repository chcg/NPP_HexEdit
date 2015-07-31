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


#ifndef HEX_H
#define HEX_H

#include "Scintilla.h"
#include "Notepad_plus_rc.h"
#include "PluginInterface.h"
#include "NativeLang_def.h"
#include <windows.h>
#include <TCHAR.h>
#include <vector>

using namespace std;


#define TITLETIP_CLASSNAME _T("MyToolTip")


const TCHAR dlgEditor[]		= _T("HEX-Editor");
const TCHAR addWidth[]		= _T("Address Width");
const TCHAR columns[]		= _T("Columns");
const TCHAR bits[]			= _T("Bits");
const TCHAR bin[]			= _T("Binary");
const TCHAR little[]		= _T("Little");
const TCHAR fontname[]		= _T("Font Name");
const TCHAR fontsize[]		= _T("Font Size");
const TCHAR bold[]			= _T("Bold");
const TCHAR italic[]		= _T("Italic");
const TCHAR underline[]		= _T("Underline");
const TCHAR capital[]		= _T("Capitel");
const TCHAR gotoProp[]		= _T("GotoIsHex");
const TCHAR extensions[]	= _T("Extensions");
const TCHAR rgbRegTxt[]		= _T("RGB Regular Text");
const TCHAR rgbRegBk[]		= _T("RGB Regular Background");
const TCHAR rgbSelTxt[]		= _T("RGB Selection Text");
const TCHAR rgbSelBk[]		= _T("RGB Selection Background");
const TCHAR rgbDiffTxt[]	= _T("RGB Difference Text");
const TCHAR rgbDiffBk[]		= _T("RGB Difference Background");
const TCHAR rgbBkMkTxt[]	= _T("RGB Bookmark Text");
const TCHAR rgbBkMkBk[]		= _T("RGB Bookmark Background");
const TCHAR rgbCurLine[]	= _T("RGB Current Line");
const TCHAR percent[]		= _T("Autostart Percent");
const TCHAR focusRect[]		= _T("Focus Rect");

const TCHAR HEXEDIT_INI[]	= _T("\\HexEditor.ini");
const TCHAR COMPARE_PATH[]	= _T("\\Compare");

#define AUTOSTART_MAX	(1024000)
#define FIND_BLOCK		(1024000)
#define CACHE_SIZE		(8192)
#define CACHE_FILL		(CACHE_SIZE * 3 / 4)
#define COMP_BLOCK		(1024000)


#define HEX_BYTE		1
#define HEX_WORD		2
#define HEX_DWORD		4
#define HEX_LONG		8

#define MAX_PATH_UNICODE 780

typedef enum eEdit
{
	HEX_EDIT_HEX,
	HEX_EDIT_ASCII
} eEdit;

typedef enum eSel
{
	HEX_SEL_NORM,
	HEX_SEL_VERTICAL,
	HEX_SEL_HORIZONTAL,
	HEX_SEL_BLOCK
} eSel;

typedef enum eLineVis
{
	HEX_LINE_FIRST,
	HEX_LINE_MIDDLE,
	HEX_LINE_LAST
} eLineVis;

typedef enum eColorType
{
	HEX_COLOR_REG_TXT,
	HEX_COLOR_REG_BK,
	HEX_COLOR_SEL_TXT,
	HEX_COLOR_SEL_BK,
	HEX_COLOR_DIFF_TXT,
	HEX_COLOR_DIFF_BK,
	HEX_COLOR_BKMK_TXT,
	HEX_COLOR_BKMK_BK,
	HEX_COLOR_CUR_LINE
} eColorType;

typedef enum eSelType
{
	HEX_COLOR_REG,
	HEX_COLOR_SEL,
	HEX_COLOR_DIFF
} eSelType;

typedef enum eSelItem
{
	HEX_ITEM_FIRST,
	HEX_ITEM_MIDDLE,
	HEX_ITEM_LAST,
	HEX_ITEM_MIDDLE_FULL
} eSelItem;


#define		COMBO_STR_MAX	1024

typedef struct tComboInfo
{
	INT 				length;
	CHAR				text[COMBO_STR_MAX];
} tComboInfo;

typedef struct tBkMk
{
	LONG				lAddress;				// bookmark address
	UINT				iItem;					// row of bookmark
} tBkMk;

typedef enum eNppCoding
{
	HEX_CODE_NPP_ASCI = 0,
	HEX_CODE_NPP_UTF8,
	HEX_CODE_NPP_UTF8_BOM,
	HEX_CODE_NPP_USCBE,
	HEX_CODE_NPP_USCLE
} eNppCoding;

typedef struct tCmpResult
{
    struct tCmpResult*  pCmpRef;                // compare reference to other view
	TCHAR				szFileName[MAX_PATH];	// file name to compare data
	HANDLE				hFile;					// file handle to compare results
	INT					offCmpCache;			// display cache offset
	INT					lenCmpCache;			// display cache length
	CHAR				cmpCache[CACHE_SIZE];	// display cache
} tCmpResult;

typedef struct tHexProp
{	
	TCHAR				szFileName[MAX_PATH];	// identifier of struct
	eNppCoding			codePage;				// in Npp selected code page
	BOOL				isModified;				// stores the modification state
	BOOL				isVisible;				// is current file visible
	INT					fontZoom;				// view zoom factor
	UINT				addWidth;				// char width of address field
	SHORT				columns;				// number of columns
	SHORT				bits;					// number of bits used
	BOOL				isBin;					// shows in binary
	BOOL				isLittle;				// shows in little endian
	eEdit				editType;				// edit in hex or in ascii
	UINT				firstVisRow;			// last selected scroll position
	vector<tBkMk>		vBookmarks;				// bookmarks of the view

	BOOL				isSel;					// is text selected...
	eSel				selection;				// selection type
	UINT				cursorItem;				// (selection end) / (pos) item
	UINT				cursorSubItem;			// (selection end) / (pos) sub item
	UINT				cursorPos;				// cursor position
	UINT				anchorItem;				// selection start item
	UINT				anchorSubItem;			// selection start sub item
	UINT				anchorPos;				// start position edit position

	tCmpResult*			pCmpResult;				// compare results
} tHexProp;

typedef struct tColor
{
	COLORREF			rgbRegTxt;				// regular text color
	COLORREF			rgbRegBk;				// regular background color
	COLORREF			rgbSelTxt;				// selected text color
	COLORREF			rgbSelBk;				// selected background color
	COLORREF			rgbDiffTxt;				// differnece text color
	COLORREF			rgbDiffBk;				// difference background color
	COLORREF			rgbBkMkTxt;				// bookmark text color
	COLORREF			rgbBkMkBk;				// bookmark background color
	COLORREF			rgbCurLine;				// current line backgound color
} tColor;

typedef struct tAutoStart
{
	TCHAR				szExtensions[MAX_PATH];	// auto association to enable hex view
	TCHAR				szPercent[4];			// autostart of Hex-Edit by % of NULL
} tAutoStart;

typedef struct tFont
{
	BOOL				isCapital;				// hex view in capital letters
	TCHAR				szFontName[128];		// font name of view
	UINT				iFontSizeElem;			// font size element (content is not size!!!)
	BOOL				isBold;					// font is bold
	BOOL				isItalic;				// font is italic
	BOOL				isUnderline;			// font is underline
	BOOL				isFocusRect;			// marker in dump is a focus rect
} tFont;

typedef struct tProp
{	
	tHexProp			hexProp;				// default hex property
	tAutoStart			autoProp;				// autostart settings
	tColor				colorProp;				// color settings
	tFont				fontProp;				// font settings
} tProp;


typedef struct tClipboard
{
	char*		text;
	UINT		length;
	eSel		selection;
	UINT		stride;
	UINT		items;
} tClipboard;

typedef enum eError
{
	E_OK		= 0,
	E_START		= -1,
	E_STRIDE	= -2,
	E_MEMORY	= -3
} eError;

typedef enum UniMode {
	uni8Bit, 
	uniUTF8,
	uni16BE,
	uni16LE,
	uniCookie,
	uniEnd
} UniMode ;

typedef struct tMenu {
	UINT			uID;
	UINT			uFlags;
	TCHAR			szName[128];
	vector<tMenu>	vSubMenu;
} tMenu;

typedef struct tShortCut {
    BOOL            isEnable;
    UINT            uID;
    ShortcutKey     scKey;
} tShortCut;

#define G_FONTSIZE_MAX		10
#define FONTSIZE_DEFAULT	6		// 16
const UINT g_iFontSize[G_FONTSIZE_MAX] = {8, 9, 10, 11, 12, 14, 16, 18, 20, 22};

#define LITTLE_REPLACE_ERROR 																			\
  if (NLMessageBox(_hInst, _hParent, _T("MsgBox ReplError"), MB_ICONERROR | MB_OK) == FALSE)			\
	::MessageBox(_hParent, _T("In Little-Endian-Mode the replacing values could only be column\n")		\
						   _T("wise. For example in 16-bit mode the find length could be 2 and\n")		\
						   _T("replace length 8 or other way round."),									\
						   _T("Hex-Editor Error"),														\
						   MB_OK | MB_ICONERROR)

#define LITTLE_DELETE_ERROR 																			\
  if (NLMessageBox(_hInst, _hParent, _T("MsgBox DelError"), MB_ICONERROR | MB_OK) == FALSE)				\
	::MessageBox(_hParent, _T("In Little-Endian-Mode deleting of values could only be column wise."),	\
						   _T("Hex-Editor Error"),														\
						   MB_OK | MB_ICONERROR)



UINT ScintillaMsg(HWND hWnd, UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
UINT ScintillaGetText(HWND hWnd, char* text, INT start, INT end);
UINT ScintillaMsg(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
UINT ScintillaGetText(char* text, INT start, INT end);
void CleanScintillaBuf(HWND hWnd);
void UpdateCurrentHScintilla(void);
HWND getCurrentHScintilla(void);



void loadSettings(void);
void saveSettings(void);
void setHexMask(void);
void initMenu(void);

void checkMenu(BOOL state);
tHexProp getProp(void);
BOOL getCLM(void);
LPCTSTR getFontName(void);
UINT getFontSize(void);
UINT getFontSizeElem(void);
void setFontSizeElem(UINT iElem);
BOOL isFontBold(void);
BOOL isFontItalic(void);
BOOL isFontUnderline(void);
BOOL isFocusRect(void);
COLORREF getColor(eColorType type);

void toggleHexEdit(void);
void compareHex(void);
void clearCompare(void);
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
void DoCompare(void);

/* Global Function of HexEdit */
BOOL IsExtensionRegistered(LPCTSTR file);
BOOL IsPercentReached(LPCTSTR file);
void ChangeClipboardDataToHex(tClipboard *clipboard);
BOOL LittleEndianChange(HWND hTarget, HWND hSource, LPINT offset, LPINT length);
eError replaceLittleToBig(HWND hTarget, HWND hSource, INT startSrc, INT startTgt, INT lengthOld, INT lengthNew);

/* Extended Window Funcions */
eNppCoding GetNppEncoding(void);
void ClientToScreen(HWND hWnd, RECT* rect);
void ScreenToClient(HWND hWnd, RECT* rect);


#endif //HEX_H

