#include "stdafx.h"
#include <Windows.h>
#include <mmsystem.h>
#include "Emu.h"

HWND hWnd;
BITMAPINFO bmi;

#define HEIGHT 240
#define WIDTH  256

unsigned int X=0;
unsigned int Y=0;

int button;
int frames;
int ticks;

u32 buffer[256*240];

class GDIDisplay: public Display
{
private:
	HWND parent;

public:
	virtual bool Init(HWND displayWindow, bool fullScreen)
	{
		bmi.bmiHeader.biSize=sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biBitCount=32;
		bmi.bmiHeader.biWidth=WIDTH;
		bmi.bmiHeader.biHeight=HEIGHT;
		bmi.bmiHeader.biCompression=BI_RGB;
		bmi.bmiHeader.biClrUsed=0;
		bmi.bmiHeader.biClrImportant=0;
		bmi.bmiHeader.biPlanes=1;
		bmi.bmiHeader.biSizeImage=0;

		hWnd = displayWindow;
		return true;
	}

	virtual bool Render()
	{
		//render into buffer here

		RECT cRect;

		GetClientRect(hWnd,&cRect);

		int hSize = cRect.right-cRect.left;
		int vSize = cRect.bottom-cRect.top;

		int hRatio = 5;
		int vRatio = 4;

		int hNeeded,hOffset;
		int vNeeded,vOffset;

		RECT bar1; int drawBar1=0;
		RECT bar2; int drawBar2=0;
		if(hSize*vRatio >= vSize*hRatio)
		{
			// horizontally centered, vertical bars on sides
			vNeeded=vSize;
			hNeeded=vSize*hRatio/vRatio;
			vOffset=0;
			hOffset=(hSize-hNeeded)/2;

			if(hOffset>0)
			{
				drawBar1=drawBar2=1;
				bar1.top=0;
				bar1.left=0;
				bar1.right=hOffset;
				bar1.bottom=cRect.bottom;
			}
			bar2.top=0;
			bar2.left=hOffset+hNeeded;
			bar2.right=cRect.right;
			bar2.bottom=cRect.bottom;
			if(bar2.right<=bar2.left) drawBar2=0; // just to keep it clean :P
		}
		else
		{
			// vertically centered, horizontal bars top/bottom
			vNeeded=hSize*vRatio/hRatio;
			hNeeded=hSize;
			vOffset=(vSize-vNeeded)/2;
			hOffset=0;

			if(vOffset>0)
			{
				drawBar1=drawBar2=1;
				bar1.top=0;
				bar1.left=0;
				bar1.right=cRect.right;
				bar1.bottom=vOffset;
			}
			bar2.top=vOffset+vNeeded;
			bar2.left=0;
			bar2.right=cRect.right;
			bar2.bottom=cRect.bottom;
			if(bar2.bottom<=bar2.top) drawBar2=0; // just to keep it clean :P
		}

		HDC winDC=GetDC(hWnd);

		if(drawBar1) FillRect(winDC,&bar1,(HBRUSH)GetStockObject(BLACK_BRUSH));
		if(drawBar2) FillRect(winDC,&bar2,(HBRUSH)GetStockObject(BLACK_BRUSH));

		SetStretchBltMode(winDC,HALFTONE);
		SetBrushOrgEx(winDC,0,0,NULL);

		StretchDIBits(
			winDC,
			hOffset,vOffset,hNeeded,vNeeded,
			0,0,WIDTH,HEIGHT,
			buffer,
			&bmi,
			DIB_RGB_COLORS,
			SRCCOPY
		);

		ReleaseDC(hWnd,winDC);

		frames++;
		
		int tickdiff=GetTickCount()-ticks;
		if(tickdiff>=1000)
		{
			static char m1[100];
			float fps=(frames*1000.0)/tickdiff;
			sprintf(m1,"%d frames rendered in %d ms = %1.3f fps.", frames,tickdiff,fps);
			SetWindowTextA(hWnd,m1);
			frames=0;
			ticks=GetTickCount();
		}

		return true;
	}

	virtual void Close()
	{
	}

	virtual void SetScanline(int line, int *data)
	{
		memcpy(buffer+256*(239-line),data,256*4);
	}

} gdiDisplay;

Display *GetGDIDisplay()
{
	return &gdiDisplay;
}
