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

#ifndef PATTERN_DEFINE_H
#define PATTERN_DEFINE_H

#include "Hex.h"
#include "HexResource.h"
#include "StaticDialog.h"
#include "MultiTypeCombo.h"



class PatternDlg : public StaticDialog
{

public:
	PatternDlg() : StaticDialog() {};
    
    void init(HINSTANCE hInst, NppData nppData)
	{
		_nppData = nppData;
		Window::init(hInst, nppData._nppHandle);
	};

	void patternReplace(HWND hHexEdit);
	void insertColumns(HWND hHexEdit);

    virtual void destroy() {
		/* deregister this dialog */
		display(false);
		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (LPARAM)_hSelf);
	};


protected :
	virtual BOOL CALLBACK run_dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

   	void doDialog(HWND hHexEdit);
	BOOL onInsert(void);
	BOOL onReplace(void);

	/* Subclassing list */
	LRESULT runProcEdit(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndEditProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((PatternDlg *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runProcEdit(hwnd, Message, wParam, lParam));
	};


private:
	/* Handles */
	NppData				_nppData;
	HWND				_hParentHandle;
	WNDPROC				_hDefaultEditProc;

	tComboInfo			_pattern;
	MultiTypeCombo*		_pCombo;

	BOOL				_isReplace;
	TCHAR				_txtCaption[64];
};



#endif // PATTERN_DEFINE_H
