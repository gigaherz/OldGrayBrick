#include "StdAfx.h"
#include "Emu.h"

unsigned char ppu_palette[][3] = {
	{124,124,124}, 
	{  0,  0,252}, {  0,  0,188}, { 68, 40,188}, {148,  0,132},
	{168,  0, 32}, {168, 16,  0}, {136, 20,  0}, { 80, 48,  0},
	{  0,120,  0}, {  0,104,  0}, {  0, 88,  0}, {  0, 64, 88},
	{  0,  0,  0},
	{  0,  0,  0}, {  0,  0,  0},

	{188,188,188},
	{  0,120,248}, {  0, 88,248}, {104, 68,252}, {216,  0,204},
	{228,  0, 88}, {248, 56,  0}, {228, 92, 16}, {172,124,  0},
	{  0,184,  0}, {  0,168,  0}, {  0,168, 68}, {  0,136,136},
	{  0,  0,  0},
	{  0,  0,  0}, {  0,  0,  0},

	{248,248,248}, 
	{ 60,188,252}, {104,136,252}, {152,120,248}, {248,120,248},
	{248, 88,152}, {248,120, 88}, {252,160, 68}, {248,184,  0},
	{184,248, 24}, { 88,216, 84}, { 88,248,152}, {  0,232,216},
	{120,120,120},
	{  0,  0,  0}, {  0,  0,  0},

	{252,252,252}, 
	{164,228,252}, {184,184,248}, {216,184,248}, {248,184,248},
	{248,164,192}, {240,208,176}, {252,224,168}, {248,216,120},
	{216,248,120}, {184,248,184}, {184,248,216}, {  0,252,252},
	{248,216,248},
	{  0,  0,  0}, {  0,  0,  0},
};

Ppu::Ppu(void)
{
}

Ppu::~Ppu(void)
{
}

void Ppu::Init(int mode) // ntsc/pal -> ntsc = shows only 224 lines isntead of 240, top and bottom 8 are hidden
{
	Reset();
	vmode=mode;
}

#define PAL_LUMA_0	(0<<4)
#define PAL_LUMA_1	(1<<4)
#define PAL_LUMA_2	(2<<4)
#define PAL_LUMA_3	(3<<4)
#define PAL_CHROMA_WHITE	0
#define PAL_CHROMA_BLUE		1
#define PAL_CHROMA_VIOLET	2
#define PAL_CHROMA_MAGENTA	3
#define PAL_CHROMA_BROWN	4
#define PAL_CHROMA_RED		5
#define PAL_CHROMA_ORANGE	6
#define PAL_CHROMA_YELLOW	7
#define PAL_CHROMA_OLIVE	8
#define PAL_CHROMA_GREEN	9
#define PAL_CHROMA_GREEN2	10
#define PAL_CHROMA_CYAN		11
#define PAL_CHROMA_BLUE2	12
#define PAL_CHROMA_GRAY		13
#define PAL_CHROMA_BLACK1	14
#define PAL_CHROMA_BLACK2	15

