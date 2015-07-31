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

#ifndef FIND_REPLACE_DEFINE_H
#define FIND_REPLACE_DEFINE_H

#include "StaticDialog.h"
#include <algorithm>
#include <commctrl.h>

#include "Hex.h"
#include "MultiTypeCombo.h"

#include "HexResource.h"




#define	DIR_DOWN				TRUE
#define DIR_UP					FALSE


#define	COUNT					0
#define REPLACE_ALL				1




class FindReplaceDlg : public StaticDialog
{

public:
	FindReplaceDlg() : StaticDialog(), _transFuncAddr(NULL)
	{
		_findReplace	= FALSE;
		_whichDirection	= DIR_DOWN;
		_isMatchCase	= FALSE;
		_isWrap			= TRUE;
		_isInSel		= FALSE;
		_hSCI			= NULL;
	};
    
    void init(HINSTANCE hInst, NppData nppData)
	{
		_nppData = nppData;
		Window::init(hInst, nppData._nppHandle);
	};

	void display(bool toShow = true);

   	void doDialog(HWND hParent, BOOL findReplace);
	void findNext(HWND hParent, BOOL isVolatile = FALSE);
	void findPrev(HWND hParent, BOOL isVolatile = FALSE);
	BOOL isFindReplace(void)
	{
		return _findReplace;
	};

    virtual void destroy() {
    };


protected :
	virtual BOOL CALLBACK run_dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    void initDialog(void);
	void updateDialog(void);
	void onFind(BOOL isVolatile);
	void onReplace(void);
	void changeCoding(void);
	void getSelText(tComboInfo* info);
	void processAll(UINT process);
	BOOL processFindReplace(BOOL find = TRUE);

	BOOL isChecked(int id) const {
		return ((BST_CHECKED == ::SendMessage(::GetDlgItem(_hSelf, id), BM_GETCHECK, 0, 0))?TRUE:FALSE);
	};

	int getDisplayPos() const {
		if (isChecked(IDC_RADIO_TOP))
			return HEX_LINE_FIRST;
		else if (isChecked(IDC_RADIO_MIDDLE))
			return HEX_LINE_MIDDLE;
		else //IDC_RADIO_BOTTOM
			return HEX_LINE_LAST;
	};

	/* set transparency */
	void setTrans(void)
	{
		if (::SendDlgItemMessage(_hSelf, IDC_CHECK_TRANSPARENT, BM_GETCHECK, 0, 0) == BST_CHECKED)
		{
			INT percent = (INT)::SendDlgItemMessage(_hSelf, IDC_SLIDER_PERCENTAGE, TBM_GETPOS, 0, 0);
			::SetWindowLongPtr(_hSelf, GWL_EXSTYLE, ::GetWindowLong(_hSelf, GWL_EXSTYLE) | /*WS_EX_LAYERED*/0x00080000);
			_transFuncAddr(_hSelf, 0, percent, 0x00000002);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_SLIDER_PERCENTAGE), SW_SHOW);

		}
		else
		{
			::SetWindowLongPtr(_hSelf, GWL_EXSTYLE,  ::GetWindowLong(_hSelf, GWL_EXSTYLE) & ~/*WS_EX_LAYERED*/0x00080000);
			::ShowWindow(::GetDlgItem(_hSelf, IDC_SLIDER_PERCENTAGE), SW_HIDE);
		}
	};


private:
	/* subclassing handle */
	WNDPROC				_hDefaultComboProc;

	/* Handles */
	NppData				_nppData;
	HWND				_hParentHandle;
    HWND				_hSCI;

	BOOL				_findReplace;
	BOOL				_whichDirection;
	BOOL				_isMatchCase;
	BOOL				_isWrap;
	BOOL				_isInSel;

	/* for coding type */
	eNppCoding			_nppBaseCode;			
	eCodingType			_currDataType;

	tComboInfo			_find;
	tComboInfo			_replace;
	MultiTypeCombo*		_pFindCombo;
	MultiTypeCombo*		_pReplaceCombo;


	/* for transparency */
	WNDPROC				_transFuncAddr;
};



#endif // FIND_REPLACE_DEFINE_H
