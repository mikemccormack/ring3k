/*
 * Message spying routines
 *
 * Copyright 1994, Bob Amstadt
 *           1995, Alex Korobka
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include "windef.h"
#include "winternl.h"
#include "ntwin32.h"

#define SPY_MAX_MSGNUM   WM_USER

static const char * const MessageTypeNames[SPY_MAX_MSGNUM + 1] =
{
    "WM_NULL",                  /* 0x00 */
    "WM_CREATE",
    "WM_DESTROY",
    "WM_MOVE",
    "wm_sizewait",
    "WM_SIZE",
    "WM_ACTIVATE",
    "WM_SETFOCUS",
    "WM_KILLFOCUS",
    "WM_SETVISIBLE",
    "WM_ENABLE",
    "WM_SETREDRAW",
    "WM_SETTEXT",
    "WM_GETTEXT",
    "WM_GETTEXTLENGTH",
    "WM_PAINT",
    "WM_CLOSE",                 /* 0x10 */
    "WM_QUERYENDSESSION",
    "WM_QUIT",
    "WM_QUERYOPEN",
    "WM_ERASEBKGND",
    "WM_SYSCOLORCHANGE",
    "WM_ENDSESSION",
    "wm_systemerror",
    "WM_SHOWWINDOW",
    "WM_CTLCOLOR",
    "WM_WININICHANGE",
    "WM_DEVMODECHANGE",
    "WM_ACTIVATEAPP",
    "WM_FONTCHANGE",
    "WM_TIMECHANGE",
    "WM_CANCELMODE",
    "WM_SETCURSOR",             /* 0x20 */
    "WM_MOUSEACTIVATE",
    "WM_CHILDACTIVATE",
    "WM_QUEUESYNC",
    "WM_GETMINMAXINFO",
    "wm_unused3",
    "wm_painticon",
    "WM_ICONERASEBKGND",
    "WM_NEXTDLGCTL",
    "wm_alttabactive",
    "WM_SPOOLERSTATUS",
    "WM_DRAWITEM",
    "WM_MEASUREITEM",
    "WM_DELETEITEM",
    "WM_VKEYTOITEM",
    "WM_CHARTOITEM",
    "WM_SETFONT",               /* 0x30 */
    "WM_GETFONT",
    "WM_SETHOTKEY",
    "WM_GETHOTKEY",
    "wm_filesyschange",
    "wm_isactiveicon",
    "wm_queryparkicon",
    "WM_QUERYDRAGICON",
    "wm_querysavestate",
    "WM_COMPAREITEM",
    "wm_testing",
    NULL,
    NULL,
    "WM_GETOBJECT",             /* 0x3d */
    "wm_activateshellwindow",
    NULL,

    NULL,                       /* 0x40 */
    "wm_compacting", NULL, NULL,
    "WM_COMMNOTIFY", NULL,
    "WM_WINDOWPOSCHANGING",     /* 0x0046 */
    "WM_WINDOWPOSCHANGED",      /* 0x0047 */
    "WM_POWER", NULL,
    "WM_COPYDATA",
    "WM_CANCELJOURNAL", NULL, NULL,
    "WM_NOTIFY", NULL,

    /* 0x0050 */
    "WM_INPUTLANGCHANGEREQUEST",
    "WM_INPUTLANGCHANGE",
    "WM_TCARD",
    "WM_HELP",
    "WM_USERCHANGED",
    "WM_NOTIFYFORMAT", NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0060 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0070 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL,
    "WM_CONTEXTMENU",
    "WM_STYLECHANGING",
    "WM_STYLECHANGED",
    "WM_DISPLAYCHANGE",
    "WM_GETICON",

    "WM_SETICON",               /* 0x0080 */
    "WM_NCCREATE",              /* 0x0081 */
    "WM_NCDESTROY",             /* 0x0082 */
    "WM_NCCALCSIZE",            /* 0x0083 */
    "WM_NCHITTEST",             /* 0x0084 */
    "WM_NCPAINT",               /* 0x0085 */
    "WM_NCACTIVATE",            /* 0x0086 */
    "WM_GETDLGCODE",            /* 0x0087 */
    "WM_SYNCPAINT",
    "WM_SYNCTASK", NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0090 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00A0 */
    "WM_NCMOUSEMOVE",           /* 0x00a0 */
    "WM_NCLBUTTONDOWN",         /* 0x00a1 */
    "WM_NCLBUTTONUP",           /* 0x00a2 */
    "WM_NCLBUTTONDBLCLK",       /* 0x00a3 */
    "WM_NCRBUTTONDOWN",         /* 0x00a4 */
    "WM_NCRBUTTONUP",           /* 0x00a5 */
    "WM_NCRBUTTONDBLCLK",       /* 0x00a6 */
    "WM_NCMBUTTONDOWN",         /* 0x00a7 */
    "WM_NCMBUTTONUP",           /* 0x00a8 */
    "WM_NCMBUTTONDBLCLK",       /* 0x00a9 */
    NULL,                       /* 0x00aa */
    "WM_NCXBUTTONDOWN",         /* 0x00ab */
    "WM_NCXBUTTONUP",           /* 0x00ac */
    "WM_NCXBUTTONDBLCLK",       /* 0x00ad */
    NULL,                       /* 0x00ae */
    NULL,                       /* 0x00af */

    /* 0x00B0 - Win32 Edit controls */
    "EM_GETSEL",                /* 0x00b0 */
    "EM_SETSEL",                /* 0x00b1 */
    "EM_GETRECT",               /* 0x00b2 */
    "EM_SETRECT",               /* 0x00b3 */
    "EM_SETRECTNP",             /* 0x00b4 */
    "EM_SCROLL",                /* 0x00b5 */
    "EM_LINESCROLL",            /* 0x00b6 */
    "EM_SCROLLCARET",           /* 0x00b7 */
    "EM_GETMODIFY",             /* 0x00b8 */
    "EM_SETMODIFY",             /* 0x00b9 */
    "EM_GETLINECOUNT",          /* 0x00ba */
    "EM_LINEINDEX",             /* 0x00bb */
    "EM_SETHANDLE",             /* 0x00bc */
    "EM_GETHANDLE",             /* 0x00bd */
    "EM_GETTHUMB",              /* 0x00be */
    NULL,                       /* 0x00bf */

    NULL,                       /* 0x00c0 */
    "EM_LINELENGTH",            /* 0x00c1 */
    "EM_REPLACESEL",            /* 0x00c2 */
    NULL,                       /* 0x00c3 */
    "EM_GETLINE",               /* 0x00c4 */
    "EM_LIMITTEXT",             /* 0x00c5 */
    "EM_CANUNDO",               /* 0x00c6 */
    "EM_UNDO",                  /* 0x00c7 */
    "EM_FMTLINES",              /* 0x00c8 */
    "EM_LINEFROMCHAR",          /* 0x00c9 */
    NULL,                       /* 0x00ca */
    "EM_SETTABSTOPS",           /* 0x00cb */
    "EM_SETPASSWORDCHAR",       /* 0x00cc */
    "EM_EMPTYUNDOBUFFER",       /* 0x00cd */
    "EM_GETFIRSTVISIBLELINE",   /* 0x00ce */
    "EM_SETREADONLY",           /* 0x00cf */

    "EM_SETWORDBREAKPROC",      /* 0x00d0 */
    "EM_GETWORDBREAKPROC",      /* 0x00d1 */
    "EM_GETPASSWORDCHAR",       /* 0x00d2 */
    "EM_SETMARGINS",            /* 0x00d3 */
    "EM_GETMARGINS",            /* 0x00d4 */
    "EM_GETLIMITTEXT",          /* 0x00d5 */
    "EM_POSFROMCHAR",           /* 0x00d6 */
    "EM_CHARFROMPOS",           /* 0x00d7 */
    "EM_SETIMESTATUS",          /* 0x00d8 */
    "EM_GETIMESTATUS",          /* 0x00d9 */
    NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x00E0 - Win32 Scrollbars */
    "SBM_SETPOS",               /* 0x00e0 */
    "SBM_GETPOS",               /* 0x00e1 */
    "SBM_SETRANGE",             /* 0x00e2 */
    "SBM_GETRANGE",             /* 0x00e3 */
    "SBM_ENABLE_ARROWS",        /* 0x00e4 */
    NULL,
    "SBM_SETRANGEREDRAW",       /* 0x00e6 */
    NULL, NULL,
    "SBM_SETSCROLLINFO",        /* 0x00e9 */
    "SBM_GETSCROLLINFO",        /* 0x00ea */
    NULL, NULL, NULL, NULL, NULL,

    /* 0x00F0 - Win32 Buttons */
    "BM_GETCHECK",              /* 0x00f0 */
    "BM_SETCHECK",              /* 0x00f1 */
    "BM_GETSTATE",              /* 0x00f2 */
    "BM_SETSTATE",              /* 0x00f3 */
    "BM_SETSTYLE",              /* 0x00f4 */
    "BM_CLICK",                 /* 0x00f5 */
    "BM_GETIMAGE",              /* 0x00f6 */
    "BM_SETIMAGE",              /* 0x00f7 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_INPUT_DEVICE_CHANGE",   /* 0x00fe */
    "WM_INPUT",                 /* 0x00ff */

    "WM_KEYDOWN",               /* 0x0100 */
    "WM_KEYUP",                 /* 0x0101 */
    "WM_CHAR",                  /* 0x0102 */
    "WM_DEADCHAR",              /* 0x0103 */
    "WM_SYSKEYDOWN",            /* 0x0104 */
    "WM_SYSKEYUP",              /* 0x0105 */
    "WM_SYSCHAR",               /* 0x0106 */
    "WM_SYSDEADCHAR",           /* 0x0107 */
    NULL,
    "WM_UNICHAR",               /* 0x0109 */
    "WM_CONVERTREQUEST",        /* 0x010a */
    "WM_CONVERTRESULT",         /* 0x010b */
    "WM_INTERIM",               /* 0x010c */
    "WM_IME_STARTCOMPOSITION",  /* 0x010d */
    "WM_IME_ENDCOMPOSITION",    /* 0x010e */
    "WM_IME_COMPOSITION",       /* 0x010f */

    "WM_INITDIALOG",            /* 0x0110 */
    "WM_COMMAND",               /* 0x0111 */
    "WM_SYSCOMMAND",            /* 0x0112 */
    "WM_TIMER",                 /* 0x0113 */
    "WM_HSCROLL",               /* 0x0114 */
    "WM_VSCROLL",               /* 0x0115 */
    "WM_INITMENU",              /* 0x0116 */
    "WM_INITMENUPOPUP",         /* 0x0117 */
    "WM_SYSTIMER",              /* 0x0118 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_MENUSELECT",            /* 0x011f */

    "WM_MENUCHAR",              /* 0x0120 */
    "WM_ENTERIDLE",             /* 0x0121 */

    "WM_MENURBUTTONUP",         /* 0x0122 */
    "WM_MENUDRAG",              /* 0x0123 */
    "WM_MENUGETOBJECT",         /* 0x0124 */
    "WM_UNINITMENUPOPUP",       /* 0x0125 */
    "WM_MENUCOMMAND",           /* 0x0126 */
    "WM_CHANGEUISTATE",         /* 0x0127 */
    "WM_UPDATEUISTATE",         /* 0x0128 */
    "WM_QUERYUISTATE",          /* 0x0129 */

    NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0130 */
    NULL,
    "WM_LBTRACKPOINT",          /* 0x0131 */
    "WM_CTLCOLORMSGBOX",        /* 0x0132 */
    "WM_CTLCOLOREDIT",          /* 0x0133 */
    "WM_CTLCOLORLISTBOX",       /* 0x0134 */
    "WM_CTLCOLORBTN",           /* 0x0135 */
    "WM_CTLCOLORDLG",           /* 0x0136 */
    "WM_CTLCOLORSCROLLBAR",     /* 0x0137 */
    "WM_CTLCOLORSTATIC",        /* 0x0138 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0140 - Win32 Comboboxes */
    "CB_GETEDITSEL",            /* 0x0140 */
    "CB_LIMITTEXT",             /* 0x0141 */
    "CB_SETEDITSEL",            /* 0x0142 */
    "CB_ADDSTRING",             /* 0x0143 */
    "CB_DELETESTRING",          /* 0x0144 */
    "CB_DIR",                   /* 0x0145 */
    "CB_GETCOUNT",              /* 0x0146 */
    "CB_GETCURSEL",             /* 0x0147 */
    "CB_GETLBTEXT",             /* 0x0148 */
    "CB_GETLBTEXTLEN",          /* 0x0149 */
    "CB_INSERTSTRING",          /* 0x014a */
    "CB_RESETCONTENT",          /* 0x014b */
    "CB_FINDSTRING",            /* 0x014c */
    "CB_SELECTSTRING",          /* 0x014d */
    "CB_SETCURSEL",             /* 0x014e */
    "CB_SHOWDROPDOWN",          /* 0x014f */

    "CB_GETITEMDATA",           /* 0x0150 */
    "CB_SETITEMDATA",           /* 0x0151 */
    "CB_GETDROPPEDCONTROLRECT", /* 0x0152 */
    "CB_SETITEMHEIGHT",         /* 0x0153 */
    "CB_GETITEMHEIGHT",         /* 0x0154 */
    "CB_SETEXTENDEDUI",         /* 0x0155 */
    "CB_GETEXTENDEDUI",         /* 0x0156 */
    "CB_GETDROPPEDSTATE",       /* 0x0157 */
    "CB_FINDSTRINGEXACT",       /* 0x0158 */
    "CB_SETLOCALE",             /* 0x0159 */
    "CB_GETLOCALE",             /* 0x015a */
    "CB_GETTOPINDEX",           /* 0x015b */
    "CB_SETTOPINDEX",           /* 0x015c */
    "CB_GETHORIZONTALEXTENT",   /* 0x015d */
    "CB_SETHORIZONTALEXTENT",   /* 0x015e */
    "CB_GETDROPPEDWIDTH",       /* 0x015f */

    "CB_SETDROPPEDWIDTH",       /* 0x0160 */
    "CB_INITSTORAGE",           /* 0x0161 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0170 - Win32 Static controls */
    "STM_SETICON",              /* 0x0170 */
    "STM_GETICON",              /* 0x0171 */
    "STM_SETIMAGE",             /* 0x0172 */
    "STM_GETIMAGE",             /* 0x0173 */
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0180 - Win32 Listboxes */
    "LB_ADDSTRING",             /* 0x0180 */
    "LB_INSERTSTRING",          /* 0x0181 */
    "LB_DELETESTRING",          /* 0x0182 */
    "LB_SELITEMRANGEEX",        /* 0x0183 */
    "LB_RESETCONTENT",          /* 0x0184 */
    "LB_SETSEL",                /* 0x0185 */
    "LB_SETCURSEL",             /* 0x0186 */
    "LB_GETSEL",                /* 0x0187 */
    "LB_GETCURSEL",             /* 0x0188 */
    "LB_GETTEXT",               /* 0x0189 */
    "LB_GETTEXTLEN",            /* 0x018a */
    "LB_GETCOUNT",              /* 0x018b */
    "LB_SELECTSTRING",          /* 0x018c */
    "LB_DIR",                   /* 0x018d */
    "LB_GETTOPINDEX",           /* 0x018e */
    "LB_FINDSTRING",            /* 0x018f */

    "LB_GETSELCOUNT",           /* 0x0190 */
    "LB_GETSELITEMS",           /* 0x0191 */
    "LB_SETTABSTOPS",           /* 0x0192 */
    "LB_GETHORIZONTALEXTENT",   /* 0x0193 */
    "LB_SETHORIZONTALEXTENT",   /* 0x0194 */
    "LB_SETCOLUMNWIDTH",        /* 0x0195 */
    "LB_ADDFILE",               /* 0x0196 */
    "LB_SETTOPINDEX",           /* 0x0197 */
    "LB_GETITEMRECT",           /* 0x0198 */
    "LB_GETITEMDATA",           /* 0x0199 */
    "LB_SETITEMDATA",           /* 0x019a */
    "LB_SELITEMRANGE",          /* 0x019b */
    "LB_SETANCHORINDEX",        /* 0x019c */
    "LB_GETANCHORINDEX",        /* 0x019d */
    "LB_SETCARETINDEX",         /* 0x019e */
    "LB_GETCARETINDEX",         /* 0x019f */

    "LB_SETITEMHEIGHT",         /* 0x01a0 */
    "LB_GETITEMHEIGHT",         /* 0x01a1 */
    "LB_FINDSTRINGEXACT",       /* 0x01a2 */
    "LB_CARETON",               /* 0x01a3 */
    "LB_CARETOFF",              /* 0x01a4 */
    "LB_SETLOCALE",             /* 0x01a5 */
    "LB_GETLOCALE",             /* 0x01a6 */
    "LB_SETCOUNT",              /* 0x01a7 */
    "LB_INITSTORAGE",           /* 0x01a8 */
    "LB_ITEMFROMPOINT",         /* 0x01a9 */
    NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01B0 */
    NULL, NULL,
    "LB_GETLISTBOXINFO",         /* 0x01b2 */
    NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01C0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01D0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01E0 */
    NULL,
    "MN_GETHMENU",              /* 0x01E1 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x01F0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_MOUSEMOVE",             /* 0x0200 */
    "WM_LBUTTONDOWN",           /* 0x0201 */
    "WM_LBUTTONUP",             /* 0x0202 */
    "WM_LBUTTONDBLCLK",         /* 0x0203 */
    "WM_RBUTTONDOWN",           /* 0x0204 */
    "WM_RBUTTONUP",             /* 0x0205 */
    "WM_RBUTTONDBLCLK",         /* 0x0206 */
    "WM_MBUTTONDOWN",           /* 0x0207 */
    "WM_MBUTTONUP",             /* 0x0208 */
    "WM_MBUTTONDBLCLK",         /* 0x0209 */
    "WM_MOUSEWHEEL",            /* 0x020A */
    "WM_XBUTTONDOWN",           /* 0x020B */
    "WM_XBUTTONUP",             /* 0x020C */
    "WM_XBUTTONDBLCLK",         /* 0x020D */
    "WM_MOUSEHWHEEL",           /* 0x020E */
    NULL,

    "WM_PARENTNOTIFY",          /* 0x0210 */
    "WM_ENTERMENULOOP",         /* 0x0211 */
    "WM_EXITMENULOOP",          /* 0x0212 */
    "WM_NEXTMENU",              /* 0x0213 */
    "WM_SIZING",
    "WM_CAPTURECHANGED",
    "WM_MOVING", NULL,
    "WM_POWERBROADCAST",
    "WM_DEVICECHANGE", NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_MDICREATE",             /* 0x0220 */
    "WM_MDIDESTROY",            /* 0x0221 */
    "WM_MDIACTIVATE",           /* 0x0222 */
    "WM_MDIRESTORE",            /* 0x0223 */
    "WM_MDINEXT",               /* 0x0224 */
    "WM_MDIMAXIMIZE",           /* 0x0225 */
    "WM_MDITILE",               /* 0x0226 */
    "WM_MDICASCADE",            /* 0x0227 */
    "WM_MDIICONARRANGE",        /* 0x0228 */
    "WM_MDIGETACTIVE",          /* 0x0229 */

    "WM_DROPOBJECT",
    "WM_QUERYDROPOBJECT",
    "WM_BEGINDRAG",
    "WM_DRAGLOOP",
    "WM_DRAGSELECT",
    "WM_DRAGMOVE",

    /* 0x0230*/
    "WM_MDISETMENU",            /* 0x0230 */
    "WM_ENTERSIZEMOVE",         /* 0x0231 */
    "WM_EXITSIZEMOVE",          /* 0x0232 */
    "WM_DROPFILES",             /* 0x0233 */
    "WM_MDIREFRESHMENU", NULL, NULL, NULL,
    /* 0x0238*/
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0240 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0250 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0260 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x0280 */
    NULL,
    "WM_IME_SETCONTEXT",        /* 0x0281 */
    "WM_IME_NOTIFY",            /* 0x0282 */
    "WM_IME_CONTROL",           /* 0x0283 */
    "WM_IME_COMPOSITIONFULL",   /* 0x0284 */
    "WM_IME_SELECT",            /* 0x0285 */
    "WM_IME_CHAR",              /* 0x0286 */
    NULL,
    "WM_IME_REQUEST",           /* 0x0288 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_IME_KEYDOWN",           /* 0x0290 */
    "WM_IME_KEYUP",             /* 0x0291 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x02a0 */
    "WM_NCMOUSEHOVER",          /* 0x02A0 */
    "WM_MOUSEHOVER",            /* 0x02A1 */
    "WM_NCMOUSELEAVE",          /* 0x02A2 */
    "WM_MOUSELEAVE",            /* 0x02A3 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_WTSSESSION_CHANGE",     /* 0x02B1 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x02c0 */
    "WM_TABLET_FIRST",          /* 0x02c0 */
    "WM_TABLET_FIRST+1",        /* 0x02c1 */
    "WM_TABLET_FIRST+2",        /* 0x02c2 */
    "WM_TABLET_FIRST+3",        /* 0x02c3 */
    "WM_TABLET_FIRST+4",        /* 0x02c4 */
    "WM_TABLET_FIRST+5",        /* 0x02c5 */
    "WM_TABLET_FIRST+7",        /* 0x02c6 */
    "WM_TABLET_FIRST+8",        /* 0x02c7 */
    "WM_TABLET_FIRST+9",        /* 0x02c8 */
    "WM_TABLET_FIRST+10",       /* 0x02c9 */
    "WM_TABLET_FIRST+11",       /* 0x02ca */
    "WM_TABLET_FIRST+12",       /* 0x02cb */
    "WM_TABLET_FIRST+13",       /* 0x02cc */
    "WM_TABLET_FIRST+14",       /* 0x02cd */
    "WM_TABLET_FIRST+15",       /* 0x02ce */
    "WM_TABLET_FIRST+16",       /* 0x02cf */
    "WM_TABLET_FIRST+17",       /* 0x02d0 */
    "WM_TABLET_FIRST+18",       /* 0x02d1 */
    "WM_TABLET_FIRST+19",       /* 0x02d2 */
    "WM_TABLET_FIRST+20",       /* 0x02d3 */
    "WM_TABLET_FIRST+21",       /* 0x02d4 */
    "WM_TABLET_FIRST+22",       /* 0x02d5 */
    "WM_TABLET_FIRST+23",       /* 0x02d6 */
    "WM_TABLET_FIRST+24",       /* 0x02d7 */
    "WM_TABLET_FIRST+25",       /* 0x02d8 */
    "WM_TABLET_FIRST+26",       /* 0x02d9 */
    "WM_TABLET_FIRST+27",       /* 0x02da */
    "WM_TABLET_FIRST+28",       /* 0x02db */
    "WM_TABLET_FIRST+29",       /* 0x02dc */
    "WM_TABLET_FIRST+30",       /* 0x02dd */
    "WM_TABLET_FIRST+31",       /* 0x02de */
    "WM_TABLET_LAST",           /* 0x02df */

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_CUT",                   /* 0x0300 */
    "WM_COPY",
    "WM_PASTE",
    "WM_CLEAR",
    "WM_UNDO",
    "WM_RENDERFORMAT",
    "WM_RENDERALLFORMATS",
    "WM_DESTROYCLIPBOARD",
    "WM_DRAWCLIPBOARD",
    "WM_PAINTCLIPBOARD",
    "WM_VSCROLLCLIPBOARD",
    "WM_SIZECLIPBOARD",
    "WM_ASKCBFORMATNAME",
    "WM_CHANGECBCHAIN",
    "WM_HSCROLLCLIPBOARD",
    "WM_QUERYNEWPALETTE",       /* 0x030f*/

    "WM_PALETTEISCHANGING",
    "WM_PALETTECHANGED",
    "WM_HOTKEY",                /* 0x0312 */
    "WM_POPUPSYSTEMMENU",       /* 0x0313 */
    NULL, NULL, NULL,
    "WM_PRINT",                 /* 0x0317 */
    "WM_PRINTCLIENT",           /* 0x0318 */
    "WM_APPCOMMAND",            /* 0x0319 */
    "WM_THEMECHANGED",          /* 0x031A */
    NULL, NULL,
    "WM_CLIPBOARDUPDATE",       /* 0x031D */
    "WM_DWMCOMPOSITIONCHANGED", /* 0x031E */
    "WM_DWMNCRENDERINGCHANGED", /* 0x031F */

    "WM_DWMCOLORIZATIONCOLORCHANGED", /* 0x0320 */
    "WM_DWMWINDOWMAXIMIZEDCHANGE", /* 0x0321 */
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_GETTITLEBARINFOEX",     /* 0x033F */

    /* 0x0340 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    /* 0x0350 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_HANDHELDFIRST",     /* 0x0358 */
    "WM_HANDHELDFIRST+1",   /* 0x0359 */
    "WM_HANDHELDFIRST+2",   /* 0x035A */
    "WM_HANDHELDFIRST+3",   /* 0x035B */
    "WM_HANDHELDFIRST+4",   /* 0x035C */
    "WM_HANDHELDFIRST+5",   /* 0x035D */
    "WM_HANDHELDFIRST+6",   /* 0x035E */
    "WM_HANDHELDLAST",      /* 0x035F */

    "WM_QUERYAFXWNDPROC",   /*  0x0360 WM_AFXFIRST */
    "WM_SIZEPARENT",        /*  0x0361 */
    "WM_SETMESSAGESTRING",  /*  0x0362 */
    "WM_IDLEUPDATECMDUI",   /*  0x0363 */
    "WM_INITIALUPDATE",     /*  0x0364 */
    "WM_COMMANDHELP",       /*  0x0365 */
    "WM_HELPHITTEST",       /*  0x0366 */
    "WM_EXITHELPMODE",      /*  0x0367 */
    "WM_RECALCPARENT",      /*  0x0368 */
    "WM_SIZECHILD",         /*  0x0369 */
    "WM_KICKIDLE",          /*  0x036A */
    "WM_QUERYCENTERWND",    /*  0x036B */
    "WM_DISABLEMODAL",      /*  0x036C */
    "WM_FLOATSTATUS",       /*  0x036D */
    "WM_ACTIVATETOPLEVEL",  /*  0x036E */
    "WM_QUERY3DCONTROLS",   /*  0x036F */
    NULL,NULL,NULL,
    "WM_SOCKET_NOTIFY",     /*  0x0373 */
    "WM_SOCKET_DEAD",       /*  0x0374 */
    "WM_POPMESSAGESTRING",  /*  0x0375 */
    "WM_OCC_LOADFROMSTREAM",     /* 0x0376 */
    "WM_OCC_LOADFROMSTORAGE",    /* 0x0377 */
    "WM_OCC_INITNEW",            /* 0x0378 */
    "WM_QUEUE_SENTINEL",         /* 0x0379 */
    "WM_OCC_LOADFROMSTREAM_EX",  /* 0x037A */
    "WM_OCC_LOADFROMSTORAGE_EX", /* 0x037B */

    NULL,NULL,NULL,
    "WM_AFXLAST",               /* 0x037F */

    "WM_PENWINFIRST",           /* 0x0380 */
    "WM_RCRESULT",              /* 0x0381 */
    "WM_HOOKRCRESULT",          /* 0x0382 */
    "WM_GLOBALRCCHANGE",        /* 0x0383 */
    "WM_SKB",                   /* 0x0384 */
    "WM_HEDITCTL",              /* 0x0385 */
    NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_PENWINLAST",            /* 0x038F */

    "WM_COALESCE_FIRST",        /* 0x0390 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "WM_COALESCE_LAST",         /* 0x039F */

    /* 0x03a0 */
    "MM_JOY1MOVE",
    "MM_JOY2MOVE",
    "MM_JOY1ZMOVE",
    "MM_JOY2ZMOVE",
                            NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x03b0 */
    NULL, NULL, NULL, NULL, NULL,
    "MM_JOY1BUTTONDOWN",
    "MM_JOY2BUTTONDOWN",
    "MM_JOY1BUTTONUP",
    "MM_JOY2BUTTONUP",
    "MM_MCINOTIFY",
                NULL,
    "MM_WOM_OPEN",
    "MM_WOM_CLOSE",
    "MM_WOM_DONE",
    "MM_WIM_OPEN",
    "MM_WIM_CLOSE",

    /* 0x03c0 */
    "MM_WIM_DATA",
    "MM_MIM_OPEN",
    "MM_MIM_CLOSE",
    "MM_MIM_DATA",
    "MM_MIM_LONGDATA",
    "MM_MIM_ERROR",
    "MM_MIM_LONGERROR",
    "MM_MOM_OPEN",
    "MM_MOM_CLOSE",
    "MM_MOM_DONE",
                NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    /* 0x03e0 */
    "WM_DDE_INITIATE",  /* 0x3E0 */
    "WM_DDE_TERMINATE", /* 0x3E1 */
    "WM_DDE_ADVISE",    /* 0x3E2 */
    "WM_DDE_UNADVISE",  /* 0x3E3 */
    "WM_DDE_ACK",       /* 0x3E4 */
    "WM_DDE_DATA",      /* 0x3E5 */
    "WM_DDE_REQUEST",   /* 0x3E6 */
    "WM_DDE_POKE",      /* 0x3E7 */
    "WM_DDE_EXECUTE",   /* 0x3E8 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,


    /* 0x03f0 */
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

    "WM_USER"                   /* 0x0400 */
};

const char *get_message_name( UINT message )
{
	if (message > WM_USER)
		return NULL;
	return MessageTypeNames[ message ];
}