void Ppu::Reset()
{
	memset(vram,0,sizeof(vram));
	memset(pal_ram,0,sizeof(pal_ram));
	memset(spr,0,sizeof(spr));

  //3F00h        Background Color (Color 0)
	pal_ram[ 0] = PAL_LUMA_0 | PAL_CHROMA_BLACK1;
  //3F01h-3F03h  Background Palette 0 (Color 1-3)
	pal_ram[ 1] = PAL_LUMA_2 | PAL_CHROMA_GRAY;
	pal_ram[ 2] = PAL_LUMA_1 | PAL_CHROMA_WHITE;
	pal_ram[ 3] = PAL_LUMA_3 | PAL_CHROMA_WHITE;
  //3F05h-3F07h  Background Palette 1 (Color 1-3)
	pal_ram[ 5] = PAL_LUMA_2 | PAL_CHROMA_GRAY;
	pal_ram[ 6] = PAL_LUMA_1 | PAL_CHROMA_WHITE;
	pal_ram[ 7] = PAL_LUMA_3 | PAL_CHROMA_WHITE;
  //3F09h-3F0Bh  Background Palette 2 (Color 1-3)
	pal_ram[ 9] = PAL_LUMA_2 | PAL_CHROMA_GRAY;
	pal_ram[10] = PAL_LUMA_1 | PAL_CHROMA_WHITE;
	pal_ram[11] = PAL_LUMA_3 | PAL_CHROMA_WHITE;
  //3F0Dh-3F0Fh  Background Palette 3 (Color 1-3)
	pal_ram[13] = PAL_LUMA_2 | PAL_CHROMA_GRAY;
	pal_ram[14] = PAL_LUMA_1 | PAL_CHROMA_WHITE;
	pal_ram[15] = PAL_LUMA_3 | PAL_CHROMA_WHITE;
  //3F11h-3F13h  Sprite Palette 0 (Color 1-3)
	pal_ram[17] = PAL_LUMA_1 | PAL_CHROMA_YELLOW;
	pal_ram[18] = PAL_LUMA_2 | PAL_CHROMA_YELLOW;
	pal_ram[19] = PAL_LUMA_3 | PAL_CHROMA_YELLOW;
  //3F15h-3F17h  Sprite Palette 1 (Color 1-3)
	pal_ram[21] = PAL_LUMA_1 | PAL_CHROMA_BLUE;
	pal_ram[22] = PAL_LUMA_2 | PAL_CHROMA_BLUE;
	pal_ram[23] = PAL_LUMA_3 | PAL_CHROMA_BLUE;
  //3F19h-3F1Bh  Sprite Palette 2 (Color 1-3)
	pal_ram[25] = PAL_LUMA_1 | PAL_CHROMA_GREEN;
	pal_ram[26] = PAL_LUMA_2 | PAL_CHROMA_GREEN;
	pal_ram[27] = PAL_LUMA_3 | PAL_CHROMA_GREEN;
  //3F1Dh-3F1Fh  Sprite Palette 3 (Color 1-3)
	pal_ram[29] = PAL_LUMA_1 | PAL_CHROMA_RED;
	pal_ram[30] = PAL_LUMA_2 | PAL_CHROMA_RED;
	pal_ram[31] = PAL_LUMA_3 | PAL_CHROMA_RED;

	tClocks = 0;
	scanline=0;
	ctrl1=0;
	ctrl2=0;
	scroll1=0;
	scroll2=0;
	spr_addr=0;
	vram_addr=0;
	status=0;
}

void Ppu::SoftReset()
{
    Reset();
}


void Ppu::Close()
{
}

