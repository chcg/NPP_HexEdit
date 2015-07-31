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
	if (NLGetText(_hInst, _hParent, "Pattern Replace", _txtCaption, sizeof(_txtCaption)) == 0) {
		strcpy(_txtCaption, "Pattern Replace");
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
	if (NLGetText(_hInst, _hParent, "Insert Columns", _txtCaption, sizeof(_txtCaption)) == 0) {
		strcpy(_txtCaption, "Insert Columns");
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
			NLChangeDialog(_hInst, _nppData._nppHandle, _hSelf, "Pattern");
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
					BOOL	ret = FALSE;
					ScintillaMsg(SCI_BEGINUNDOACTION);
					if (_isReplace == TRUE)
					{
						ret = onReplace();
					}
					else
					{
						ret = onInsert();
					}
					ScintillaMsg(SCI_ENDUNDOACTION);

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
	_pCombo->getText(&_pattern);
	if (_pattern.length == 0)
	{
		if (NLMessageBox(_hInst, _hParent, "MsgBox PatNotSet", MB_OK) == FALSE)
			::MessageBox(_hParent, "Pattern is not set!", "Hex-Editor", MB_OK);
		return FALSE;
	}

	HWND		hSCI;
	tHexProp	prop;
	INT			count = 0;
	INT			lines = 0;
	INT			pos   = 0;
	char		buffer[MAX_PATH];

	/* copy data into scintilla handle (encoded if necessary) */
	hSCI = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
	LittleEndianChange(hSCI, getCurrentHScintilla());

	/* get params of Hex Edit */
	::SendMessage(_hParentHandle, HEXM_GETSETTINGS, 0, (LPARAM)&prop);
	::SendMessage(_hParentHandle, HEXM_GETLINECNT, 0, (LPARAM)&lines);

	/* get count and test if params ok */
	::GetDlgItemText(_hSelf, IDC_EDIT_COUNT, buffer, 16);
	count = atoi(buffer);
	if ((count == 0) || ((count + prop.columns) > (128 / prop.bits)))
	{
		if (NLMessageBox(_hInst, _hParent, "MsgBox MaxColCnt", MB_OK|MB_ICONERROR) == FALSE)
			::MessageBox(_hParent, "Maximum of 128 bytes can be shown in a row.", "Hex-Editor", MB_OK|MB_ICONERROR);
		return FALSE;
	}

	/* get column position and test if exists */
	::GetDlgItemText(_hSelf, IDC_EDIT_COL, buffer, 16);
	pos = atoi(buffer);
	if (pos > prop.columns)
	{
		CHAR	txtMsgBox[MAX_PATH];

		if (NLGetText(_hInst, _hParent, "Pos Between", buffer, sizeof(buffer)) == 0) {
			sprintf(txtMsgBox, "Only column position between 0 and %d possible.", prop.columns);
		} else {
			sprintf(txtMsgBox, buffer, prop.columns);
		}
		::MessageBox(_hParent, txtMsgBox, "Hex-Editor", MB_OK|MB_ICONERROR);
		return FALSE;
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

	/* fill data */
	char*	pattern = new char[patSize+1];
	for (INT i = 0; i < cntPat; i++)
	{
		memcpy(&pattern[i*_pattern.length], _pattern.text, _pattern.length);
	}

	/* extend column width of target */
	::SendMessage(_hParentHandle, HEXM_SETCOLUMNCNT, 0, count + prop.columns);

	/* set pattern in columns */
	cntPat = 0;
	for (i = 0; i < lines; i++)
	{
		ScintillaMsg(hSCI, SCI_SETCURRENTPOS, pos);
		ScintillaMsg(hSCI, SCI_ADDTEXT, prop.bits * count, (LPARAM)&pattern[cntPat]);
		::SendMessage(_hParentHandle, HEXM_UPDATEBKMK, pos, (LPARAM)prop.bits * count);
		pos += (count + prop.columns) * prop.bits;
		cntPat += prop.bits * count;
		if (cntPat >= patSize)
			cntPat = 0;
	}

	/* copy modified text back */
	LittleEndianChange(getCurrentHScintilla(), hSCI);

	/* free allocated space */
	CleanScintillaBuf(hSCI);
	::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSCI);
	delete [] pattern;

	/* add text to combo ;) */
	_pCombo->addText(_pattern);

	return TRUE;
}


BOOL PatternDlg::onReplace(void)
{
	HWND		hSCI;
	tHexProp	prop;
	INT			count  = 0;
	INT			lines  = 0;
	INT			posBeg = 0;
	INT			posEnd = 0;

	/* test if something is selected */
	::SendMessage(_hParentHandle, HEXM_GETSEL, (WPARAM)&posBeg, (LPARAM)&posEnd);
	if (posBeg == posEnd)
	{
		if (NLMessageBox(_hInst, _hParent, "MsgBox SelectSomething", MB_OK) == FALSE)
			::MessageBox(_hParent, "Select something in the text!", "Hex-Editor", MB_OK);
		return FALSE;
	}

	/* test if pattern is set */
	_pCombo->getText(&_pattern);
	if (_pattern.length == 0)
	{
		if (NLMessageBox(_hInst, _hParent, "MsgBox PatNotSet", MB_OK) == FALSE)
			::MessageBox(_hParent, "Pattern is not set!", "Hex-Editor", MB_OK);
		return FALSE;
	}

	/* copy data into scintilla handle (encoded if necessary) */
	hSCI = (HWND)::SendMessage(_hParent, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
	LittleEndianChange(hSCI, getCurrentHScintilla());

	/* get params of Hex Edit */
	::SendMessage(_hParentHandle, HEXM_GETSETTINGS, 0, (LPARAM)&prop);
	::SendMessage(_hParentHandle, HEXM_GETLINECNT, 0, (LPARAM)&lines);

	if (prop.selection != HEX_SEL_BLOCK)
	{
		UINT	tempPosBeg = posBeg;
		UINT	tempPosEnd = posEnd;

		count = abs(posEnd - posBeg);

		if (posEnd < posBeg)
		{
			tempPosBeg = posEnd;
			tempPosEnd = posBeg;
			posBeg = tempPosBeg;
			posEnd = tempPosEnd;
		}

		posEnd = posBeg + _pattern.length;

		/* replace pattern in lines */
		for (INT i = 0; i < count; i += _pattern.length)
		{
			ScintillaMsg(hSCI, SCI_SETSEL, posBeg, posEnd);
			ScintillaMsg(hSCI, SCI_TARGETFROMSELECTION);
			ScintillaMsg(hSCI, SCI_REPLACETARGET, _pattern.length, (LPARAM)_pattern.text);
			posBeg += _pattern.length;
			posEnd += _pattern.length;
		}

		UINT	rest = tempPosBeg + (i * _pattern.length) - tempPosEnd;

		if (rest != 0)
		{
			ScintillaMsg(hSCI, SCI_SETSEL, tempPosEnd - rest, tempPosEnd);
			ScintillaMsg(hSCI, SCI_TARGETFROMSELECTION);
			ScintillaMsg(hSCI, SCI_REPLACETARGET, rest, (LPARAM)_pattern.text);
		}

		if (E_OK != replaceLittleToBig(hSCI, tempPosBeg, count, count))
		{
			LITTLE_REPLEACE_ERROR;

			/* free allocated space */
			CleanScintillaBuf(hSCI);
			::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSCI);

			return FALSE;
		}
	}
	else
	{		
		/* get horizontal and vertical gap size */
		count = abs(prop.anchorPos  - prop.cursorPos);
		lines = abs(prop.anchorItem - prop.cursorItem);

		/* create pattern */
		INT	patSize = 0;
		INT	cntPat	= 0;

		do {
			patSize += _pattern.length;
			cntPat++;
		} while (patSize % count);

		/* fill data */
		char*	pattern = new char[patSize+1];
		for (INT i = 0; i < cntPat; i++)
		{
			memcpy(&pattern[i*_pattern.length], _pattern.text, _pattern.length);
		}

		/* get selection */
		::SendMessage(_hParentHandle, HEXM_GETSEL, (WPARAM)&posBeg, (LPARAM)&posEnd);
		if (posBeg > posEnd)
			posBeg = posEnd;

		posEnd = posBeg + count;

		/* replace pattern in columns */
		cntPat = 0;
		for (i = 0; i <= lines; i++)
		{
			ScintillaMsg(hSCI, SCI_SETSEL, posBeg, posEnd);
			ScintillaMsg(hSCI, SCI_TARGETFROMSELECTION);
			ScintillaMsg(hSCI, SCI_REPLACETARGET, count, (LPARAM)&pattern[cntPat]);
			if (E_OK != replaceLittleToBig(hSCI, posBeg, count, count))
			{
				LITTLE_REPLEACE_ERROR;
				
				/* free allocated space */
				CleanScintillaBuf(hSCI);
				::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSCI);
				delete [] pattern;

				return FALSE;
			}

			posBeg += prop.bits * prop.columns;
			posEnd += prop.bits * prop.columns;
			cntPat += count;
			if (cntPat >= patSize)
				cntPat = 0;
		}
		delete [] pattern;
	}
	/* set cursor position */
	::SendMessage(_hParentHandle, HEXM_SETPOS, 0, posEnd - prop.bits * prop.columns);
	
	/* free allocated space */
	CleanScintillaBuf(hSCI);
	::SendMessage(_hParent, NPPM_DESTROYSCINTILLAHANDLE, 0, (LPARAM)hSCI);

	/* add text to combo ;) */
	_pCombo->addText(_pattern);

	return TRUE;
}

