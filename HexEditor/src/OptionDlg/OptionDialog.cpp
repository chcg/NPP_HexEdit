//this file is part of Function List Plugin for Notepad++
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

#include "OptionDialog.h"
#include "PluginInterface.h"
#include <Commctrl.h>
#include <shlobj.h>
#include <uxtheme.h>

typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);


static char*	szTabNames[32] = {
	"Start Layout",
	"File Associations",
	"Colors",
	"Font"
};

typedef enum {
	PROP_VIEW = 0,
	PROP_EXT,
	PROP_COLOR,
	PROP_FONT,
	PROP_MAX
} ePropStr;

static int CALLBACK EnumFontFamExProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, int FontType, LPARAM lParam)
{
	vector<string> *pvStrFont = (vector<string> *)lParam;
    size_t vectSize = pvStrFont->size();
    if (vectSize == 0)
		pvStrFont->push_back((char *)lpelfe->elfFullName);
    else
    {
        const char *lastFontName = pvStrFont->at(vectSize - 1).c_str();
        if (strcmp(lastFontName, (const char *)lpelfe->elfFullName))
			pvStrFont->push_back((char *)lpelfe->elfFullName);
    } 
	return 1; // I want to get all fonts
};


UINT OptionDlg::doDialog(tProp *prop)
{
	_pProp = prop;
	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_OPTION_DLG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
}


BOOL CALLBACK OptionDlg::run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			TCITEM		item;
			CHAR		text[32];

			goToCenter();

			ETDTProc	EnableDlgTheme = (ETDTProc)::SendMessage(_nppData._nppHandle, NPPM_GETENABLETHEMETEXTUREFUNC, 0, 0);
			if (EnableDlgTheme != NULL)
                EnableDlgTheme(_hSelf, ETDT_ENABLETAB);
			
			/* set tab texts */
			item.mask		= TCIF_TEXT;

			for (UINT i = 0; i < PROP_MAX; i++)
			{
				if (NLGetText(_hInst, _hParent, szTabNames[i], text, sizeof(text)) == FALSE)
					strcpy(text, szTabNames[i]);
				item.pszText	= text;
				item.cchTextMax	= (int)strlen(text);
				::SendDlgItemMessage(_hSelf, IDC_TAB_PROP, TCM_INSERTITEM, i, (LPARAM)&item);
			}
			TabUpdate();

			/* init color combos */
			_ColCmbRegTxt.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_REGTXT_TXT));
			_ColCmbRegBk.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_REGTXT_BK));
			_ColCmbSelTxt.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_SEL_TXT));
			_ColCmbSelBk.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_SEL_BK));
			_ColCmbDiffTxt.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_DIFF_TXT));
			_ColCmbDiffBk.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_DIFF_BK));
			_ColCmbBkMk.init(_hInst, _hParent, ::GetDlgItem(_hSelf, IDC_COMBO_BKMK));

			/* init the font name combo */
			LOGFONT lf;
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfFaceName[0]='\0';

			vector<string>	vFontList;
			vFontList.push_back("");
			HDC hDC = ::GetDC(NULL);
			::EnumFontFamiliesEx(hDC, &lf, (FONTENUMPROC)EnumFontFamExProc, (LPARAM) &vFontList, 0);

			for (size_t i = 0 ; i < vFontList.size(); i++) {
				::SendDlgItemMessage(_hSelf, IDC_COMBO_FONTNAME, CB_ADDSTRING, 0, (LPARAM)vFontList[i].c_str());
			}

			/* init font size combos */
			for (size_t i = 0 ; i < G_FONTSIZE_MAX; i++) {
				sprintf(text, "%d", g_iFontSize[i]);
				::SendDlgItemMessage(_hSelf, IDC_COMBO_FONTSIZE, CB_ADDSTRING, 0, (LPARAM)text);
			}

			SetParams();

			/* change language */
			NLChangeDialog(_hInst, _nppData._nppHandle, _hSelf, "Options");

			break;
		}
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
					if (GetParams() == FALSE)
						return FALSE;
					::EndDialog(_hSelf, IDOK);
					return TRUE;

				case IDCANCEL:
					::EndDialog(_hSelf, IDCANCEL);
					return TRUE;

				case IDC_COMBO_REGTXT_TXT:
					_ColCmbRegTxt.onSelect();
					break;

				case IDC_COMBO_REGTXT_BK:
					_ColCmbRegBk.onSelect();
					break;

				case IDC_COMBO_SEL_TXT:
					_ColCmbSelTxt.onSelect();
					break;

				case IDC_COMBO_SEL_BK:
					_ColCmbSelBk.onSelect();
					break;

				case IDC_COMBO_DIFF_TXT:
					_ColCmbDiffTxt.onSelect();
					break;

				case IDC_COMBO_DIFF_BK:
					_ColCmbDiffBk.onSelect();
					break;

				case IDC_COMBO_BKMK:
					_ColCmbBkMk.onSelect();
					break;

				default:
					return FALSE;
			}
			break;
		}
		case WM_NOTIFY:
		{
			NMHDR	nmhdr = *((LPNMHDR)lParam);
			if ((nmhdr.idFrom == IDC_TAB_PROP) && (nmhdr.code == TCN_SELCHANGE)) {
				TabUpdate();
				return TRUE;
			}
			break;
		}
		default:
			break;
	}
	return FALSE;
}


