#include "stdafx.h"
#include "Emu.h"

Core   *core;
Display*display;
Memory *memory;
Mapper *mapper;
Cpu    *cpu;
Ppu    *ppu;
Apu    *apu;
Pad    *pad;

HWND hMainWnd;

int X;
int Y;

struct MenuItem {

	int parent;
	int id;

	enum {
		Type_Normal = 0,
		Type_Checked,
		Type_Radio,
		Type_Popup,
		Type_Separator,
		
		Type_Disabled = 0x10000,

		Type_END = 0xFFFFFFFF
	} type;

	char caption[256];

	HMENU hPopupMenu; // defaults to 0;

} menuItems[] = {

	{   0,   1, MenuItem::Type_Popup, "File" },
		{    1,  101, MenuItem::Type_Normal, "Load ROM..." },
		{    1,  102, MenuItem::Type_Normal, "Load Recent" },
		{    1,  103, MenuItem::Type_Separator, "" },
		{    1,  104, MenuItem::Type_Normal, "Exit" },
	{   0,   2, MenuItem::Type_Popup, "Emulation" },
		{    2,  201, MenuItem::Type_Normal, "Start" },
		{    2,  202, MenuItem::Type_Normal, "Stop" },
		{    2,  203, MenuItem::Type_Normal, "Reset" },
        {    2,  204, MenuItem::Type_Normal, "Simulate Reset Button" },
	{   0,   3, MenuItem::Type_Popup, "Options" },
		{    3,  301, MenuItem::Type_Normal, "Limit Speed" },
		{    3,  302, MenuItem::Type_Separator, "" },
		{    3,  303, MenuItem::Type_Disabled, "(todo)" },
	{   0,   4, MenuItem::Type_Popup, "Help" },
		{    4,  401, MenuItem::Type_Disabled, "(nothing yet)" },

	{   0,   0, MenuItem::Type_END, "" },
};

HMENU hMenu = 0;

#define MENU_BASE 0x8000;

MenuItem* FindMenuItem(int menuId)
{
	for(int j=0;menuItems[j].type!=MenuItem::Type_END;j++)
	{
		if(menuItems[j].id == menuId)
		{
			return &(menuItems[j]);
		}
	}
	return NULL;
}

void CreateMenus(HWND hWnd)
{
	HMENU oldMenu = hMenu;

	hMenu = CreateMenu();

	for(int i=0;menuItems[i].type!=MenuItem::Type_END;i++)
	{
		UINT flags = 0;
		UINT_PTR menuId = (UINT_PTR)menuItems[i].id;

		HMENU menu = hMenu;

		if(menuItems[i].parent!=0)
		{
			MenuItem *item = FindMenuItem(menuItems[i].parent);

			if(item)
			{
				menu = item->hPopupMenu;
			}
		}

		switch(menuItems[i].type&0xFFFF)
		{
		case MenuItem::Type_Checked:
		case MenuItem::Type_Radio:
			flags |= MF_CHECKED;
			break;
		case MenuItem::Type_Popup:
			flags |= MF_POPUP;

			if(menuItems[i].hPopupMenu)
			{
				DestroyMenu(menuItems[i].hPopupMenu);
			}				
			menuItems[i].hPopupMenu = CreateMenu();
			menuId = (UINT_PTR)menuItems[i].hPopupMenu;

			break;
		case MenuItem::Type_Separator:
			flags |= MF_SEPARATOR;
			break;
		}

		if(menuItems[i].type&MenuItem::Type_Disabled)
		{
			flags|=MF_DISABLED;
		}

		AppendMenuA(menu, flags, menuId, menuItems[i].caption);
	}

	SetMenu(hWnd,hMenu);
	if(oldMenu!=0) DestroyMenu(oldMenu);
	DrawMenuBar(hWnd);
}

void LoadROM()
{
	char fName[1024]="";

	OPENFILENAMEA ofn = {
		sizeof(OPENFILENAME),
		hMainWnd,
		GetModuleHandle(NULL),  
		"NES Roms (*.nes)\0*.nes\0",
		NULL,
		0,
		0,
		fName,
		1023,
		NULL,0,
		NULL,0,
		OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST,
		0,0,
		NULL,
		0,NULL,NULL,0,
		0 /* OFN_EX_NOPLACESBAR */
	};

	if(GetOpenFileNameA(&ofn))
	{
		core->Init(ofn.lpstrFile);
	}
}

void HandleMenuClick(HWND hWnd, int menuId)
{
	MenuItem *pItem = FindMenuItem(menuId);

	if(!pItem)
		return;

	MenuItem &item(*pItem);

	switch(menuId)
	{
	case 101:
		LoadROM();
		break;

	case 104:
		PostQuitMessage( 0 );
		break;

	case 201:
		core->Start();
		break;
	case 202:
		core->Stop();
		break;
	case 203:
		core->Reset();
		break;
    case 204:
        core->SoftReset();
        break;

		//just for fun
	case 301:
		if(item.type == MenuItem::Type_Checked)
			item.type = MenuItem::Type_Normal;
		else
			item.type = MenuItem::Type_Checked;
		CreateMenus(hWnd);
		break;
	}
}

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_DESTROY:
		//Cleanup();
		PostQuitMessage( 0 );
		return 0;

	case WM_KEYDOWN:
		core->SetKey(wParam,1);
		if (wParam == VK_PAUSE)
		{
			core->TogglePause();
		}
		return 0;

	case WM_KEYUP:
		core->SetKey(wParam,0);
		return 0;

	case WM_MOUSEMOVE:

		if(wParam)
		{
			X=((unsigned int)lParam)&0xFFFF;
			Y=((unsigned int)lParam>>16);
		}
		break;

	case WM_COMMAND:
		if(HIWORD(wParam)==0)
		{
			// menu
			UINT menuId = LOWORD(wParam);

			if(menuId!=0)
			{
				HandleMenuClick(hWnd, menuId);
			}
		}
		return 0;


	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}

int _tmain(int argc, _TCHAR* argv[])
{
	HWND hWnd;

	// Register the window class
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		_T("DrawingWnd"), NULL };
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	RegisterClassEx( &wc );

	NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS), 0 };
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	// Create the application's window
	hWnd = CreateWindow(_T("DrawingWnd"),_T("OldGrayBrick"),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, WIDTH*3+ncm.iBorderWidth*2, HEIGHT*3+ncm.iBorderWidth*2+ncm.iCaptionHeight+ncm.iMenuHeight,
		GetDesktopWindow(), NULL, wc.hInstance, NULL );

	// Show the window
	ShowWindow( hWnd, SW_SHOWDEFAULT );
	UpdateWindow( hWnd );

	CreateMenus(hWnd);

	hMainWnd = hWnd;
	core = new Core();
	display=NULL;
	memory=NULL;
	mapper=NULL;
	cpu=NULL;
	ppu=NULL;
	apu=NULL;
	pad=NULL;

	//display = GetGDIDisplay();

	//display->Init(hWnd,false);

	// Enter the message loop
	MSG msg;
	ZeroMemory( &msg, sizeof(msg) );
	while( msg.message!=WM_QUIT )
	{
		if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			core->Emulate();
			//display->Render();
		}
	}

	//display->Close();

	core->Close();
	delete core;

	UnregisterClassA( "DrawingWnd", wc.hInstance );
	return 0;
}

