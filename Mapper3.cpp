#include "StdAfx.h"
#include "Emu.h"
#include "Mapper.h"

Mapper3::Mapper3(NesROMHeader hdr, FILE* rom_file)
{
	prom_banks = hdr.romBanks;
	vrom_banks = hdr.vromBanks;

	prom_size = 16384 * prom_banks;
	vrom_size =  8192 * vrom_banks;

	PRAM = new u8[2048];
	PROM = new u8[prom_size];

	if(vrom_size==0)
		VROM = new u8[8192];
	else
		VROM = new u8[vrom_size];

	pram_mask = 2047;
	prom_mask = prom_size-1;
	vrom_mask = vrom_size-1;

	which_bank=0;

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

Mapper3::~Mapper3(void)
{
	delete PROM;
	delete VROM;
}

void Mapper3::Write(u16 addr, u8 value)
{
	if (addr >= 0x6000 && addr < 0x8000)
	{
		PRAM[addr & pram_mask] = value;
	}
	else if (addr >= 0x8000)
	{
		which_bank = value % vrom_banks;
		VROM_AREA0 = VROM + which_bank % vrom_mask;
	}
}

static bool Mapper3errorprint = false;
int Mapper3::Read (u16 addr)
{
	if (addr >= 0x6000 && addr < 0x8000)
	{
		return PRAM[addr & pram_mask];
	}
	else if(addr<0x8000)
	{
		if (!Mapper3errorprint)
		{
			Mapper3errorprint = true;
			printf("Unhandled read from Mapper3 addr 0x%04x", addr);
		}
		return -1; // 0xA5;
	}
	else
	{
		return PROM[addr & prom_mask];
	}
}

void Mapper3::WritePPU(u16 addr, u8 value)
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
			VROM[addr] = value;
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

int Mapper3::ReadPPU(u16 addr)
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

void Mapper3::Init()
{
}

void Mapper3::Reset()
{
}

void Mapper3::SoftReset()
{
}

void Mapper3::Close()
{
}

void Mapper3::Emulate(u32 clocks)
{
}

int Mapper3::VMode() 
{
	return header.is_pal;
}

void Mapper3::PPUHBlank()
{
}