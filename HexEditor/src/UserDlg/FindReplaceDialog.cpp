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
	if (_hSCI == NULL)
	{
		/* create new scintilla handle */
		_hSCI = (HWND)::SendMessage(_nppData._nppHandle, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
	}

    if (!isCreated())
	{
        create(IDD_FINDREPLACE_DLG);
		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGADD, (LPARAM)_hSelf);

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

			/* change language */
			NLChangeDialog(_hInst, _nppData._nppHandle, _hSelf, _T("FindReplace"));

            break;
        }
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDOK :
                {
					onFind(FALSE);
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
					HWND	hSci = getCurrentHScintilla();
					ScintillaMsg(hSci, SCI_BEGINUNDOACTION);
					processAll(REPLACE_ALL);
					ScintillaMsg(hSci, SCI_ENDUNDOACTION);
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
	TCHAR		txtTab[32];

	item.mask		= TCIF_TEXT;
	if (NLGetText(_hInst, _hParent, _T("Find"), txtTab, sizeof(txtTab) / sizeof(TCHAR)) == FALSE)
		_tcscpy(txtTab, _T("Find"));
	item.pszText	= txtTab;
	item.cchTextMax	= (int)_tcslen(txtTab);
	::SendDlgItemMessage(_hSelf, IDC_SWITCH, TCM_INSERTITEM, 0, (LPARAM)&item);
	if (NLGetText(_hInst, _hParent, _T("Replace"), txtTab, sizeof(txtTab) / sizeof(TCHAR)) == FALSE)
		_tcscpy(txtTab, _T("Replace"));
	item.pszText	= txtTab;
	item.cchTextMax	= (int)_tcslen(txtTab);
	::SendDlgItemMessage(_hSelf, IDC_SWITCH, TCM_INSERTITEM, 1, (LPARAM)&item);

	/* init comboboxes */
	_pFindCombo			= new MultiTypeCombo;
	_pReplaceCombo		= new MultiTypeCombo;

	_pFindCombo->init(_hParent, ::GetDlgItem(_hSelf, IDC_COMBO_FIND));
	_pReplaceCombo->init(_hParent, ::GetDlgItem(_hSelf, IDC_COMBO_REPLACE));

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
	HMODULE hUser32 = ::GetModuleHandle(_T("User32"));

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
	TCHAR	txtCaption[32];

	if (_findReplace == TRUE)
	{
		if (NLGetText(_hInst, _hParent, _T("Replace"), txtCaption, sizeof(txtCaption) / sizeof(TCHAR)) == FALSE)
			_tcscpy(txtCaption, _T("Replace"));
		::SetWindowText(_hSelf, txtCaption);
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
		if (NLGetText(_hInst, _hParent, _T("Find"), txtCaption, sizeof(txtCaption) / sizeof(TCHAR)) == FALSE)
			_tcscpy(txtCaption, _T("Find"));
		::SetWindowText(_hSelf, txtCaption);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_COUNT), SW_SHOW);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACE), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_REPLACE), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_COMBO_REPLACE), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_REPLACEALL), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_CHECK_IN_SEL), SW_HIDE);
		::ShowWindow(::GetDlgItem(_hSelf, IDC_STATIC_REPALL), SW_HIDE);
	}
	TabCtrl_SetCurSel(::GetDlgItem(_hSelf, IDC_SWITCH), _findReplace);

	/* set codepages of combo boxes */
	eNppCoding codepage = HEX_CODE_NPP_ASCI;
	::SendMessage(_hParentHandle, HEXM_GETDOCCP, 0, (LPARAM)&codepage);
	_pFindCombo->setDocCodePage(codepage);
	_pReplaceCombo->setDocCodePage(codepage);

	/* get selected length */
	tComboInfo	info	= {0};

	getSelText(&info);
	if (info.length != 0) {
		_pFindCombo->setText(info);
	}

	/* enable box setting for replace all */
	::EnableWindow(::GetDlgItem(_hSelf, IDC_CHECK_IN_SEL), (info.length == 0)?FALSE:TRUE);
}