void OptionDlg::TabUpdate(void)
{
	INT		iSel	= (INT)::SendDlgItemMessage(_hSelf, IDC_TAB_PROP, TCM_GETCURSEL, 0, 0);

	BOOL	bView	= (iSel == PROP_VIEW? SW_SHOW:SW_HIDE);
	BOOL	bExt	= (iSel == PROP_EXT ? SW_SHOW:SW_HIDE);
	BOOL	bColor	= (iSel == PROP_COLOR ? SW_SHOW:SW_HIDE);
	BOOL	bFont	= (iSel == PROP_FONT ? SW_SHOW:SW_HIDE);

	/* set tab visibility state of "View" */
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_8),				bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_16),			bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_32),			bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_64),			bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_BIG),			bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_LITTLE),		bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_HEX),			bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_RADIO_BIN),			bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COLUMN_EDIT),			bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_COLUMN),		bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_ADDWIDTH_EDIT),		bView);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_ADDWIDTH),		bView);

	/* set tab visibility state of "Extension" */
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_EXAMPLE),		bExt);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_EDIT_EXTLIST),		bExt);

	/* set tab visibility state of "Color" */
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_TXT),			bColor);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_BK),			bColor);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_REGTXT),		bColor);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COMBO_REGTXT_TXT),	bColor);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COMBO_REGTXT_BK),		bColor);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_SEL),			bColor);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COMBO_SEL_TXT),		bColor);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COMBO_SEL_BK),		bColor);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_DIFF),			bColor);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COMBO_DIFF_TXT),		bColor);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COMBO_DIFF_BK),		bColor);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_BKMK),			bColor);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COMBO_BKMK),			bColor);

	/* set tab visibility state of "Font" */
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_FONTNAME),		bFont);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COMBO_FONTNAME),		bFont);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_FONTSIZE),		bFont);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_COMBO_FONTSIZE),		bFont);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_CLM),			bFont);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_BOLD),			bFont);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_ITALIC),		bFont);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_UNDERLINE),		bFont);

	if (bExt == TRUE) {
		::SetFocus(::GetDlgItem(_hSelf, IDC_EDIT_EXTLIST));
	} else if (bFont == TRUE) {
		::SetFocus(::GetDlgItem(_hSelf, IDC_COMBO_FONTNAME));
	}
}


