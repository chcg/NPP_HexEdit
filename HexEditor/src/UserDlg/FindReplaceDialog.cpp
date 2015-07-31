/*
this file is part of HexEdit Plugin for Notepad++
Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "FindReplaceDialog.h"
#include "PluginInterface.h"
#include "tables.h"
#include <uxtheme.h>


typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);
#define CB_SETMINVISIBLE 0x1701



extern char	hexMask[256][3];


void FindReplaceDlg::doDialog(HWND hParent, BOOL findReplace)
{
    if (!isCreated())
	{
        create(IDD_FINDREPLACE_DLG);
		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)_hSelf);

		/* create new scintilla handle */
		_hSCI = (HWND)::SendMessage(_nppData._nppHandle, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);

			ETDTProc	EnableDlgTheme = (ETDTProc)::SendMessage(_nppData._nppHandle, NPPM_GETENABLETHEMETEXTUREFUNC, 0, 0);
			if (EnableDlgTheme != NULL)
				EnableDlgTheme(_hSelf, ETDT_ENABLETAB);
	}

	/* set kind of dialog */
	_findReplace = findReplace;

	_hParentHandle = hParent;

	/* update dialog */
	display();
	updateDialog();
	::SetFocus(::GetDlgItem(_hSelf, IDC_COMBO_FIND));
}


void FindReplaceDlg::display(bool toShow)
{
	::ShowWindow(_hSelf, toShow?SW_SHOW:SW_HIDE);
}


BOOL CALLBACK FindReplaceDlg::run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
        case WM_INITDIALOG :
		{
			initDialog();
			goToCenter();
			return TRUE;
		}
		case WM_ACTIVATE :
        {
			UINT	posBeg;
			UINT	posEnd;

			::SendMessage(_hParentHandle, HEXM_GETSEL, (WPARAM)&posBeg, (LPARAM)&posEnd);
			::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_IN_SEL), ((posBeg == posEnd)?FALSE:TRUE));
            break;
        }
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDOK :
                {
					onFind();
					return TRUE;
                }
				case IDC_REPLACE:
                {
					onReplace();
					break;
                }
				case IDC_COUNT :
				{
					processAll(COUNT);
					break;
                }
				case IDC_REPLACEALL :
                {
					ScintillaMsg(SCI_BEGINUNDOACTION);
					processAll(REPLACE_ALL);
					ScintillaMsg(SCI_ENDUNDOACTION);
					return TRUE;
                }
				case IDC_COMBO_DATATYPE:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
						changeCoding();
					break;
				}
				case IDC_CHECK_TRANSPARENT :
				{
					setTrans();
					break;
				}
				case IDC_RADIO_DIRUP :
				case IDC_RADIO_DIRDOWN :
				{
					_whichDirection = isChecked(IDC_RADIO_DIRDOWN);
					return TRUE;
				}
				case IDC_CHECK_MATCHCASE :
				{
					_isMatchCase = isChecked(IDC_CHECK_MATCHCASE);
					return TRUE;
				}
				case IDC_CHECK_WRAP :
				{
					_isWrap = isChecked(IDC_CHECK_WRAP);
					return TRUE;
				}
				case IDC_CHECK_IN_SEL :
				{
					_isInSel = isChecked(IDC_CHECK_IN_SEL);
					return TRUE;
				}
				case IDCANCEL :
				{
					display(FALSE);
					::SetFocus(_hParentHandle);
					break;
				}
				default :
					break;
			}
			break;
		}
		case WM_NOTIFY:
		{
			NMHDR	nmhdr = *((LPNMHDR)lParam);

			if ((nmhdr.idFrom == IDC_SWITCH) && (nmhdr.code == TCN_SELCHANGE))
			{
        _findReplace = TabCtrl_GetCurSel(::GetDlgItem(_hSelf, IDC_SWITCH));
				updateDialog();
			}
			break;
		}
		case WM_HSCROLL :
		{
			if ((HWND)lParam == ::GetDlgItem(_hSelf, IDC_SLIDER_PERCENTAGE))
			{
				setTrans();
			}
			return TRUE;
		}
	}
	return FALSE;
}