void FindReplaceDlg::onFind(BOOL isVolatile)
{
	/* get current scintilla */
	HWND		hSciSrc		= getCurrentHScintilla();
	INT			lenSrc		= ScintillaMsg(hSciSrc, SCI_GETLENGTH);
	INT			offset		= 0;
	INT			length		= 0;
	INT			posBeg		= 0;
	INT			posEnd		= 0;
	INT			wrapPos		= 0;
	BOOL		loopEnd		= FALSE;
	BOOL		doWrap		= FALSE;
	BOOL		wrapDone	= FALSE;
	tComboInfo	info		= {0};

	if (_hSCI == NULL)
	{
		/* create new scintilla handle */
		_hSCI = (HWND)::SendMessage(_nppData._nppHandle, NPPM_CREATESCINTILLAHANDLE, 0, (LPARAM)_hSelf);
	}

	/* in dependency of find type get search information from combo or directly from source */
	if (isVolatile == FALSE)
	{
		_pFindCombo->getText(&_find);
		info = _find;
		if (info.length == 0)
			return;
	}
	else
	{
		getSelText(&info);
		if (info.length == 0) {
			if (NLMessageBox(_hInst, _hParent, _T("MsgBox SelectSomething"), MB_OK) == FALSE)
				::MessageBox(_hParent, _T("Select something in the text!"), _T("Hex-Editor"), MB_OK);
			return;
		}
	}

	/* set match case */
	ScintillaMsg(_hSCI, SCI_SETSEARCHFLAGS, _isMatchCase ? SCFIND_MATCHCASE : 0, 0);

	/* get selection and correct anchor and cursor position */
	::SendMessage(_hParentHandle, HEXM_GETSEL, (WPARAM)&posBeg, (LPARAM)&posEnd);
	if (posEnd < posBeg) {
		UINT posTmp = posBeg;
		posBeg = posEnd;
		posEnd = posTmp;
	}
	wrapPos = posBeg;

	do {
		BOOL	isConverted = FALSE;

		/* copy data into scintilla handle (encoded if necessary) and select string */

		if ((wrapDone == TRUE) && (lenSrc < FIND_BLOCK)) {
			if (_whichDirection == DIR_DOWN) {
				length = wrapPos + info.length + 1;
			} else {
				length = FIND_BLOCK;
			}
		} else {
			length = FIND_BLOCK;
		}

		if (_whichDirection == DIR_DOWN)
		{
			offset = posBeg;
			if (LittleEndianChange(_hSCI, hSciSrc, &offset, &length) == TRUE)
			{
				ScintillaMsg(_hSCI, SCI_SETTARGETSTART, posEnd - offset);
				ScintillaMsg(_hSCI, SCI_SETTARGETEND, length);
				isConverted = TRUE;
			}
		}
		else
		{
			posEnd -= FIND_BLOCK;
			offset = posEnd;
			if (LittleEndianChange(_hSCI, hSciSrc, &offset, &length) == TRUE)
			{
				ScintillaMsg(_hSCI, SCI_SETTARGETSTART, posBeg);
				ScintillaMsg(_hSCI, SCI_SETTARGETEND, posEnd - offset);
				isConverted = TRUE;
			}
		}

		if (isConverted == TRUE)
		{
			/* find string */
			INT posFind = ScintillaMsg(_hSCI, SCI_SEARCHINTARGET, info.length, (LPARAM)info.text);
			if (posFind != -1)
			{
				/* found */
				posFind += offset;
				::SendMessage(_hParentHandle, HEXM_SETSEL, posFind, posFind + info.length);
				if (isVolatile == FALSE) {
					_pFindCombo->addText(info);
				}
				loopEnd = TRUE;
			}
			else
			{
				/* calculate new start find position */
				if (_whichDirection == DIR_DOWN)
				{
					posBeg = offset + length;

					/* test if out of bound */
					if ((posBeg >= lenSrc) && (wrapDone == FALSE)) {
						posBeg = 0;
						/* notify wrap is done */
						doWrap = TRUE;
					} else if (posBeg != lenSrc) {
						/* calculate new start find position */
						posBeg -= (info.length + 1);
					}

					/* indicate that wrap is still done */
					wrapDone = doWrap;

				} 
				else
				{
					/* indicate wrap done next time */
					wrapDone = doWrap;

					posBeg = offset;

					/* test if out of bound */
					if ((posBeg <= 0) && (wrapDone == FALSE)) {
						posBeg = lenSrc;
						/* notify wrap is done */
						doWrap = TRUE;
					} else if (posBeg != 0) {
						/* calculate new start find position */
						posBeg += (info.length + 1);
					}
				}

				/* if wrap was done and posBeg is jump over the wrapPos (start pos on function call)... */
				if ((wrapDone == TRUE) &&
					(((_whichDirection == DIR_DOWN) && (posBeg >= wrapPos)) ||
					 ((_whichDirection == DIR_UP  ) && (posEnd <= wrapPos))))
				{
					/* ... leave the function */
					TCHAR	text[128];
					TCHAR	TEMP[128];

					if (NLGetText(_hInst, _hParent, _T("CantFind"), TEMP, 128)) {
						_tcscpy(text, TEMP);
						if (NLGetText(_hInst, _hParent, (_findReplace == TRUE)?_T("Replace"):_T("Find"), TEMP, 128)) {
							::MessageBox(_hParent, text, TEMP, MB_OK);
						} else {
							::MessageBox(_hParent, text, (_findReplace == TRUE)?_T("Replace"):_T("Find"), MB_OK);
						}
					} else {
						::MessageBox(_hSelf, _T("Can't find"),(_findReplace == TRUE)?_T("Replace"):_T("Find"), MB_OK);
					}

					loopEnd = TRUE;
				}

				/* for further calculation */
				posEnd = posBeg;
			}
			CleanScintillaBuf(_hSCI);
		}
		else
		{
			loopEnd = TRUE;
		}
	} while (loopEnd == FALSE);
}


