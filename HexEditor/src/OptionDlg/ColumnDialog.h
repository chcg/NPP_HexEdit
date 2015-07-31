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


#ifndef COLUMN_DIALOG_H
#define COLUMN_DIALOG_H

#include "StaticDialog.h"
#include "Hex.h"
#include "HEXResource.h"



class ColumnDlg : public StaticDialog
{

public:
	ColumnDlg() : StaticDialog() {};
    
    void init(HINSTANCE hInst, NppData nppData)
	{
		_nppData = nppData;
		Window::init(hInst, nppData._nppHandle);
	};

   	UINT doDialogColumn(UINT column);
   	UINT doDialogAddWidth(UINT column);

    virtual void destroy() {};


protected :
	BOOL CALLBACK run_dlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

private:
	/* Handles */
	NppData			_nppData;
    HWND			_HSource;

	BOOL			_isColumn;
	UINT			_column;
	UINT			_width;
};


#endif // COLUMN_DIALOG_H
