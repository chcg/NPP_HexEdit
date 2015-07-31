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

#ifndef HEXEDITOR_RC_H
#define HEXEDITOR_RC_H


#define IDB_TB_HEX								1001


#define IDD_HEX_DLG              20000
    #define IDC_HEX_LIST                        (IDD_HEX_DLG + 1)
	#define IDC_HEX_CURSORTIMER	    			(IDD_HEX_DLG + 2)
	#define IDC_HEX_TESTLENGTH	    			(IDD_HEX_DLG + 3)

#define IDD_COLUMN_DLG           20100
    #define IDC_COLUMN_EDIT                     (IDD_COLUMN_DLG + 1)

#define IDD_OPTION_DLG           20200
	#define IDC_RADIO_8                         (IDD_OPTION_DLG + 1)
	#define IDC_RADIO_16                        (IDD_OPTION_DLG + 2)
	#define IDC_RADIO_32                        (IDD_OPTION_DLG + 3)
	#define IDC_RADIO_64                        (IDD_OPTION_DLG + 4)
	#define IDC_RADIO_LITTLE                    (IDD_OPTION_DLG + 5)
	#define IDC_RADIO_BIG                       (IDD_OPTION_DLG + 6)
	#define IDC_RADIO_BIN                       (IDD_OPTION_DLG + 7)
	#define IDC_RADIO_HEX                       (IDD_OPTION_DLG + 8)
	#define IDC_STATIC_COLUMN					(IDD_OPTION_DLG + 9)
    #define IDC_STATIC_ADDWIDTH                 (IDD_OPTION_DLG + 10)
    #define IDC_ADDWIDTH_EDIT                   (IDD_OPTION_DLG + 11)
	#define IDC_CHECK_CLM						(IDD_OPTION_DLG + 12)
	#define IDC_TAB_PROP                        (IDD_OPTION_DLG + 13)
	#define IDC_STATIC_EXAMPLE					(IDD_OPTION_DLG + 14)
	#define IDC_EDIT_EXTLIST					(IDD_OPTION_DLG + 15)
	#define IDC_STATIC_PERCENT					(IDD_OPTION_DLG + 16)
	#define IDC_EDIT_PERCENT					(IDD_OPTION_DLG + 17)
	#define IDC_STATIC_FONTNAME					(IDD_OPTION_DLG + 18)
	#define IDC_COMBO_FONTNAME					(IDD_OPTION_DLG + 19)
	#define IDC_STATIC_FONTSIZE					(IDD_OPTION_DLG + 20)
	#define IDC_COMBO_FONTSIZE					(IDD_OPTION_DLG + 21)
	#define IDC_CHECK_BOLD					    (IDD_OPTION_DLG + 22)
	#define IDC_CHECK_ITALIC					(IDD_OPTION_DLG + 23)
	#define IDC_CHECK_UNDERLINE					(IDD_OPTION_DLG + 24)
    #define IDC_STATIC_TXT 	    		        (IDD_OPTION_DLG + 25)
    #define IDC_STATIC_BK 	    		        (IDD_OPTION_DLG + 26)
    #define IDC_STATIC_REGTXT 	    		    (IDD_OPTION_DLG + 27)
    #define IDC_COMBO_REGTXT_TXT 	    		(IDD_OPTION_DLG + 28)
    #define IDC_COMBO_REGTXT_BK  	    		(IDD_OPTION_DLG + 29)
    #define IDC_STATIC_SEL   	    		    (IDD_OPTION_DLG + 30)
    #define IDC_COMBO_SEL_TXT 	    		    (IDD_OPTION_DLG + 31)
    #define IDC_COMBO_SEL_BK  	    		    (IDD_OPTION_DLG + 32)
    #define IDC_STATIC_DIFF   	    		    (IDD_OPTION_DLG + 33)
    #define IDC_COMBO_DIFF_TXT 	    		    (IDD_OPTION_DLG + 34)
    #define IDC_COMBO_DIFF_BK  	    		    (IDD_OPTION_DLG + 35)
    #define IDC_STATIC_BKMK   	    		    (IDD_OPTION_DLG + 36)
    #define IDC_COMBO_BKMK_TXT 	    		    (IDD_OPTION_DLG + 37)
    #define IDC_COMBO_BKMK_BK 	    		    (IDD_OPTION_DLG + 38)
	#define IDC_CHECK_FOCUSRC					(IDD_OPTION_DLG + 39)
    #define IDC_STATIC_CURLINE	    		    (IDD_OPTION_DLG + 40)
    #define IDC_COMBO_CURLINE 	    		    (IDD_OPTION_DLG + 41)