void Ppu::Emulate(u32 clocks)
{
	//Item              NTSC             PAL
	//Video Clock       21.47727MHz      26.601712MHz
	//CPU Clock         1.7897725MHz     1.7734474MHz
	//Clock Divider     CPU=Video/12     CPU=Video/15
	//Cycles/Scanline   113.66; 1364/12  106.53; 1598/15
	//Total Scanlines   262 (240+22)     312 (240+72)
	//Frame Rate        60.098Hz         53.355Hz

	float scanline_clocks;

	int scanline_last;


	// accumulate clocks
	if(vmode) // PAL
	{
		tClocks += clocks * 15;
		scanline_clocks = 1598;
		scanline_last = 311;
	}
	else
	{
		tClocks += clocks * 12;
		scanline_clocks = 1364;
		scanline_last = 261;
	}

	if(tClocks >= scanline_clocks)
	{
		tClocks -= scanline_clocks;

		if(scanline==19)
		{
			status&=0x7f;
		}

		if((scanline>20)&&(scanline<261))
		{
			int sprites_address = (ctrl1&0x08)<<9; // 0x0000/0x1000 
			int pattern_address = (ctrl1&0x10)<<8; // 0x0000/0x1000 

			int spr_layer = (ctrl2&0x10)>>8;
			int bg_layer = (ctrl2&0x08)>>7;
			int spr_clip = (ctrl2&0x04)>>1;
			int bg_clip = (ctrl2&0x02)>>2;
			int bg_mono = (ctrl2&0x01);

			int x_scroll = scroll1 + (ctrl1&1)*256;
			int y_scroll = (ctrl1&2)*120; // 240 = 2*120

			if(scroll2>=240)
				y_scroll += 240-scroll2;
			else
				y_scroll += scroll2;


			int y_pos = scanline-21;
			int y_offset = (y_pos + y_scroll);

			int y_tile = y_offset>>3;
			int y_pixel = y_offset&7;

			int table_y = y_offset/240;
			int y_off = (y_tile%30);

			int attr_y = (y_off>>2);

#define PIXEL_BASED 1
#ifdef PIXEL_BASED
			int pixels[256];

			for(int x=0;x<256;x++)
			{
				int x_offset = (x_scroll+x);

				int x_tile = x_offset>>3;
				int x_pixel = x_offset&7;

				int table_x = x_offset>>8;
				int x_off = (x_tile&31);

				int name_offset = 0x2000 + ((table_x&1)<<10) + ((table_y&1)<<11);

				int tile_number = ((y_off<<5) + x_off);

				int name_addr = name_offset + tile_number;
				int name = ReadPPU(name_addr);

				int pattern0 = ReadPPU(pattern_address + name*16 + 0 + y_pixel);
				int pattern1 = ReadPPU(pattern_address + name*16 + 8 + y_pixel);

				int attr_x = (x_off>>2); // 32tiles=>5 bits. 5-2 = 3 bits

				int attribute_addr = name_offset + 0x03c0 + (attr_y<<3) + attr_x;
				int attribute = ReadPPU(attribute_addr);
				int attr_sel = ((y_tile&2)<<1)+(x_tile&2);
				int attr = (attribute>>attr_sel)&3;

				int color0 = (pattern0>>(7-x_pixel))&1;
				int color1 = (pattern1>>(7-x_pixel))&1;
				int color = color0 | (color1<<1);

				int pal=0;

				if(color==0)
				{
					pal = ReadPPU(0x3F00);
				}
				else
				{
					pal = ReadPPU(0x3F00 + 4*attr + color);
				}

				int r = min(255,((int)ppu_palette[pal][0]*(256 + 32*((ctrl2>>6)&1)))>>8);
				int g = min(255,((int)ppu_palette[pal][1]*(256 + 32*((ctrl2>>5)&1)))>>8);
				int b = min(255,((int)ppu_palette[pal][2]*(256 + 32*((ctrl2>>7)&1)))>>8);

				pixels[x] = b | (g<<8) | (r<<16);
			}
			display->SetScanline(y_pos,pixels);
#else
			// will render the whole scanline tile by tile, and then compensate for the offset
			int scan[256+16];

			int first_tile = x_scroll>>3;
			
			for(int x=0;x<33;x++)
			{
				int x_tile = first_tile+x;
				int x_offset = x_tile*8;

				int table_x = x_offset>>8;
				int x_off = (x_tile&31);

				int name_offset = 0x2000; // + ((table_x&1)<<10) + ((table_y&1)<<11);

				int tile_number = ((y_off*32) + x_off);

				int name_addr = name_offset + tile_number;
				int name = ReadPPU(name_addr);

				int pattern0 = ReadPPU(pattern_address + name*16 + 0 + y_pixel);
				int pattern1 = ReadPPU(pattern_address + name*16 + 8 + y_pixel);

				int attr_x = (x_off>>2); // 32tiles=>5 bits. 5-2 = 3 bits

				int attribute_addr = name_offset + 0x03c0 + ((attr_y<<3) + attr_x);
				int attribute = ReadPPU(attribute_addr);
				int attr_sel = ((y_tile&2)<<1)+(x_tile&2);
				int attr = (attribute>>attr_sel)&3;

				int *ts = scan+(x*8);

				for(int x_pixel=0;x_pixel<8;x_pixel++)
				{
					int color0 = (pattern0>>(7-x_pixel))&1;
					int color1 = (pattern1>>(7-x_pixel))&1;
					int color = color0 | (color1<<1);

					int pal=0;

					if(color==0)
					{
						pal = ReadPPU(0x3F00);
					}
					else
					{
						pal = ReadPPU(0x3F00 + 4*attr + color);
					}

					int r = min(255,((int)ppu_palette[pal][0]*(256 + 32*((ctrl2>>6)&1)))>>8);
					int g = min(255,((int)ppu_palette[pal][1]*(256 + 32*((ctrl2>>5)&1)))>>8);
					int b = min(255,((int)ppu_palette[pal][2]*(256 + 32*((ctrl2>>7)&1)))>>8);

					ts[x_pixel] = b | (g<<8) | (r<<16);
				}
			}

			int* line = scan+(x_scroll&7);
			display->SetScanline(y_pos,line);
#endif
		}

		scanline++;

		if(scanline==261)
		{
			status|=0x80;
			if(ctrl1>>7)
				cpu->NMI();
			core->DoVSync();

			spr_addr = 0;
		}

		if(scanline==scanline_last)
		{
			scanline=0;
			frame++;

#define TIME_PER_FRAME_PAL (1000.0f/53.355f)
#define TIME_PER_FRAME_NTSC (1000.0f/60.098f)

            float time_per_frame =
                vmode ? TIME_PER_FRAME_PAL : TIME_PER_FRAME_NTSC;

            static int ticks_last = 0;
            static float elapsed_acc = 0;
            do {
                int ticks_now = GetTickCount();
                int elapsed = ticks_now - ticks_last;
                ticks_last = ticks_now;
                elapsed_acc += elapsed;
                if (elapsed_acc < time_per_frame)
                {
                    SleepEx(max(1,(int)(time_per_frame - elapsed_acc)), TRUE);
                }
                else {
                    elapsed_acc -= time_per_frame;
                    if (elapsed_acc > 10 * time_per_frame)
                        elapsed_acc = 0;
                    break;
                }
            } while (1);
		}
	}

}
//
//void Ppu::Emulate(u32 clocks)
//{
//	//Item              NTSC             PAL
//	//Video Clock       21.47727MHz      26.601712MHz
//	//CPU Clock         1.7897725MHz     1.7734474MHz
//	//Clock Divider     CPU=Video/12     CPU=Video/15
//	//Cycles/Scanline   113.66; 1364/12  106.53; 1598/15
//	//Total Scanlines   262 (240+22)     312 (240+72)
//	//Frame Rate        60.098Hz         53.355Hz
//
//	float scanline_clocks;
//
//	int scanline_last;
//
//	// accumulate clocks
//	if(vmode) // PAL
//	{
//		tClocks += clocks * 15;
//		scanline_clocks = 1598;
//		scanline_last = 311;
//	}
//	else
//	{
//		tClocks += clocks * 12;
//		scanline_clocks = 1364;
//		scanline_last = 261;
//	}
//	
//	if(tClocks >= scanline_clocks)
//	{
//		tClocks -= scanline_clocks;
//
//		if(scanline==19)
//		{
//			status&=0x7f;
//		}
//
//		if((scanline>20)&&(scanline<261))
//		{
//			int sprites_offset = (ctrl1&0x08)<<9;
//			int pattern_offset = (ctrl1&0x10)<<8;
//
//			int names_offset = 0x2000 + ((ctrl1&3)<<10);
//			int attr_offset  = 0x03C0 + names_offset;
//
//			int spr_layer = (ctrl2&0x10)>>8;
//			int bg_layer = (ctrl2&0x08)>>7;
//			int spr_clip = (ctrl2&0x04)>>1;
//			int bg_clip = (ctrl2&0x02)>>2;
//			int bg_mono = (ctrl2&0x01);
//
//			int table_x = (ctrl1&1)*256;
//			int table_y = (ctrl1&2)*120; // 240 = 2*120
//
//			int x_offset = scroll1;
//			int y_offset = scroll2;
//
//			if(y_offset>=240)
//			{
//				y_offset = 240-y_offset;
//			}
//
//			int y_pos = scanline-21;
//			int y = ((table_y + y_pos + y_offset)%480);
//
//			int y_tile = y>>3;
//			int tile_y = y&7;
//