void OptionDlg::SetParams(void)
{
	char		text[16];

	/* set default format */
	::SetWindowText(::GetDlgItem(_hSelf, IDC_COLUMN_EDIT), itoa(_pProp->hexProp.columns, text, 10));
	::SetWindowText(::GetDlgItem(_hSelf, IDC_ADDWIDTH_EDIT), itoa(_pProp->hexProp.addWidth, text, 10));
	switch (_pProp->hexProp.bits)
	{
		case HEX_BYTE:
			::SendDlgItemMessage(_hSelf, IDC_RADIO_8, BM_SETCHECK, BST_CHECKED, 0); 
			break;
		case HEX_WORD:
			::SendDlgItemMessage(_hSelf, IDC_RADIO_16, BM_SETCHECK, BST_CHECKED, 0); 
			break;
		case HEX_DWORD:
			::SendDlgItemMessage(_hSelf, IDC_RADIO_32, BM_SETCHECK, BST_CHECKED, 0); 
			break;
		case HEX_LONG:
			::SendDlgItemMessage(_hSelf, IDC_RADIO_64, BM_SETCHECK, BST_CHECKED, 0); 
			break;
		default:
			break;
	}
	if (_pProp->hexProp.isLittle == FALSE) {
		::SendDlgItemMessage(_hSelf, IDC_RADIO_BIG, BM_SETCHECK, BST_CHECKED, 0); 
	} else {
		::SendDlgItemMessage(_hSelf, IDC_RADIO_LITTLE, BM_SETCHECK, BST_CHECKED, 0); 
	}
	if (_pProp->hexProp.isBin == FALSE) {
		::SendDlgItemMessage(_hSelf, IDC_RADIO_HEX, BM_SETCHECK, BST_CHECKED, 0); 
	} else {
		::SendDlgItemMessage(_hSelf, IDC_RADIO_BIN, BM_SETCHECK, BST_CHECKED, 0); 
	}

	/* set extension list */
	::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_EXTLIST), _pProp->szExtensions);

	/* set font information */
	UINT iPos = ::SendDlgItemMessage(_hSelf, IDC_COMBO_FONTNAME, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)_pProp->fontProp.szFontName);
	if (iPos == CB_ERR) iPos = 0;
	::SendDlgItemMessage(_hSelf, IDC_COMBO_FONTNAME, CB_SETCURSEL, iPos, 0);

	::SendDlgItemMessage(_hSelf, IDC_COMBO_FONTSIZE, CB_SETCURSEL, _pProp->fontProp.iFontSizeElem, 0);

	if (_pProp->fontProp.isBold == TRUE) {
		::SendDlgItemMessage(_hSelf, IDC_CHECK_BOLD, BM_SETCHECK, BST_CHECKED, 0); 
	}
	if (_pProp->fontProp.isItalic == TRUE) {
		::SendDlgItemMessage(_hSelf, IDC_CHECK_ITALIC, BM_SETCHECK, BST_CHECKED, 0); 
	}
	if (_pProp->fontProp.isUnderline == TRUE) {
		::SendDlgItemMessage(_hSelf, IDC_CHECK_UNDERLINE, BM_SETCHECK, BST_CHECKED, 0); 
	}
	if (_pProp->fontProp.isCapital == TRUE) {
		::SendDlgItemMessage(_hSelf, IDC_CHECK_CLM, BM_SETCHECK, BST_CHECKED, 0); 
	}

	/* set colors */
	_ColCmbRegTxt.setColor(_pProp->colorProp.rgbRegTxt);
	_ColCmbRegBk.setColor(_pProp->colorProp.rgbRegBk);
	_ColCmbSelTxt.setColor(_pProp->colorProp.rgbSelTxt);
	_ColCmbSelBk.setColor(_pProp->colorProp.rgbSelBk);
	_ColCmbDiffTxt.setColor(_pProp->colorProp.rgbDiffTxt);
	_ColCmbDiffBk.setColor(_pProp->colorProp.rgbDiffBk);
	_ColCmbBkMk.setColor(_pProp->colorProp.rgbBkMk);
}


