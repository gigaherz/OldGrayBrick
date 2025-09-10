#include "StdAfx.h"
#include "Emu.h"
#include "Mapper.h"

Mapper2::Mapper2(NesROMHeader hdr, FILE* rom_file)
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

	which_bank=0;

	PROM_AREA0 = PROM;
	PROM_AREA1 = PROM + (prom_banks - 1) * 16384;

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

Mapper2::~Mapper2(void)
{
	delete PROM;
	delete VROM;
}

void Mapper2::Write(u16 addr, u8 value)
{
	if(addr>=0x8000)
	{
		which_bank = value % prom_banks;
		PROM_AREA0 = PROM + which_bank * 16384;
	}
}

static bool Mapper2errorprint = false;
int Mapper2::Read (u16 addr)
{
	if(addr<0x8000)
	{
		if (!Mapper2errorprint)
		{
			Mapper2errorprint = true;
			printf("Unhandled read from Mapper2 addr 0x%04x", addr);
		}
		return -1; // 0xA5;
	}
	else if (addr < 0xC000)
	{
		return PROM_AREA0[addr - 0x8000];
	}
	else
	{
		return PROM_AREA1[addr - 0xC000];
	}
}

void Mapper2::WritePPU(u16 addr, u8 value)
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

int Mapper2::ReadPPU(u16 addr)
{
	if (addr < 0x2000)
	{
		return VROM[addr & vrom_mask];
	}
	else // 4KB VRAM (0x1000)
	{
		int whichbank = (addr & 0xC00) >> 10;
		int whichoffset = (addr & 0x3ff);

		int vaddr = bank_offsets0[header.mirror][whichbank] + whichoffset;

		return ppu->ReadVRAM(vaddr);
	}
}

void Mapper2::Init()
{
}

void Mapper2::Reset()
{
}

void Mapper2::SoftReset()
{
}

void Mapper2::Close()
{
}

void Mapper2::Emulate(u32 clocks)
{
}

int Mapper2::VMode() 
{
	return header.is_pal;
}

void Mapper2::PPUHBlank()
{
}