//		}
//
//		scanline++;
//
//		if(scanline==261)
//		{
//			status|=0x80;
//			if(ctrl1>>7)
//				cpu->NMI();
//			core->DoVSync();
//
//			spr_addr = 0;
//		}
//
//		if(scanline==scanline_last)
//		{
//			scanline=0;
//			frame++;
//		}
//	}
//	
//}

void Ppu::WriteVRAM(u16 addr, u8 value)
{
	vram[addr&2047]=value;
}

u8   Ppu::ReadVRAM (u16 addr)
{
	return vram[addr&2047];
}


void Ppu::WritePPU(u16 addr, u8 value)
{
	if(addr<0x3f00)
	{
		mapper->WritePPU(addr,value);
	}
	else
	{
		pal_ram[addr&0x1f] = value;
	}
}

u8   Ppu::ReadPPU (u16 addr)
{
	if(addr<0x3f00)
	{
		return mapper->ReadPPU(addr);
	}
	else
	{
		return pal_ram[addr&0x1f];
	}
}

void Ppu::Write(u16 addr, u8 value)
{
	if(addr<0x4000) switch(addr&0x0007) 
	{
	case 0x0000:
		//Bit7  Execute NMI on VBlank             (0=Disabled, 1=Enabled)
		//Bit6  PPU Master/Slave Selection        (0=Master, 1=Slave) (Not used in NES)
		//Bit5  Sprite Size                       (0=8x8, 1=8x16)
		//Bit4  Pattern Table Address Background  (0=VRAM 0000h, 1=VRAM 1000h)
		//Bit3  Pattern Table Address 8x8 Sprites (0=VRAM 0000h, 1=VRAM 1000h)
		//Bit2  Port 2007h VRAM Address Increment (0=Increment by 1, 1=Increment by 32)
		//Bit1-0 Name Table Scroll Address        (0-3=VRAM 2000h,2400h,2800h,2C00h)
		//(That is, Bit0=Horizontal Scroll by 256, Bit1=Vertical Scroll by 240)
		ctrl1 = value;
		break;

	case 0x0001:
		//Bit7-5 Color Emphasis       (0=Normal, 1-7=Emphasis) (see Palettes chapter)
		//Bit4  Sprite Visibility     (0=Not displayed, 1=Displayed)
		//Bit3  Background Visibility (0=Not displayed, 1=Displayed)
		//Bit2  Sprite Clipping       (0=Hide in left 8-pixel column, 1=No clipping)
		//Bit1  Background Clipping   (0=Hide in left 8-pixel column, 1=No clipping)
		//Bit0  Monochrome Mode       (0=Color, 1=Monochrome)  (see Palettes chapter)
		//If both sprites and BG are disabled (Bit 3,4=0) then video output is disabled, and VRAM can be accessed at any time (instead of during VBlank only).
		//However, SPR-RAM does no longer receive refresh cycles, and its content will gradually degrade when the display is disabled.
		ctrl2 = value;
		break;

	case 0x0002:
		printf("Write to PPU Status register value=0x%04x\n",value);
		break;
	case 0x0003:
		//  D7-D0: 8bit address in SPR-RAM  (00h-FFh)
		//
		//Specifies the destination address in Sprite RAM for use with Port 2004h (Single byte write), and Port 4014h (256 bytes DMA transfer).
		//This register is internally used during rendering (and typically contains 00h at the begin of the VBlank period).
		spr_addr = value;
		break;
	case 0x0004:
		//  D7-D0: 8bit data written to SPR-RAM.
		//
		//Read/write data to/from selected address in Sprite RAM.
		//The Port 2003h address is auto-incremented by 1 after each <write> to 2004h.
		//The address is NOT auto-incremented after <reading> from 2004h.
		spr[spr_addr++] = value;
		break;
	case 0x0005:
		//printf("PPU: Unhandled Memory Write: Scroll Offset (0x2005) [0x%04x]=0x%02x\n",addr,value);
		if(write_num^=1) // if is 1, was 0, so first write
		{
			scroll1=value;
		}
		else
		{
			scroll2=value;
		}
		latch_2005 = value;
		break;
	case 0x0006:
		//Used to specify the 14bit VRAM Address for use with Port 2007h.
		//
		//  Port 2006h-1st write: VRAM Address Pointer MSB (6bit)
		//  Port 2006h-2nd write: VRAM Address Pointer LSB (8bit)
		//
		//Caution: Writes to Port 2006h are overwriting scroll reload bits (in Port 2005h and Bit0-1 of Port 2000h). 
		//And, the PPU uses the Port 2006h register internally during rendering, when the display is enabled one should
		//thus reinitialize Port 2006h at begin of VBlank before accessing VRAM via Port 2007h.
		if(write_num^=1) // if is 1, was 0, so first write
		{
			// applied from reload bits on second write
			//vram_addr = (vram_addr&0xFF)|((value&0x3f)<<8);

			//VRAM-Pointer            Scroll-Reload
			//A8  2006h/1st-Bit0 <--> Y*64  2005h/2nd-Bit6
			//A9  2006h/1st-Bit1 <--> Y*128 2005h/2nd-Bit7
			scroll2 = (scroll2&0x3F)|((value&3)<<6);
			//A10 2006h/1st-Bit2 <--> X*256 2000h-Bit0
			//A11 2006h/1st-Bit3 <--> Y*240 2000h-Bit1
			ctrl1 = (ctrl1&0xFC)|((value>>2)&3);
			//A12 2006h/1st-Bit4 <--> Y*1   2005h/2nd-Bit0
			//A13 2006h/1st-Bit5 <--> Y*2   2005h/2nd-Bit1
			//-   2006h/1st-Bit6 <--> Y*4   2005h/2nd-Bit2
			scroll2 = (scroll2&0xF8)|((value>>4)&7);
		}
		else // was 1, second write
		{
			//A0  2006h/2nd-Bit0 <--> X*8   2005h/1st-Bit3
			//A1  2006h/2nd-Bit1 <--> X*16  2005h/1st-Bit4
			//A2  2006h/2nd-Bit2 <--> X*32  2005h/1st-Bit5
			//A3  2006h/2nd-Bit3 <--> X*64  2005h/1st-Bit6
			//A4  2006h/2nd-Bit4 <--> X*128 2005h/1st-Bit7
			scroll1 = (scroll1&7) | (value<<3);

			//A5  2006h/2nd-Bit5 <--> Y*8   2005h/2nd-Bit3
			//A6  2006h/2nd-Bit6 <--> Y*16  2005h/2nd-Bit4
			//A7  2006h/2nd-Bit7 <--> Y*32  2005h/2nd-Bit5
			scroll2 = (scroll2&0xC7)|((value>>2)&0x38);

			//-   -              <--> X*1   2005h/1st-Bit0
			//-   -              <--> X*2   2005h/1st-Bit1
			//-   -              <--> X*4   2005h/1st-Bit2

			int temp = 0;
			// apply also MSB from reload
			//VRAM-Pointer            Scroll-Reload
			//A8  2006h/1st-Bit0 <--> Y*64  2005h/2nd-Bit6
			//A9  2006h/1st-Bit1 <--> Y*128 2005h/2nd-Bit7
			temp |= (scroll2>>6)&0x3;
			//A10 2006h/1st-Bit2 <--> X*256 2000h-Bit0
			//A11 2006h/1st-Bit3 <--> Y*240 2000h-Bit1
			temp |= (ctrl1&0x03)<<2;
			//A12 2006h/1st-Bit4 <--> Y*1   2005h/2nd-Bit0
			//A13 2006h/1st-Bit5 <--> Y*2   2005h/2nd-Bit1
			//-   2006h/1st-Bit6 <--> Y*4   2005h/2nd-Bit2
			temp |= (scroll2&0x07)<<4;

			vram_addr = value | (temp<<8);
		}
		latch_2006 = value;
		break;
	case 0x0007:
		//The PPU will auto-increment the VRAM address (selected via Port 2006h) after each read/write from/to Port 2007h by 1 or 32 (depending on Bit2 of $2000).
		//
		//  Bit7-0  8bit data read/written from/to VRAM
		//
		//Caution: Reading from VRAM 0000h-3EFFh loads the desired value into a latch,
		//and returns the OLD content of the latch to the CPU.
		//After changing the address one should thus always issue a dummy read to flush the old content.
		//However, reading from Palette memory VRAM 3F00h-3FFFh, or writing to VRAM 0000-3FFFh does directly access
		//the desired address.
		WritePPU(vram_addr,value);

		vram_addr += (ctrl1&4)?32:1;

		break;
	}
	else switch(addr)
	{
	case 0x4014:
		cpu->Stall(512);
		for(int i=0;i<256;i++)
		{
			spr[(spr_addr+i)&255] = memory->Read((u16(value)<<8)+i);
		}

		break;
	}
/*
Registers used to Read and Write VRAM data, and for Background Scrolling.
The CPU can Read/Write VRAM during VBlank only - because the PPU permanently accesses VRAM during rendering (even in HBlank phases),
and because the PPU uses the VRAM Address register as scratch pointer. Respectively, the address in Port 2006h is destroyed after rendering,
and must be re-initialized before using Port 2007h.

1st/2nd Write
Below Port 2005h and 2006h require two 8bit writes to receive a 16bit parameter, the current state (1st or 2nd write)
is memorized in a single flipflop, which is shared for BOTH Port 2005h and 2006h. The flipflop is reset when reading
from PPU Status Register Port 2002h (the next write will be then treated as 1st write) (and of course it is also reset after any 2nd write).

2005h - PPU Background Scrolling Offset (W2)
Defines the coordinates of the upper-left background pixel, together with PPU Control Register 1, Port 2000h, Bits 0-1).

  Port 2005h-1st write: Horizontal Scroll Origin (X*1) (0-255)
  Port 2005h-2nd write: Vertical Scroll Origin   (Y*1) (0-239)
  Port 2000h-Bit0: Horizontal Name Table Origin  (X*256)
  Port 2000h-Bit1: Vertical Name Table Origin    (Y*240)

Caution: The above scroll reload settings are overwritten by writes to Port 2006h. See PPU Scrolling chapter for more info.



 PPU Scrolling

The PPU allows to scroll the background pixelwise horizontally and vertically. The total scroll-able area is 512x480 pixels
(though the full size can be usedwith external memory only, see Name Tables chapter), of which circa 256x240 pixels are displayed
(see visible screen resolution).

Vertical offsets 240-255 (aka Tile Rows 30-31) will cause garbage Tile numbers to be fetched from the Attribute Table
(instead of from Name Table), after line 255 it will wrap to line 0, but without producing a carry-out to the Name Table Address.

Scroll Pointer and Reload Registers
Scrolling relies on a Pointer register (Port 2006h), and on a Reload register (Port 2005h, and Bit0-1 of Port 2000h). 
The Pointer is automatically incremented by the hardware during rendering, and points to the currently drawn tile row,
the same pointer register is also used by software to access VRAM during VBlank or when the display is disabled. 
The Reload value defines the horizontal and vertical origin of upper-left pixel, the reload value is automatically loaded
into the Pointer at the end of the vblank period (vertical reload bits), and at the begin of each scanline (horizontal reload bits).
The relation between Pointer and Reload bits is:
*/
  //VRAM-Pointer            Scroll-Reload
  //A8  2006h/1st-Bit0 <--> Y*64  2005h/2nd-Bit6
  //A9  2006h/1st-Bit1 <--> Y*128 2005h/2nd-Bit7
  //A10 2006h/1st-Bit2 <--> X*256 2000h-Bit0
  //A11 2006h/1st-Bit3 <--> Y*240 2000h-Bit1
  //A12 2006h/1st-Bit4 <--> Y*1   2005h/2nd-Bit0
  //A13 2006h/1st-Bit5 <--> Y*2   2005h/2nd-Bit1
  //-   2006h/1st-Bit6 <--> Y*4   2005h/2nd-Bit2
  //-   2006h/1st-Bit7 <--> -     -
  //A0  2006h/2nd-Bit0 <--> X*8   2005h/1st-Bit3
  //A1  2006h/2nd-Bit1 <--> X*16  2005h/1st-Bit4
  //A2  2006h/2nd-Bit2 <--> X*32  2005h/1st-Bit5
  //A3  2006h/2nd-Bit3 <--> X*64  2005h/1st-Bit6
  //A4  2006h/2nd-Bit4 <--> X*128 2005h/1st-Bit7
  //A5  2006h/2nd-Bit5 <--> Y*8   2005h/2nd-Bit3
  //A6  2006h/2nd-Bit6 <--> Y*16  2005h/2nd-Bit4
  //A7  2006h/2nd-Bit7 <--> Y*32  2005h/2nd-Bit5
  //-   -              <--> X*1   2005h/1st-Bit0
  //-   -              <--> X*2   2005h/1st-Bit1
  //-   -              <--> X*4   2005h/1st-Bit2
/*

Port 2006h-1st Write (VRAM Pointer MSB)
As one might (not) have expected, this does NOT change the VRAM Pointer, instead, the written value is stored in the corresponding Reload bits (Port 2005h/2000h settings), the VRAM pointer is left unchanged for now.

Port 2006h-2nd Write (VRAM Pointer LSB)
The written value is stored in the VRAM Pointer LSB Bits (and maybe also in the corresponding Reload bits ?). And, the VRAM Pointer MSB is now loaded from the corresponding Reload bits (ie. the value from the previous Port 2006h-1st Write is applied now).

Port 2005h-1st Write (Horizontal Scroll Origin, X*1, 0-255)
Port 2005h-2nd Write (Vertical Scroll Origin, Y*1, 0-239)
Port 2000h-Bit0 (Horizontal Name Table Origin, X*256)
Port 2000h-Bit1 (Vertical Name Table Origin, Y*240)
Writing to these registers changes the Reload value bits only, the VRAM Pointer is left unchanged (except for indirect changes at times when the Reload value is loaded into the Pointer during rendering).

Full-screen and Mid-frame Scrolling
Simple full-screen scrolling can be implemented by initializing the Reload value via Ports 2005h and 2000h. Many games change the scroll settings mid-frame to split the screen into a scrolled and non-scrolled area: The Horizontal bits can be changed by re-writing the Reload value via Ports 2005h and 2000h, the vertical bits by re-writing the Pointer value via Port 2006h. Changing both horizontal and vertical bits is possible by mixed writes to Port 2005h and 2006h, for example:

  [2006h.1st]=(X/256)*4 + (Y/240)*8
  [2005h.2nd]=((Y MOD 240) AND C7h)
  [2005h.1st]=(X AND 07h)
  [2006h.2nd]=(X AND F8h)/8 + ((Y MOD 240) AND 38h)*4

Notes: In that example, most bits are updated twice, once via 2006h and once via 2005h, above shows only the relevant bits, the other bits would be don't care (eg. writing unmasked values to 2005h would be faster, and wouldn't change the functionality). The 1st/2nd-write-flipflop is toggled on each of the four writes, so that above does <first> change 2005h-2nd-write, and <then> 2005h-1st-write.

Pointer Increment/Reload during Rendering
During rendering, A4-A0 is incremented per tile, with carry-out to A10, at end of HBlank A4-A0 and A10 are reset to the Reload value. "A14-A12" are used as LSBs of Tile Data address, these bits are incremented per scanline, with carry-out to tile row A9-A5, the tile row wraps from 29 to 0 with carry-out to A11.
Note: Initializing the tile row to 30 or 31 will display garbage tiles (fetched from Attribute table area), in that case the row wraps from 31 to 0, but without carry-out to A11.
*/

}

