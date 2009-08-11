/*
 * Clock
 *
 * Copyright 1998 Marcel Baur <mbaur@g26.ethz.ch>
 *
 * Clock is partially based on
 * - Program Manager by Ulrich Schmied
 * - rolex.c by Jim Peterson
 *
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

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "windows.h"

#define REDUCE_FLICKER 0

#define INITIAL_WINDOW_SIZE 200
#define TIMER_ID 1

typedef struct
{
	HANDLE  hInstance;
	HWND    hMainWnd;
	HMENU   hMainMenu;

	BOOL    bAnalog;
	BOOL    bAlwaysOnTop;
	BOOL    bWithoutTitle;
	BOOL    bSeconds;
	BOOL    bDate;

	int     MaxX;
	int     MaxY;
} CLOCK_GLOBALS;

CLOCK_GLOBALS Globals;

//#define FaceColor (GetSysColor(COLOR_3DFACE))
//#define BackgroundColor (GetSysColor(COLOR_3DFACE))

static const int SHADOW_DEPTH = 2;

typedef struct {
	POINT Start;
	POINT End;
} HandData;

HandData HourHand, MinuteHand, SecondHand;

void dprintf(const char *format, ...)
{
    char str[0x100];
    va_list va;
    va_start( va, format );
    vsprintf( str, format, va );
    va_end( va );
    OutputDebugString( str );
}

static COLORREF HandColor(void)
{
    COLORREF color = GetSysColor(COLOR_3DHIGHLIGHT);
    dprintf("COLOR_3DHIGHLIGHT = %08lx\n", color);
    return color;
}

static COLORREF ShadowColor(void)
{
    COLORREF color = GetSysColor(COLOR_3DDKSHADOW);
    dprintf("COLOR_3DDKSHADOW = %08lx\n", color);
    return color;
}

static COLORREF TickColor(void)
{
    COLORREF color = GetSysColor(COLOR_3DHIGHLIGHT);
    dprintf("COLOR_3DHIGHLIGHT = %08lx\n", color);
    return color;
}

static void DrawTicks(HDC dc, const POINT* centre, int radius)
{
    int t;

    /* Minute divisions */
    if (radius>64)
        for(t=0; t<60; t++) {
            MoveToEx(dc,
                     centre->x + sin(t*M_PI/30)*0.9*radius,
                     centre->y - cos(t*M_PI/30)*0.9*radius,
                     NULL);
	    LineTo(dc,
		   centre->x + sin(t*M_PI/30)*0.89*radius,
		   centre->y - cos(t*M_PI/30)*0.89*radius);
	}

    /* Hour divisions */
    for(t=0; t<12; t++) {

        MoveToEx(dc,
                 centre->x + sin(t*M_PI/6)*0.9*radius,
                 centre->y - cos(t*M_PI/6)*0.9*radius,
                 NULL);
        LineTo(dc,
               centre->x + sin(t*M_PI/6)*0.8*radius,
               centre->y - cos(t*M_PI/6)*0.8*radius);
    }
}

static void DrawFace(HDC dc, const POINT* centre, int radius, int border)
{
    /* Ticks */
    SelectObject(dc, CreatePen(PS_SOLID, 2, ShadowColor()));
    dprintf("OffsetWindowOrgEx begin\n");
    OffsetWindowOrgEx(dc, -SHADOW_DEPTH, -SHADOW_DEPTH, NULL);
    dprintf("OffsetWindowOrgEx done\n");
    DrawTicks(dc, centre, radius);
    DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 2, TickColor())));
    OffsetWindowOrgEx(dc, SHADOW_DEPTH, SHADOW_DEPTH, NULL);
    DrawTicks(dc, centre, radius);
    if (border)
    {
        SelectObject(dc, GetStockObject(NULL_BRUSH));
        DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 5, ShadowColor())));
        Ellipse(dc, centre->x - radius, centre->y - radius, centre->x + radius, centre->y + radius);
    }
    DeleteObject(SelectObject(dc, GetStockObject(NULL_PEN)));
}

static void DrawHand(HDC dc,HandData* hand)
{
    MoveToEx(dc, hand->Start.x, hand->Start.y, NULL);
    LineTo(dc, hand->End.x, hand->End.y);
}

static void DrawHands(HDC dc, BOOL bSeconds)
{
    if (bSeconds) {
	SelectObject(dc, CreatePen(PS_SOLID, 1, HandColor()));
        DrawHand(dc, &SecondHand);
	DeleteObject(SelectObject(dc, GetStockObject(NULL_PEN)));
    }

    SelectObject(dc, CreatePen(PS_SOLID, 4, ShadowColor()));

    OffsetWindowOrgEx(dc, -SHADOW_DEPTH, -SHADOW_DEPTH, NULL);
    DrawHand(dc, &MinuteHand);
    DrawHand(dc, &HourHand);

    DeleteObject(SelectObject(dc, CreatePen(PS_SOLID, 4, HandColor())));
    OffsetWindowOrgEx(dc, SHADOW_DEPTH, SHADOW_DEPTH, NULL);
    DrawHand(dc, &MinuteHand);
    DrawHand(dc, &HourHand);

    DeleteObject(SelectObject(dc, GetStockObject(NULL_PEN)));
}

static void PositionHand(const POINT* centre, double length, double angle, HandData* hand)
{
    hand->Start = *centre;
    hand->End.x = centre->x + sin(angle)*length;
    hand->End.y = centre->y - cos(angle)*length;
}