void FindReplaceDlg::initDialog(void)
{
	TCITEM		item;

	item.mask		= TCIF_TEXT;
	item.pszText	= "Find";
	item.cchTextMax	= (int)strlen(item.pszText);
	::SendDlgItemMessage(_hSelf, IDC_SWITCH, TCM_INSERTITEM, 0, (LPARAM)&item);
	item.pszText	= "Replace";
	item.cchTextMax	= (int)strlen(item.pszText);
	::SendDlgItemMessage(_hSelf, IDC_SWITCH, TCM_INSERTITEM, 1, (LPARAM)&item);

	/* init comboboxes */
	_pFindCombo			= new MultiTypeCombo;
	_pReplaceCombo		= new MultiTypeCombo;

	_pFindCombo->init(::GetDlgItem(_hSelf, IDC_COMBO_FIND));
	_pReplaceCombo->init(::GetDlgItem(_hSelf, IDC_COMBO_REPLACE));

	/* set default coding */
	_currDataType = HEX_CODE_HEX;
	_pFindCombo->setCodingType(_currDataType);
	_pReplaceCombo->setCodingType(_currDataType);

	/* set data types */
	::SendDlgItemMessage(_hSelf, IDC_COMBO_DATATYPE, CB_SETMINVISIBLE, HEX_CODE_MAX, 0);
    ::SendDlgItemMessage(_hSelf, IDC_COMBO_DATATYPE, CB_INSERTSTRING, HEX_CODE_HEX, (LPARAM)strCode[HEX_CODE_HEX]);
    ::SendDlgItemMessage(_hSelf, IDC_COMBO_DATATYPE, CB_INSERTSTRING, HEX_CODE_ASCI, (LPARAM)strCode[HEX_CODE_ASCI]);
    ::SendDlgItemMessage(_hSelf, IDC_COMBO_DATATYPE, CB_INSERTSTRING, HEX_CODE_UNI, (LPARAM)strCode[HEX_CODE_UNI]);
	::SendDlgItemMessage(_hSelf, IDC_COMBO_DATATYPE, CB_SETCURSEL, _currDataType, 0);


	/* set radios */
	::SendDlgItemMessage(_hSelf, IDC_RADIO_DIRDOWN, BM_SETCHECK, BST_CHECKED, 0);
	::SendDlgItemMessage(_hSelf, IDC_RADIO_BOTTOM, BM_SETCHECK, BST_CHECKED, 0);

	/* set wrap */
	::SendDlgItemMessage(_hSelf, IDC_CHECK_WRAP, BM_SETCHECK, BST_CHECKED, 0);

	/* is transparent mode available? */
	HMODULE hUser32 = ::GetModuleHandle("User32");

	if (hUser32)
	{
		/* set transparency */
		::SendDlgItemMessage(_hSelf, IDC_SLIDER_PERCENTAGE, TBM_SETRANGE, FALSE, MAKELONG(20, 225));
		::SendDlgItemMessage(_hSelf, IDC_SLIDER_PERCENTAGE, TBM_SETPOS, TRUE, 200);
		_transFuncAddr = (WNDPROC)GetProcAddress(hUser32, "SetLayeredWindowAttributes");
		setTrans();
	}
	else
	{
		::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_TRANSPARENT), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_SLIDER_PERCENTAGE), SW_HIDE);
	}
	changeCoding();
}


void FindReplaceDlg::updateDialog(void)
{
	if (_findReplace == TRUE)
	{
		::SetWindowText(_hSelf, "Replace");
		::ShowWindow(::GetDlgItem(_hSelf, IDC_COUNT), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACE), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_REPLACE), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_COMBO_REPLACE), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACEALL), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_IN_SEL), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_REPALL), SW_SHOW);
	}
	else
	{
		::SetWindowText(_hSelf, "Find");
		::ShowWindow(::GetDlgItem(_hSelf, IDC_COUNT), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACE), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_REPLACE), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_COMBO_REPLACE), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACEALL), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_IN_SEL), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_REPALL), SW_HIDE);	
	}
  TabCtrl_SetCurSel(::GetDlgItem(_hSelf, IDC_SWITCH), _findReplace);

	/* get selection and set find text */
	UINT		posBeg	= 0;
	UINT		posEnd	= 0;
	tComboInfo	info	= {0};

	/* get selected length */
	::SendMessage(_hParentHandle, HEXM_GETSEL, (WPARAM)&posBeg, (LPARAM)&posEnd);

	/* enable box setting for replace all */
	::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_IN_SEL), ((posBeg == posEnd)?FALSE:TRUE));

	info.length = abs(posEnd-posBeg);

	if (info.length != 0)
	{
		char	*text	= (char*)new char[info.length+1];

		/* convert and select and get the text */
		LittleEndianChange(_hSCI, getCurrentHScintilla());
		ScintillaMsg(_hSCI, SCI_SETSELECTIONSTART, posBeg, 0);
		ScintillaMsg(_hSCI, SCI_SETSELECTIONEND, posEnd, 0);
		ScintillaMsg(_hSCI, SCI_TARGETFROMSELECTION, 0, 0);
		ScintillaMsg(_hSCI, SCI_GETSELTEXT, 0, (LPARAM)text);

		/* encode the text in dependency of selected data type */
		memcpy(info.text, text, info.length);
		_pFindCombo->setText(info);

		delete [] text;
	}
}


