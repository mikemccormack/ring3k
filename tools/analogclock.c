/*
 * Analog Clock Test for GDI Operation
 *
 * Copyright 2009 Hilary Cheng
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

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <time.h>

#ifndef MONITOR_DEFAULTTONEAREST
#define MONITOR_DEFAULTTONEAREST    0x00000002
#endif

#define WINE_TRACE dprintf

static const char appname[] = "ACLOCK";

void dprintf( const char *format, ... )
{
	char str[0x100];
	va_list va;
	int len;

	va_start( va, format );
	vsprintf( str, format, va );
	va_end( va );
	len = strlen(str);
	if (len && str[len - 1] == '\n')
		str[len-1] = 0;
	OutputDebugString( str );
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT pPaint;
    HDC         hDC;
    HPEN        hPen, hOldPen;
    HBRUSH      hBrush;

	switch (uMsg) {
	case WM_CREATE:
		return 0;

	case WM_PAINT:
//        dprintf("-----------SelectObjectA\n");
        hDC = BeginPaint(hWnd, &pPaint);
//        SelectObject( hDC, GetStockObject(WHITE_PEN) );
//        dprintf("-----------SelectObject0 %08lx\n", hDC);
        hPen = CreatePen(PS_DOT, 0x44, RGB(0x33, 0x22, 0x11));
        hBrush = CreateSolidBrush(RGB(0x55, 0x66, 0x77));
        SelectObject(hDC, hBrush);
        SelectObject(hDC, hPen);
//        dprintf("-----------SelectObject1\n");
//        hOldPen = SelectObject(hDC, GetStockObject(WHITE_PEN));
//        dprintf("-----------SelectObject1\n");
//        dprintf("-----------SelectObject2\n");
//        SelectObject(hDC, GetStockObject(BLACK_BRUSH));
//        dprintf(" HPEN : %08X %08X\n", hPen, hDC);
//        DWORD p = *((DWORD *) hDC);
//        dprintf(" DC   : %08X\n", p);
/*
        UCHAR *ptr = (UCHAR *) p;
        {
            int count;
            for (count = 0; count < 32; count++) {
                dprintf(" C : %02X\n", ptr[count]);
            }
        }
*/
/*
        SetBkColor(hDC, RGB(0x11, 0x22, 0x33));
        SetTextColor(hDC, RGB(0x44, 0x55, 0x66));
*/
        MoveToEx( hDC, 0, 0, NULL );
        LineTo( hDC, 100, 100 );
        Rectangle( hDC, 200, 200, 300, 300 );
//        SelectObject(hDC, hOldPen);
        EndPaint(hWnd, &pPaint);
        DeleteObject(hPen);
        DeleteObject(hBrush);
        return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// this is required when we're replacing winlogon
void init_window_station( void )
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hwsta, hdesk;

	sa.nLength = sizeof sa;
	sa.lpSecurityDescriptor = 0;
	sa.bInheritHandle = TRUE;

	hwsta = CreateWindowStationW( L"winsta0", 0, MAXIMUM_ALLOWED, &sa );
	SetProcessWindowStation( hwsta );
	hdesk = CreateDesktopW( L"Winlogon", 0, 0, 0, MAXIMUM_ALLOWED, &sa );
	SetThreadDesktop( hdesk );
}

int APIENTRY WinMain( HINSTANCE Instance, HINSTANCE Prev, LPSTR CmdLine, int Show )
{
	// running as winlogon.exe?
	HWINSTA hwsta = GetProcessWindowStation();
	if (!hwsta)
		init_window_station();

	WNDCLASS wc;
	MSG msg;
	HWND hWnd;

    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = Instance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor( 0, IDI_APPLICATION );
    wc.hbrBackground = (HBRUSH) GetStockObject( BLACK_BRUSH );
    wc.lpszMenuName = NULL;
    wc.lpszClassName = appname;
    if (!RegisterClass(&wc)) ExitProcess(1);

    hWnd = CreateWindow(appname, appname, WS_VISIBLE | WS_POPUP | WS_DLGFRAME,
                    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                    0, 0, Instance, NULL);
    if (!hWnd) ExitProcess(1);

    ShowWindow( hWnd, Show );
    UpdateWindow( hWnd );

	while (GetMessage(&msg, 0, 0, 0)) {
        TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