static void PositionHands(const POINT* centre, int radius, BOOL bSeconds)
{
    SYSTEMTIME st;
    double hour, minute, second;

    /* 0 <= hour,minute,second < 2pi */
    /* Adding the millisecond count makes the second hand move more smoothly */

    GetLocalTime(&st);

    second = st.wSecond + st.wMilliseconds/1000.0;
    minute = st.wMinute + second/60.0;
    hour   = st.wHour % 12 + minute/60.0;

    PositionHand(centre, radius * 0.5,  hour/12   * 2*M_PI, &HourHand);
    PositionHand(centre, radius * 0.65, minute/60 * 2*M_PI, &MinuteHand);
    if (bSeconds)
        PositionHand(centre, radius * 0.79, second/60 * 2*M_PI, &SecondHand);
}

static void AnalogClock(HDC dc, int x, int y, BOOL bSeconds, BOOL border)
{
    POINT centre;
    int radius;

    radius = min(x, y)/2 - SHADOW_DEPTH;
    if (radius < 0)
	return;

    centre.x = x/2;
    centre.y = y/2;

    DrawFace(dc, &centre, radius, border);

    PositionHands(&centre, radius, bSeconds);
    DrawHands(dc, bSeconds);
}

/***********************************************************************
 *
 *           CLOCK_ResetTimer
 */
static BOOL CLOCK_ResetTimer(void)
{
    UINT period; /* milliseconds */

    KillTimer(Globals.hMainWnd, TIMER_ID);

    if (Globals.bSeconds)
	if (Globals.bAnalog)
	    period = 50;
	else
	    period = 500;
    else
	period = 1000;

    return SetTimer (Globals.hMainWnd, TIMER_ID, period, NULL);
}

/***********************************************************************
 *
 *           CLOCK_Paint
 */
static VOID CLOCK_Paint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hWnd, &ps);
#if REDUCE_FLICKER
    HDC dcMem;
    HBITMAP bmMem, bmOld;

    /* Use an offscreen dc to avoid flicker */
    dcMem = CreateCompatibleDC(dc);
    bmMem = CreateCompatibleBitmap(dc, ps.rcPaint.right - ps.rcPaint.left,
				    ps.rcPaint.bottom - ps.rcPaint.top);

    bmOld = SelectObject(dcMem, bmMem);

    SetViewportOrgEx(dcMem, -ps.rcPaint.left, -ps.rcPaint.top, NULL);
    /* Erase the background */
    FillRect(dcMem, &ps.rcPaint, GetSysColorBrush(COLOR_3DFACE));

    AnalogClock(dcMem, Globals.MaxX, Globals.MaxY, Globals.bSeconds, Globals.bWithoutTitle);

    /* Blit the changes to the screen */
    BitBlt(dc,
	   ps.rcPaint.left, ps.rcPaint.top,
	   ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
           dcMem,
	   ps.rcPaint.left, ps.rcPaint.top,
           SRCCOPY);

    SelectObject(dcMem, bmOld);
    DeleteObject(bmMem);
    DeleteDC(dcMem);
#else
    AnalogClock(dc, Globals.MaxX, Globals.MaxY, Globals.bSeconds, Globals.bWithoutTitle);
#endif

    EndPaint(hWnd, &ps);
}

/***********************************************************************
 *
 *           CLOCK_WndProc
 */

static LRESULT WINAPI CLOCK_WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	/* L button drag moves the window */
        case WM_NCHITTEST: {
	    LRESULT ret = DefWindowProc (hWnd, msg, wParam, lParam);
	    if (ret == HTCLIENT)
		ret = HTCAPTION;
            return ret;
	}

        case WM_PAINT: {
	    CLOCK_Paint(hWnd);
            break;
        }

        case WM_SIZE: {
            Globals.MaxX = LOWORD(lParam);
            Globals.MaxY = HIWORD(lParam);
            break;
        }

        case WM_TIMER: {
            /* Could just invalidate what has changed,
             * but it doesn't really seem worth the effort
             */
	    InvalidateRect(Globals.hMainWnd, NULL, !REDUCE_FLICKER);
	    break;
        }

        case WM_DESTROY: {
            PostQuitMessage (0);
            break;
        }

        default:
            return DefWindowProc (hWnd, msg, wParam, lParam);
    }
    return 0;
}


/***********************************************************************
 *
 *           WinMain
 */

int PASCAL WinMain (HINSTANCE hInstance, HINSTANCE prev, LPSTR cmdline, int show)
{
    MSG      msg;
    WNDCLASS class;

    static const char szClassName[] = "CLClass";  /* To make sure className >= 0x10000 */
    static const char szWinName[]   = "Clock";

    /* Setup Globals */
    Globals.bAnalog         = TRUE;
    Globals.bSeconds        = FALSE;

    if (!prev){
        class.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        class.lpfnWndProc   = CLOCK_WndProc;
        class.cbClsExtra    = 0;
        class.cbWndExtra    = 0;
        class.hInstance     = hInstance;
        class.hIcon         = LoadIcon (0, IDI_APPLICATION);
        class.hCursor       = LoadCursor (0, IDC_ARROW);
        class.hbrBackground = GetStockObject( BLACK_BRUSH );
        class.lpszMenuName  = 0;
        class.lpszClassName = szClassName;
    }

    if (!RegisterClass (&class)) return FALSE;

    Globals.MaxX = Globals.MaxY = INITIAL_WINDOW_SIZE;
    Globals.hMainWnd = CreateWindow (szClassName, szWinName, WS_OVERLAPPEDWINDOW,
                                     CW_USEDEFAULT, CW_USEDEFAULT,
                                     Globals.MaxX, Globals.MaxY, 0,
                                     0, hInstance, 0);

    if (!CLOCK_ResetTimer())
        return FALSE;

    ShowWindow (Globals.hMainWnd, show);
    UpdateWindow (Globals.hMainWnd);

    while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(Globals.hMainWnd, TIMER_ID);

    return 0;
}