void FindReplaceDlg::onFind(void)
{
	UINT		length  = ScintillaMsg(SCI_GETLENGTH);
	UINT		posBeg  = 0;
	UINT		posEnd  = 0;

	_pFindCombo->getText(&_find);

	/* copy data into scintilla handle (encoded if necessary) */
	LittleEndianChange(_hSCI, getCurrentHScintilla());

	/* set start and end position */
	::SendMessage(_hParentHandle, HEXM_GETSEL, (WPARAM)&posBeg, (LPARAM)&posEnd);

	if (_whichDirection == DIR_UP)
	{
		ScintillaMsg(_hSCI, SCI_SETTARGETSTART, posBeg);
		ScintillaMsg(_hSCI, SCI_SETTARGETEND, 0);
	}
	else
	{
		ScintillaMsg(_hSCI, SCI_SETTARGETSTART, posEnd);
		ScintillaMsg(_hSCI, SCI_SETTARGETEND, length);
	}

	ScintillaMsg(_hSCI, SCI_SETSEARCHFLAGS, _isMatchCase ? SCFIND_MATCHCASE : 0, 0);

	/* find string */
	UINT posFind = ScintillaMsg(_hSCI, SCI_SEARCHINTARGET, _find.length, (LPARAM)_find.text);
	if (posFind != -1)
	{
		/* found */
		::SendMessage(_hParentHandle, HEXM_SETSEL, posFind, posFind + _find.length);
		::SendMessage(_hParentHandle, HEXM_ENSURE_VISIBLE, 
					  posFind + (HEX_LINE_FIRST == getDisplayPos()?0:_find.length), getDisplayPos());
		_pFindCombo->addText(_find);
	}
	else if (_isWrap == FALSE)
	{
		/* not found and wrapping mode is off */
		::MessageBox(_hSelf, "Can't find",(_findReplace == TRUE)?"Replace":"Find", MB_OK);
	}
	else
	{
		/* wrapping mode on -> find it again */
		if (_whichDirection == DIR_UP)
		{
			ScintillaMsg(_hSCI, SCI_SETTARGETSTART, length);
		}
		else
		{
			ScintillaMsg(_hSCI, SCI_SETTARGETSTART, 0);
		}

		posFind = ScintillaMsg(_hSCI, SCI_SEARCHINTARGET, _find.length, (LPARAM)_find.text);
		if (posFind != -1)
		{
			::SendMessage(_hParentHandle, HEXM_SETSEL, posFind, posFind + _find.length);
			::SendMessage(_hParentHandle, HEXM_ENSURE_VISIBLE, 
						  posFind + (HEX_LINE_FIRST == getDisplayPos()?0:_find.length), getDisplayPos());
			_pFindCombo->addText(_find);
		}
		else
		{
			::MessageBox(_hSelf, "Can't find",(_findReplace == TRUE)?"Replace":"Find", MB_OK);
		}
	}
}


void FindReplaceDlg::onReplace(void)
{
	UINT	posBeg  = 0;
	UINT	posEnd  = 0;
	eError	isRep	= E_OK;

	_pFindCombo->getText(&_find);
	_pReplaceCombo->getText(&_replace);

	/* copy data into scintilla handle (encoded if necessary) */
	LittleEndianChange(_hSCI, getCurrentHScintilla());

	/* get selection */
	::SendMessage(_hParentHandle, HEXM_GETSEL, (WPARAM)&posBeg, (LPARAM)&posEnd);
	ScintillaMsg(_hSCI, SCI_SETSELECTIONSTART, posBeg, 0);
	ScintillaMsg(_hSCI, SCI_SETSELECTIONEND, posEnd, 0);

	UINT	length  = abs(posEnd-posBeg);
	char*	text	= (char*)new char[length+1];

	ScintillaMsg(_hSCI, SCI_GETSELTEXT, 0, (LPARAM)text);

	if (memcmp(text, _find.text, length) == 0)
	{
		ScintillaMsg(_hSCI, SCI_TARGETFROMSELECTION);
		ScintillaMsg(_hSCI, SCI_REPLACETARGET, _replace.length, (LPARAM)&_replace.text);
		isRep = replaceLittleToBig(_hSCI, posBeg, length, _replace.length);
		if (isRep == E_OK)
		{
			::SendMessage(_hParentHandle, HEXM_SETPOS, 0, posBeg + _replace.length);
			_pFindCombo->addText(_find);
			_pReplaceCombo->addText(_replace);
		}
	}

	if (isRep == E_OK)
	{
		onFind();
	}
	else
	{
		LITTLE_REPLEACE_ERROR;
	}

	delete [] text;
}