#define    IDD_GOTO_DLG          20300
    #define IDC_EDIT_GOTO                       (IDD_GOTO_DLG + 1)
    #define IDC_CURRLINE                        (IDD_GOTO_DLG + 2)
    #define IDC_LASTLINE                        (IDD_GOTO_DLG + 3)
	#define IDC_CHECK_LINE                      (IDD_GOTO_DLG + 4)
	#define IDC_STATIC_HERE                 	(IDD_GOTO_DLG + 5)
	#define IDC_STATIC_GO                   	(IDD_GOTO_DLG + 6)
	#define IDC_STATIC_MAX                  	(IDD_GOTO_DLG + 7)
	#define IDC_RADIO_ADDRESS               	(IDD_GOTO_DLG + 8)
	#define IDC_RADIO_OFFSET                	(IDD_GOTO_DLG + 9)


#define    IDD_FINDREPLACE_DLG   20400
    #define IDC_COMBO_FIND                      (IDD_FINDREPLACE_DLG + 1)
    #define IDC_COMBO_REPLACE                   (IDD_FINDREPLACE_DLG + 2)
    #define IDC_CHECK_MATCHCASE                 (IDD_FINDREPLACE_DLG + 3)
    #define IDC_CHECK_WRAP                      (IDD_FINDREPLACE_DLG + 4)
    #define IDC_RADIO_DIRUP                     (IDD_FINDREPLACE_DLG + 5)
    #define IDC_RADIO_DIRDOWN                   (IDD_FINDREPLACE_DLG + 6)
    #define IDC_REPLACE                         (IDD_FINDREPLACE_DLG + 7)
    #define IDC_COUNT                           (IDD_FINDREPLACE_DLG + 8)
    #define IDC_REPLACEALL                      (IDD_FINDREPLACE_DLG + 9)
    #define IDC_SWITCH                          (IDD_FINDREPLACE_DLG + 10)
    #define IDC_CHECK_TRANSPARENT               (IDD_FINDREPLACE_DLG + 11)
    #define IDC_RADIO_TOP                       (IDD_FINDREPLACE_DLG + 12)
    #define IDC_RADIO_MIDDLE                    (IDD_FINDREPLACE_DLG + 13)
    #define IDC_RADIO_BOTTOM                    (IDD_FINDREPLACE_DLG + 14)
    #define IDC_CHECK_IN_SEL                    (IDD_FINDREPLACE_DLG + 15)
    #define IDC_STATIC_REPLACE                  (IDD_FINDREPLACE_DLG + 16)
    #define IDC_STATIC_REPALL                   (IDD_FINDREPLACE_DLG + 17)
	#define IDC_STATIC_FIND                 	(IDD_FINDREPLACE_DLG + 18)
	#define IDC_STATIC_TYPE                 	(IDD_FINDREPLACE_DLG + 19)
	#define IDC_STATIC_DIRECTION            	(IDD_FINDREPLACE_DLG + 20)
	#define IDC_STATIC_POSITION             	(IDD_FINDREPLACE_DLG + 21)
    #define IDC_SLIDER_PERCENTAGE               (IDD_FINDREPLACE_DLG + 22)
    #define IDC_COMBO_DATATYPE                  (IDD_FINDREPLACE_DLG + 23)

#define		IDD_PATTERN_DLG		 20500
    #define IDC_STATIC_PATTERN                  (IDD_PATTERN_DLG + 1)
    #define IDC_STATIC_COUNT                    (IDD_PATTERN_DLG + 2)
	#define IDC_STATIC_COL                      (IDD_PATTERN_DLG + 3)
    #define IDC_COMBO_PATTERN                   (IDD_PATTERN_DLG + 4)
    #define IDC_EDIT_COUNT                      (IDD_PATTERN_DLG + 5)
	#define IDC_EDIT_COL                        (IDD_PATTERN_DLG + 6)