void FindReplaceDlg::onReplace(void)
{
	HWND	hSciSrc	= getCurrentHScintilla();
	INT		lenSrc  = ScintillaMsg(hSciSrc, SCI_GETLENGTH);
	INT		lenStr	= 0;
	INT		offset	= 0;
	INT		length  = 0;
	INT		posBeg  = 0;
	INT		posEnd  = 0;
	eError	isRep	= E_OK;

	_pFindCombo->getText(&_find);
	_pReplaceCombo->getText(&_replace);

	/* get selection and correct anchor and cursor position */
	::SendMessage(_hParentHandle, HEXM_GETSEL, (WPARAM)&posBeg, (LPARAM)&posEnd);
	if (posEnd < posBeg) {
		UINT posTmp = posBeg;
		posBeg = posEnd;
		posEnd = posTmp;
	}

	/* copy data into scintilla handle (encoded if necessary) and select string */
	offset = posBeg;
	length = posEnd - posBeg;
	lenStr = length;
	if (LittleEndianChange(_hSCI, hSciSrc, &offset, &length) == TRUE)
	{
		LPSTR	text	= (LPSTR)new CHAR[lenStr+1];

		if (text != NULL)
		{
			/* get selection and compare if it is equal to expected text */
			ScintillaMsg(_hSCI, SCI_SETSELECTIONSTART, posBeg - offset, 0);
			ScintillaMsg(_hSCI, SCI_SETSELECTIONEND, posEnd - offset, 0);
    		ScintillaMsg(_hSCI, SCI_GETSELTEXT, 0, (LPARAM)text);

			/* make difference between match case modes */
    		if (((_isMatchCase == TRUE) && (memcmp(text, _find.text, lenStr) == 0)) ||
				((_isMatchCase == FALSE) && (stricmp(text, _find.text) == 0)))
    		{
    			ScintillaMsg(_hSCI, SCI_TARGETFROMSELECTION);
    			ScintillaMsg(_hSCI, SCI_REPLACETARGET, _replace.length, (LPARAM)&_replace.text);
    			isRep = replaceLittleToBig(hSciSrc, _hSCI, posBeg - offset, posBeg, lenStr, _replace.length);
    			if (isRep == E_OK)
    			{
    				::SendMessage(_hParentHandle, HEXM_SETPOS, 0, posBeg + _replace.length);
    				_pFindCombo->addText(_find);
    				_pReplaceCombo->addText(_replace);
    			}
    		}

    		if (isRep == E_OK) {
    			onFind(FALSE);
    		} else {
    			LITTLE_REPLACE_ERROR;
    		}

    		delete [] text;
		}
   		CleanScintillaBuf(_hSCI);
	}
}



