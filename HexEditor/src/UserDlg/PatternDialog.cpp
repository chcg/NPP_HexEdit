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

#include "PatternDialog.h"
#include "PluginInterface.h"
#include "tables.h"


extern char	hexMask[256][3];


void PatternDlg::patternReplace(HWND hHexEdit)
{
	if (NLGetText(_hInst, _hParent, _T("Pattern Replace"), _txtCaption, sizeof(_txtCaption)) == 0) {
		_tcscpy(_txtCaption, _T("Pattern Replace"));
	}

	doDialog(hHexEdit);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_COUNT), SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_EDIT_COUNT), SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_COL), SW_HIDE);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_EDIT_COL), SW_HIDE);
	_isReplace = TRUE;
	display();
}


void PatternDlg::insertColumns(HWND hHexEdit)
{
	if (NLGetText(_hInst, _hParent, _T("Insert Columns"), _txtCaption, sizeof(_txtCaption)) == 0) {
		_tcscpy(_txtCaption, _T("Insert Columns"));
	}

	doDialog(hHexEdit);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_COUNT), SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_EDIT_COUNT), SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_COL), SW_SHOW);
	::ShowWindow(::GetDlgItem(_hSelf, IDC_EDIT_COL), SW_SHOW);
	_isReplace = FALSE;
	display();
}


void PatternDlg::doDialog(HWND hHexEdit)
{
    if (!isCreated())
	{
        create(IDD_PATTERN_DLG);
		goToCenter();
		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)_hSelf);
	}

	_hParentHandle = hHexEdit;
	::SetFocus(::GetDlgItem(_hSelf, IDC_COMBO_PATTERN));
}


BOOL CALLBACK PatternDlg::run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			_pCombo = new MultiTypeCombo;
			_pCombo->init(_hParent, ::GetDlgItem(_hSelf, IDC_COMBO_PATTERN));
			_pCombo->setCodingType(HEX_CODE_HEX);

			 /* change language */
			NLChangeDialog(_hInst, _nppData._nppHandle, _hSelf, _T("Pattern"));
			::SetWindowText(_hSelf, _txtCaption);
			break;
		}
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					display(false);
					break;
				case IDOK:
				{
					BOOL	ret		= FALSE;
					HWND	hSci	= getCurrentHScintilla();

					::SendMessage(hSci, SCI_BEGINUNDOACTION, 0, 0);
					if (_isReplace == TRUE)
					{
						ret = onReplace();
					}
					else
					{
						ret = onInsert();
					}
					::SendMessage(hSci, SCI_ENDUNDOACTION, 0, 0);

					if (ret == TRUE)
					{
						display(false);
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		case WM_DESTROY :
		{
			destroy();
			break;
		}
		default:
			break;
	}
	return FALSE;
}



BOOL PatternDlg::onInsert(void)
{
	BOOL	bRet	= FALSE;

	_pCombo->getText(&_pattern);
	if (_pattern.length == 0)
	{
		if (NLMessageBox(_hInst, _hParent, _T("MsgBox PatNotSet"), MB_OK) == FALSE)
			::MessageBox(_hParent, _T("Pattern is not set!"), _T("Hex-Editor"), MB_OK);
		return bRet;
	}

	tHexProp	prop;
	HWND		hSciPat	= NULL;
	HWND		hSciTgt	= getCurrentHScintilla();
	INT			count	= 0;
	INT			lines	= 0;
	INT			pos		= 0;
	TCHAR		buffer[17];

	/* create pattern container */
	hSciPat = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);

	/* get params of Hex Edit */
	::SendMessage(_hParentHandle, HEXM_GETSETTINGS, 0, (LPARAM)&prop);
	::SendMessage(_hParentHandle, HEXM_GETLINECNT, 0, (LPARAM)&lines);

	/* get count and test if params ok */
	::GetDlgItemText(_hSelf, IDC_EDIT_COUNT, buffer, 16);
	count = _ttoi(buffer);
	if ((count == 0) || ((count + prop.columns) > (128 / prop.bits)))
	{
		if (NLMessageBox(_hInst, _hParent, _T("MsgBox MaxColCnt"), MB_OK|MB_ICONERROR) == FALSE)
			::MessageBox(_hParent, _T("Maximum of 128 bytes can be shown in a row."), _T("Hex-Editor"), MB_OK|MB_ICONERROR);
		return bRet;
	}

	/* get column position and test if exists */
	::GetDlgItemText(_hSelf, IDC_EDIT_COL, buffer, 16);
	pos = _ttoi(buffer);
	if (pos > prop.columns)
	{
		TCHAR	txtMsgBox[MAX_PATH];

		if (NLGetText(_hInst, _hParent, _T("Pos Between"), buffer, sizeof(buffer)) == 0) {
			_stprintf(txtMsgBox, _T("Only column position between 0 and %d possible."), prop.columns);
		} else {
			_stprintf(txtMsgBox, buffer, prop.columns);
		}
		::MessageBox(_hParent, txtMsgBox, _T("Hex-Editor"), MB_OK|MB_ICONERROR);
		return bRet;
	}
	else
	{
		pos *= prop.bits;
	}

	/* Values are ok -> create pattern */
	INT	patSize = 0;
	INT	cntPat	= 0;

	do {
		patSize += _pattern.length;
		cntPat++;
	} while (patSize % (count * prop.bits));

	/* fill data in pattern buffer */
	for (INT i = 0; i < cntPat; i++) {
		::SendMessage(hSciPat, SCI_ADDTEXT, _pattern.length, (LPARAM)_pattern.text);
	}

	/* extend column width of target */
	::SendMessage(_hParentHandle, HEXM_SETCOLUMNCNT, 0, count + prop.columns);

	/* set pattern in columns */
	cntPat = 0;
	for (i = 0; i < lines; i++)
	{
		if (replaceLittleToBig(hSciTgt, hSciPat, cntPat, pos, 0, prop.bits * count) == E_OK)
		{
			::SendMessage(_hParentHandle, HEXM_UPDATEBKMK, pos, (LPARAM)prop.bits * count);
			pos += (count + prop.columns) * prop.bits;
			cntPat += prop.bits * count;
			if (cntPat >= patSize)
				cntPat = 0;
		}
		else
		{
			LITTLE_REPLACE_ERROR;

			/* free allocated space */
			CleanScintillaBuf(hSciPat);
			::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciPat);
			return FALSE;
		}
	}

	/* free allocated space */
	CleanScintillaBuf(hSciPat);
	::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciPat);

	if (bRet == TRUE) {
		/* add text to combo ;) */
		_pCombo->addText(_pattern);
	}
	return TRUE;
}