BOOL OptionDlg::GetParams(void)
{
	char		text[16];
	UINT		col		= 0;
	UINT		add		= 0;
	UINT		bits	= 0;
	BOOL		bRet	= FALSE;

	/* get default layout */
	::GetWindowText(::GetDlgItem(_hSelf, IDC_ADDWIDTH_EDIT), text, 16);
	add = atoi(text);
	::GetWindowText(::GetDlgItem(_hSelf, IDC_COLUMN_EDIT), text, 16);
	col = atoi(text);

	/* get bit alignment */
	if (::SendDlgItemMessage(_hSelf, IDC_RADIO_8, BM_GETCHECK, 0, 0) == BST_CHECKED)
		bits = HEX_BYTE;
	else if (::SendDlgItemMessage(_hSelf, IDC_RADIO_16, BM_GETCHECK, 0, 0) == BST_CHECKED)
		bits = HEX_WORD;
	else if (::SendDlgItemMessage(_hSelf, IDC_RADIO_32, BM_GETCHECK, 0, 0) == BST_CHECKED)
		bits = HEX_DWORD;
	else
		bits = HEX_LONG;

	/* test if values are possible */
	if ((col > 0) && (col <= (128 / bits)))
	{
		bRet = TRUE;
	}
	else
	{
		if (NLMessageBox(_hInst, _hParent, "MsgBox MaxColCnt", MB_OK|MB_ICONERROR) == FALSE)
			::MessageBox(_hParent, "Maximum of 128 bytes can be shown in a row.", "Hex-Editor: Column Count", MB_OK|MB_ICONERROR);
		bRet = FALSE;
	}

	if ((add >= 4) && (add <= 16))
	{
		bRet = TRUE;
	}
	else
	{
		if (NLMessageBox(_hInst, _hParent, "MsgBox MaxAddCnt", MB_OK|MB_ICONERROR) == FALSE)
			::MessageBox(_hParent, "Only values between 4 and 16 possible.", "Hex-Editor: Address Width", MB_OK|MB_ICONERROR);
		bRet = FALSE;
	}

	if (bRet == TRUE) {
		_pProp->hexProp.addWidth	= add;
		_pProp->hexProp.columns		= col;
		_pProp->hexProp.bits		= bits;
		
		/* get endian */
		if (::SendDlgItemMessage(_hSelf, IDC_RADIO_BIG, BM_GETCHECK, 0, 0) == BST_CHECKED)
			_pProp->hexProp.isLittle = FALSE;
		else
			_pProp->hexProp.isLittle = TRUE;

		/* get display form */
		if (::SendDlgItemMessage(_hSelf, IDC_RADIO_HEX, BM_GETCHECK, 0, 0) == BST_CHECKED)
			_pProp->hexProp.isBin    = FALSE;
		else
			_pProp->hexProp.isBin    = TRUE;

		/* get extensions */
		::GetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_EXTLIST), _pProp->szExtensions, sizeof(_pProp->szExtensions));

		/* get font information */
		::GetWindowText(::GetDlgItem(_hSelf, IDC_COMBO_FONTNAME), _pProp->fontProp.szFontName, sizeof(_pProp->fontProp.szFontName));
		_pProp->fontProp.iFontSizeElem	= ::SendDlgItemMessage(_hSelf, IDC_COMBO_FONTSIZE, CB_GETCURSEL, 0, 0);
		_pProp->fontProp.isBold			= (::SendDlgItemMessage(_hSelf, IDC_CHECK_BOLD, BM_GETCHECK, 0, 0) == BST_CHECKED);
		_pProp->fontProp.isItalic		= (::SendDlgItemMessage(_hSelf, IDC_CHECK_ITALIC, BM_GETCHECK, 0, 0) == BST_CHECKED);
		_pProp->fontProp.isUnderline	= (::SendDlgItemMessage(_hSelf, IDC_CHECK_UNDERLINE, BM_GETCHECK, 0, 0) == BST_CHECKED);
		_pProp->fontProp.isCapital		= (::SendDlgItemMessage(_hSelf, IDC_CHECK_CLM, BM_GETCHECK, 0, 0) == BST_CHECKED);

		_ColCmbRegTxt.getColor(&_pProp->colorProp.rgbRegTxt);
		_ColCmbRegBk.getColor(&_pProp->colorProp.rgbRegBk);
		_ColCmbSelTxt.getColor(&_pProp->colorProp.rgbSelTxt);
		_ColCmbSelBk.getColor(&_pProp->colorProp.rgbSelBk);
		_ColCmbDiffTxt.getColor(&_pProp->colorProp.rgbDiffTxt);
		_ColCmbDiffBk.getColor(&_pProp->colorProp.rgbDiffBk);
		_ColCmbBkMk.getColor(&_pProp->colorProp.rgbBkMk);
	}
	else
	{
		::SendDlgItemMessage(_hSelf, IDC_TAB_PROP, TCM_SETCURSEL, PROP_VIEW, NULL);
	}

	return bRet;
}