void FindReplaceDlg::processAll(UINT process)
{
	HWND	hSciSrc		= getCurrentHScintilla();
	INT		lenSrc		= ScintillaMsg(hSciSrc, SCI_GETLENGTH);
	INT		cnt			= 0;
	INT		cntError	= 0;
	INT		offset		= 0;
	INT		length		= 0;
	INT		posBeg		= 0;
	INT		posEnd		= 0;
	BOOL	loopEnd		= FALSE;
	eError	isRep		= E_OK;

	/* get strings */
	_pFindCombo->getText(&_find);
	_pReplaceCombo->getText(&_replace);

	if (_find.length != 0)
	{
		/* selection dependent start position */
		if ((_isInSel == TRUE) && (process == REPLACE_ALL))
		{
			::SendMessage(_hParentHandle, HEXM_GETSEL, (WPARAM)&posBeg, (LPARAM)&lenSrc);
		}

		/* settings */
		ScintillaMsg(_hSCI, SCI_SETSEARCHFLAGS, _isMatchCase ? SCFIND_MATCHCASE : 0, 0);

		/* keep sure that end and begin at the same position */
		posEnd = posBeg;

		do {
			/* copy data into scintilla handle (encoded if necessary) and select string */
			offset = posBeg;
			length = FIND_BLOCK;
			if (LittleEndianChange(_hSCI, hSciSrc, &offset, &length) == TRUE)
			{
				ScintillaMsg(_hSCI, SCI_SETTARGETSTART, posBeg - offset);
				ScintillaMsg(_hSCI, SCI_SETTARGETEND, length);

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
							isRep = replaceLittleToBig(	hSciSrc, _hSCI, 
														ScintillaMsg(_hSCI, SCI_GETTARGETSTART, 0, 0),
														ScintillaMsg(_hSCI, SCI_GETTARGETSTART, 0, 0) + offset,
														_find.length, 
														_replace.length );
							if (isRep == E_STRIDE)
							{
								LITTLE_REPLACE_ERROR;
								CleanScintillaBuf(_hSCI);
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

							/* calc offset */
							lenSrc += (_replace.length - _find.length);
							break;

						default:
							break;
					}
					ScintillaMsg(_hSCI, SCI_SETTARGETSTART, ScintillaMsg(_hSCI, SCI_GETTARGETEND));
					ScintillaMsg(_hSCI, SCI_SETTARGETEND, length);
				}

				/* calculate offset or end loop */
				posBeg = offset + length;
				if (posBeg < lenSrc) {
					posBeg -= (_find.length - 1);
				} else {
					loopEnd = TRUE;
				}
			}
		} while (loopEnd == FALSE);
	}

	TCHAR	TEMP[128];
	TCHAR	text[128];

	/* display result */
	if (cnt == 0)
	{
		if (NLGetText(_hInst, _hParent, _T("CantFind"), TEMP, 128)) {
			_tcscpy(text, TEMP);
			if (NLGetText(_hInst, _hParent, _T("Find"), TEMP, 128)) {
				::MessageBox(_hParent, text, TEMP, MB_OK);
			} else {
				::MessageBox(_hParent, text, _T("Find"), MB_OK);
			}
		} else {
			::MessageBox(_hSelf, _T("Can't find"), _T("Find"), MB_OK);
		}
	}
	else
	{
		switch (process)
		{
			case COUNT:
			{
				if (NLGetText(_hInst, _hParent, _T("Tokens Found"), TEMP, 128)) {
					_stprintf(text, TEMP, cnt);
				} else {
					_stprintf(text, _T("%i tokens are found."), cnt);
				}

				if (NLGetText(_hInst, _hParent, _T("Count"), TEMP, 128)) {
					::MessageBox(_hParent, text, TEMP, MB_OK);
				} else {
					::MessageBox(_hParent, text, _T("Count"), MB_OK);
				}

				_pFindCombo->addText(_find);
				break;
			}
			case REPLACE_ALL:
			{
				UINT	pos;
				::SendMessage(_hParentHandle, HEXM_GETPOS, 0, (LPARAM)&pos);
				::SendMessage(_hParentHandle, HEXM_SETPOS, 0, (LPARAM)pos);

				if (NLGetText(_hInst, _hParent, _T("Tokens Replaced"), TEMP, 128)) {
					_stprintf(text, TEMP, cnt);
				} else {
					_stprintf(text, _T("%i tokens are replaced.\n"), cnt);
				}

				if (cntError != 0)
				{
					if (NLGetText(_hInst, _hParent, _T("Tokens Skipped"), TEMP, 128)) {
						_stprintf(text, TEMP, text, cntError);
					} else {
						_stprintf(text, _T("%s%i tokens are skipped, because of alignment error.\n"), text, cntError);
					}
				}

				if (NLGetText(_hInst, _hParent, _T("Replace"), TEMP, 128)) {
					::MessageBox(_hParent, text, TEMP, MB_OK);
				} else {
					::MessageBox(_hParent, text, _T("Replace"), MB_OK);
				}

				_pFindCombo->addText(_find);
				_pReplaceCombo->addText(_replace);
				break;
			}
			default:
				break;
		}
		_pFindCombo->addText(_find);
	}
}