BOOL PatternDlg::onReplace(void)
{
	tHexProp	prop;
	BOOL		bRet	= TRUE;
	HWND		hSciTgt	= getCurrentHScintilla();
	HWND		hSciPat	= NULL;
	INT			length	= 0;
	INT			lines	= 0;
	INT			posBeg	= 0;
	INT			posEnd	= 0;

	/* test if something is selected */
	::SendMessage(_hParentHandle, HEXM_GETSEL, (WPARAM)&posBeg, (LPARAM)&posEnd);
	if (posBeg == posEnd)
	{
		if (NLMessageBox(_hInst, _hParent, _T("MsgBox SelectSomething"), MB_OK) == FALSE)
			::MessageBox(_hParent, _T("Select something in the text!"), _T("Hex-Editor"), MB_OK);
		return FALSE;
	}

	/* test if pattern is set */
	_pCombo->getText(&_pattern);
	if (_pattern.length == 0)
	{
		if (NLMessageBox(_hInst, _hParent, _T("MsgBox PatNotSet"), MB_OK) == FALSE)
			::MessageBox(_hParent, _T("Pattern is not set!"), _T("Hex-Editor"), MB_OK);
		return FALSE;
	}

	/* create pattern container */
	hSciPat = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);

	/* get params of Hex Edit */
	::SendMessage(_hParentHandle, HEXM_GETSETTINGS, 0, (LPARAM)&prop);
	::SendMessage(_hParentHandle, HEXM_GETLINECNT, 0, (LPARAM)&lines);

	if (prop.selection == HEX_SEL_NORM)
	{
		length = abs(posEnd-posBeg);

		/* fill data in pattern buffer */
		for (INT rest = length; rest > 0; rest -= _pattern.length)
		{
			if (rest >= _pattern.length) {
				::SendMessage(hSciPat, SCI_ADDTEXT, _pattern.length, (LPARAM)_pattern.text);
			} else {
				::SendMessage(hSciPat, SCI_ADDTEXT, rest, (LPARAM)_pattern.text);
			}
		}

		if (E_OK != replaceLittleToBig(hSciTgt, hSciPat, 0, (posBeg < posEnd ? posBeg : posEnd), length, length))
		{
			LITTLE_REPLACE_ERROR;
			bRet = FALSE;
		}
	}
	else
	{
		/* get horizontal and vertical gap size */
		length = abs(prop.anchorPos - prop.cursorPos);
		lines = abs(prop.anchorItem - prop.cursorItem);

		/* create pattern */
		INT	patSize = 0;
		do {
			patSize += _pattern.length;
			::SendMessage(hSciPat, SCI_ADDTEXT, _pattern.length, (LPARAM)_pattern.text);
		} while (patSize % length);

		if (posBeg > posEnd) {
			posBeg = posEnd;
		}

		/* set pattern in columns */
		for (INT i = 0; i <= lines; i++)
		{
			if (replaceLittleToBig(hSciTgt, hSciPat, 0, posBeg, length, length) == E_OK)
			{
				posBeg += prop.bits * prop.columns;
			}
			else
			{
				LITTLE_REPLACE_ERROR;

				/* free allocated space */
				CleanScintillaBuf(hSciPat);
				::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciPat);
				return FALSE;
			}
		}

		/* set cursor position */
		::SendMessage(_hParentHandle, HEXM_SETPOS, 0, posBeg - prop.bits * prop.columns + length);
	}

	/* free allocated space */
	CleanScintillaBuf(hSciPat);
	::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSciPat);

	if (bRet == TRUE) {
		/* add text to combo ;) */
		_pCombo->addText(_pattern);
	}
	return bRet;
}