u8   Ppu::Read (u16 addr)
{
	u8 ret = 0;
	if(addr<0x4000) switch(addr&0x0007) 
	{
	case 0x0000:
		ret= ctrl1; break;

	case 0x0001:
		ret= ctrl2; break;

	case 0x0002:
		//  Bit7   VBlank Flag    (1=VBlank)
		//  Bit6   Sprite 0 Hit   (1=Background-to-Sprite0 collision)
		//  Bit5   Lost Sprites   (1=More than 8 sprites in 1 scanline)
		//  Bit4-0 Not used       (Undefined garbage)
		//
		//Reading resets the 1st/2nd-write flipflop (used by Port 2005h and 2006h).
		//Reading resets Bit 7, can be used to acknowledge NMIs, Bit 7 is also automatically reset at the end of VBlank, so manual acknowledge is normally not required (unless one wants to free the NMI signal for external NMI inputs).
		//
		//Status Notes
		//VBlank flag is set in each frame, even if the display is fully disabled, and even if NMIs are disabled.
		//Hit flag may become set only if both BG and OBJ are enabled. Lost Sprites flag may become set only if video is enabled
		//  (ie. BG or OBJ must be on). For info about the "Not used" status bits, and some other PPU bits see: Unpredictable Things
		//
		write_num=0;
		ret = status; 
		status&=0x7f;
		return ret;
		break;

	case 0x0003:
		ret= spr_addr; break;

	case 0x0004:
		ret= spr[spr_addr++]; break;

	case 0x0005:
		ret = latch_2005; break;

	case 0x0006:
		ret = latch_2006; break;

	case 0x0007:
		//The PPU will auto-increment the VRAM address (selected via Port 2006h) after each read/write from/to Port 2007h by 1 or 32 (depending on Bit2 of $2000).
		//
		//  Bit7-0  8bit data read/written from/to VRAM
		//
		//Caution: Reading from VRAM 0000h-3EFFh loads the desired value into a latch,
		//and returns the OLD content of the latch to the CPU.
		//After changing the address one should thus always issue a dummy read to flush the old content.
		//However, reading from Palette memory VRAM 3F00h-3FFFh, or writing to VRAM 0000-3FFFh does directly access
		//the desired address.

		if(vram_addr<0x3f00)
		{
			ret = vram_latch;
			vram_latch = ReadPPU(vram_addr);
		}
		else {
			ret = ReadPPU(vram_addr);
		}

		vram_addr += (ctrl1&4)?32:1;

		break;
	default:
		printf("PPU: Unhandled Memory Read: [0x%04x]\n",addr);
		return 0;
	}
	printf(" * DEBUG: Read from PPU [%04x] value=0x%04x\n",addr,ret);

	return ret;
}