void FindReplaceDlg::processAll(UINT process)
{
	UINT	cnt			= 0;
	UINT	cntError	= 0;
	UINT	posBeg		= 0;
	UINT	posEnd		= ScintillaMsg(SCI_GETLENGTH);
	eError	isRep		= E_OK;

	/* copy data into scintilla handle (encoded if necessary) */
	LittleEndianChange(_hSCI, getCurrentHScintilla());

	if (_isInSel == FALSE)
	{
		ScintillaMsg(_hSCI, SCI_SETTARGETSTART, 0);
		ScintillaMsg(_hSCI, SCI_SETTARGETEND, posEnd);
	}
	else
	{
		/* set start and end position */
		::SendMessage(_hParentHandle, HEXM_GETSEL, (WPARAM)&posBeg, (LPARAM)&posEnd);

		ScintillaMsg(_hSCI, SCI_SETTARGETSTART, posBeg);
		ScintillaMsg(_hSCI, SCI_SETTARGETEND, posEnd);
	}

	/* get strings */
	_pFindCombo->getText(&_find);
	_pReplaceCombo->getText(&_replace);

	/* settings */
	ScintillaMsg(_hSCI, SCI_SETSEARCHFLAGS, _isMatchCase ? SCFIND_MATCHCASE : 0, 0);

	/* search */
	while (ScintillaMsg(_hSCI, SCI_SEARCHINTARGET, _find.length, (LPARAM)&_find.text) != -1)
	{
		switch (process)
		{
			case COUNT:
				cnt++;
				break;
			
			case REPLACE_ALL:
				ScintillaMsg(_hSCI, SCI_REPLACETARGET, _replace.length, (LPARAM)&_replace.text);
				isRep = replaceLittleToBig(	_hSCI, 
											ScintillaMsg(_hSCI, SCI_GETTARGETSTART, 0, 0), 
											_find.length, 
											_replace.length );
				if (isRep == E_STRIDE)
				{
					LITTLE_REPLEACE_ERROR;
					return;
				}
				else if (isRep == E_OK)
				{
					cnt++;
				}
				else if (isRep == E_START)
				{
					cntError++;
				}
				break;

			default:
				break;
		}
		ScintillaMsg(_hSCI, SCI_SETTARGETSTART, ScintillaMsg(_hSCI, SCI_GETTARGETEND), 0);
		ScintillaMsg(_hSCI, SCI_SETTARGETEND, posEnd, 0);
	}

	/* display result */
	if (cnt == 0)
	{
		::MessageBox(_hSelf, "Can't find", "Find", MB_OK);
	}
	else
	{
		char	text[128];

		itoa(cnt, text, 10);
		switch (process)
		{
			case COUNT:
			{
				strcat(text, " tokens are found.");
				_pFindCombo->addText(_find);
				break;
			}
			case REPLACE_ALL:
			{
				UINT	pos;
				::SendMessage(_hParentHandle, HEXM_GETPOS, 0, (LPARAM)&pos);
				::SendMessage(_hParentHandle, HEXM_SETPOS, 0, (LPARAM)pos);

				strcat(text, " tokens are replaced.\n");
				if (cntError != 0)
				{
					char	temp[16];
					strcat(text, itoa(cntError, temp, 10));
					strcat(text, " tokens are skipped, because of alignment error.\n");
				}
				_pFindCombo->addText(_find);
				_pReplaceCombo->addText(_replace);
				break;
			}
			default:
				break;
		}
		::MessageBox(_hSelf, text, "Find", MB_OK);
		_pFindCombo->addText(_find);
	}
}


void FindReplaceDlg::findNext(HWND hParent)
{
	BOOL storeDir = _whichDirection;

	_hParentHandle	= hParent;
	_whichDirection = DIR_DOWN;
	onFind();
	_whichDirection = storeDir;
}


void FindReplaceDlg::findPrev(HWND hParent)
{
	BOOL storeDir = _whichDirection;

	_hParentHandle	= hParent;
	_whichDirection = DIR_UP;
	onFind();
	_whichDirection = storeDir;
}


void FindReplaceDlg::changeCoding(void)
{
	_currDataType = (eCodingType)::SendDlgItemMessage(_hSelf, IDC_COMBO_DATATYPE, CB_GETCURSEL, 0, 0);

	_pFindCombo->setCodingType(_currDataType);
	_pReplaceCombo->setCodingType(_currDataType);

	/* in hex mode keep sure that is match case */
	if (_currDataType == HEX_CODE_HEX)
	{
		::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_MATCHCASE), SW_HIDE);
		_isMatchCase = TRUE;
	}
	else
	{
		::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_MATCHCASE), SW_SHOW);
		_isMatchCase = isChecked(IDC_CHECK_MATCHCASE);
	}

}