#define    IDD_HELP_DLG	         20600
    #define IDC_EMAIL_LINK                      (IDD_HELP_DLG + 1)
	#define IDC_NPP_PLUGINS_URL					(IDD_HELP_DLG + 2)
	#define IDC_STATIC_AUTHOR               	(IDD_HELP_DLG + 3)
	#define IDC_STATIC_EMAIL                	(IDD_HELP_DLG + 4)
	#define IDC_STATIC_THX                  	(IDD_HELP_DLG + 5)
	#define IDC_STATIC_MENU                 	(IDD_HELP_DLG + 6)
	#define IDC_STATIC_HELP_0               	(IDD_HELP_DLG + 7)
	#define IDC_STATIC_HELP_1               	(IDD_HELP_DLG + 8)
	#define IDC_STATIC_HELP_2               	(IDD_HELP_DLG + 9)
	#define IDC_STATIC_HELP_4               	(IDD_HELP_DLG + 10)
	#define IDC_STATIC_HELP_5               	(IDD_HELP_DLG + 11)
	#define IDC_STATIC_HELP_7               	(IDD_HELP_DLG + 12)
	#define IDC_STATIC_HELP_8               	(IDD_HELP_DLG + 13)

#define	IDD_COLOR_POPUP          20700
    #define	IDC_COLOR_LIST                      (IDD_COLOR_POPUP + 1)

#define IDD_COMPARE_DLG          20750
    #define IDC_STATIC_COMPARE_LR               (IDD_COMPARE_DLG + 1)
    #define IDC_STATIC_COMPARE_TB               (IDD_COMPARE_DLG + 2)
    #define IDC_BUTTON_RIGHT                    (IDD_COMPARE_DLG + 3)
    #define IDC_BUTTON_LEFT                     (IDD_COMPARE_DLG + 4)
    #define IDC_BUTTON_BOTTOM                   (IDD_COMPARE_DLG + 5)
    #define IDC_BUTTON_TOP                      (IDD_COMPARE_DLG + 6)
    
	#define IDC_STATIC                      -1

    
#define HEXM_MSG				 20800
	#define HEXM_SETSEL                         (HEXM_MSG + 1)
	#define HEXM_GETSEL                         (HEXM_MSG + 2)
	#define HEXM_SETPOS                         (HEXM_MSG + 3)
	#define HEXM_GETPOS                         (HEXM_MSG + 4)
    #define HEXM_ENSURE_VISIBLE                 (HEXM_MSG + 5)
    #define HEXM_GETSETTINGS                    (HEXM_MSG + 6)
	#define HEXM_SETCURLINE						(HEXM_MSG + 7)
	#define	HEXM_GETCURLINE						(HEXM_MSG + 8)
	#define HEXM_SETCOLUMNCNT					(HEXM_MSG + 9)
	#define HEXM_GETLINECNT						(HEXM_MSG + 10)
	#define HEXM_UPDATEBKMK						(HEXM_MSG + 11)
	#define HEXM_GETLENGTH						(HEXM_MSG + 12)
    #define HEXM_PICKUP_COLOR                   (HEXM_MSG + 13)
    #define HEXM_PICKUP_CANCEL                  (HEXM_MSG + 14)
	#define HEXM_GETDOCCP						(HEXM_MSG + 15)

    
#define IDCMD					  50000
	#define	IDC_NP_AUTOCOMPLETE    				(IDCMD+0)
	#define	IDC_NP_SEARCH_FINDNEXTSELECTED		(IDCMD+1)
	#define	IDC_NP_SEARCH_FINDPREVSELECTED		(IDCMD+2)
	#define	IDC_NP_PREV_DOC						(IDCMD+3)
	#define	IDC_NP_NEXT_DOC						(IDCMD+4)
	#define	IDC_NP_EDIT_TOGGLEMACRORECORDING	(IDCMD+5)
	#define	IDC_NP_KEY_HOME						(IDCMD+6)
	#define	IDC_NP_KEY_END						(IDCMD+7)
	#define	IDC_NP_KEY_SELECT_2_HOME			(IDCMD+8)
	#define	IDC_NP_KEY_SELECT_2_END				(IDCMD+9)

    
#endif // HEXEDITOR_RC_H
