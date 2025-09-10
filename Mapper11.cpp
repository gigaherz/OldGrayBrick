#include "StdAfx.h"
#include "Emu.h"
#include "Mapper.h"

Mapper11::Mapper11(NesROMHeader hdr, FILE* rom_file)
{
	prom_banks = hdr.romBanks;
	vrom_banks = hdr.vromBanks;

	prom_size = 16384 * prom_banks;
	vrom_size =  8192 * vrom_banks;

	PROM = new u8[prom_size];

	if(vrom_size==0)
		VROM = new u8[8192];
	else
		VROM = new u8[vrom_size];

	prom_mask = prom_size-1;
	vrom_mask = vrom_size-1;

	p_which_bank=0;
	v_which_bank = 0;

	PROM_AREA0 = PROM;
	VROM_AREA0 = VROM;

	int read = 0;
	if((read=fread(PROM,1,prom_size,rom_file))!=prom_size)
	{
		printf("WARNING: Error reading PROM from ROM\n");
	}

	if(vrom_size>0)
	{
		if((read=fread(VROM,1,vrom_size,rom_file))!=vrom_size)
		{
			printf("WARNING: Error reading VROM from ROM\n");
		}
	}
	header=hdr;
}

Mapper11::~Mapper11(void)
{
	delete PROM;
	delete VROM;
}

void Mapper11::Write(u16 addr, u8 value)
{
	if(addr>=0x8000)
	{
		p_which_bank = min(value & 3, prom_banks-1);
		v_which_bank = min((value>>4) & 7, vrom_banks-1);
		PROM_AREA0 = PROM + p_which_bank * 32768;
		VROM_AREA0 = VROM + v_which_bank * 8192;
	}
}

static bool Mapper11errorprint = false;
int  Mapper11::Read (u16 addr)
{
	if(addr<0x8000)
	{
		if (!Mapper11errorprint)
		{
			Mapper11errorprint = true;
			printf("Unhandled read from Mapper11 addr 0x%04x", addr);
		}
		return -1; // 0xA5;
	}
	else
	{
		return PROM_AREA0[(addr - 0x8000)& 32767];
	}
}

void Mapper11::WritePPU(u16 addr, u8 value)
{
	if (addr < 0x2000)
	{
		if (vrom_banks > 0)
		{
			//printf("WARNING: Write to VROM[0x%04x]=0x%02x, IGNORED.\n",addr,value);
			//VROM[addr]=value;
		}
		else
		{
			VROM_AREA0[addr] = value;
		}
	}
	else
	{
		int addr_base = addr & 0x0fff;
		int whichbank = (addr_base >> 10);
		int whichoffset = addr_base & 0x3ff;

		int vaddr = bank_offsets0[header.mirror][whichbank] + whichoffset;

		ppu->WriteVRAM(vaddr, value);
	}
}

int Mapper11::ReadPPU(u16 addr)
{
	if (addr < 0x2000)
	{
		return VROM_AREA0[addr & vrom_mask];
	}
	else // 4KB VRAM (0x1000)
	{
		int whichbank = (addr & 0xC00) >> 10;
		int whichoffset = (addr & 0x3ff);

		int vaddr = bank_offsets0[header.mirror][whichbank] + whichoffset;

		return ppu->ReadVRAM(vaddr);
	}
}

void Mapper11::Init()
{
}

void Mapper11::Reset()
{
}

void Mapper11::SoftReset()
{
}

void Mapper11::Close()
{
}

void Mapper11::Emulate(u32 clocks)
{
}

int Mapper11::VMode() 
{
	return header.is_pal;
}
