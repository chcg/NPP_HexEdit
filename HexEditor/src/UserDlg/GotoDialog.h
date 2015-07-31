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

#ifndef GOTO_DEFINE_H
#define GOTO_DEFINE_H

#include "StaticDialog.h"
#include "Hex.h"
#include "HexResource.h"



class GotoDlg : public StaticDialog
{

public:
	GotoDlg() : StaticDialog(), _isOff(FALSE) {};
    
    void init(HINSTANCE hInst, NppData nppData, LPTSTR iniFilePath)
	{
		_nppData = nppData;
		_iniFilePath = iniFilePath;
		Window::init(hInst, nppData._nppHandle);
	};

   	void doDialog(HWND hParent);

    virtual void destroy() {};


protected :
	virtual BOOL CALLBACK run_dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void calcAddress(void);
    void UpdateDialog(void);

	/* Subclassing list */
	LRESULT runProcEdit(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK wndEditProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {
		return (((GotoDlg *)(::GetWindowLong(hwnd, GWL_USERDATA)))->runProcEdit(hwnd, Message, wParam, lParam));
	};


private:
	/* Handles */
	NppData			_nppData;
    HWND			_HSource;
	HWND			_hParentHandle;

	HWND			_hLineEdit;
	WNDPROC			_hDefaultEditProc;

	LPTSTR			_iniFilePath;

	BOOL			_isHex;
	BOOL			_isOff;
};



#endif // GOTO_DEFINE_H