void FindReplaceDlg::findNext(HWND hParent, BOOL isVolatile)
{
	if (isCreated() || (isVolatile == TRUE))
	{
		BOOL storeDir = _whichDirection;

		_hParentHandle	= hParent;
		_whichDirection = DIR_DOWN;
		onFind(isVolatile);
		_whichDirection = storeDir;
	}
}


void FindReplaceDlg::findPrev(HWND hParent, BOOL isVolatile)
{
	if (isCreated() || (isVolatile == TRUE))
	{
		BOOL storeDir = _whichDirection;

		_hParentHandle	= hParent;
		_whichDirection = DIR_UP;
		onFind(isVolatile);
		_whichDirection = storeDir;
	}
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


void FindReplaceDlg::getSelText(tComboInfo* info)
{
	if (info == NULL)
		return;

	UINT	posBeg	= 0;
	UINT	posEnd	= 0;

	/* get selection and set find text */
	::SendMessage(_hParentHandle, HEXM_GETSEL, (WPARAM)&posBeg, (LPARAM)&posEnd);

	INT	offset	= (INT)(posBeg < posEnd ? posBeg : posEnd);
	INT	length	= (abs(posEnd-posBeg) > COMBO_STR_MAX ? COMBO_STR_MAX : abs(posEnd-posBeg));
	info->length = length;

	if (info->length != 0)
	{
		CHAR	*text	= (CHAR*)new CHAR[info->length+1];
		if (text != NULL)
		{
			/* convert and select and get the text */
			if (LittleEndianChange(_hSCI, getCurrentHScintilla(), &offset, &length) == TRUE)
			{
				ScintillaMsg(_hSCI, SCI_SETSELECTIONSTART, posBeg - offset, 0);
				ScintillaMsg(_hSCI, SCI_SETSELECTIONEND, posEnd - offset, 0);
				ScintillaMsg(_hSCI, SCI_TARGETFROMSELECTION, 0, 0);
				ScintillaMsg(_hSCI, SCI_GETSELTEXT, 0, (LPARAM)text);

				/* encode the text in dependency of selected data type */
				memcpy(info->text, text, info->length);

				CleanScintillaBuf(_hSCI);
			}
			delete [] text;
		}
	}
}


