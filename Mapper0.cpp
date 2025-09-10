#include "StdAfx.h"
#include "Emu.h"
#include "Mapper.h"

Mapper0::Mapper0(NesROMHeader hdr, FILE* rom_file)
{
	prom_banks = hdr.romBanks;
	vrom_banks = hdr.vromBanks;

	prom_size = 16384 * prom_banks;
	vrom_size =  8192 * vrom_banks;

	prom_mask = prom_size-1;
	vrom_mask = vrom_size-1;

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

Mapper0::~Mapper0(void)
{
}

void Mapper0::Write(u16 addr, u8 value)
{
	printf("WARNING: Write to PROM[0x%04x]=0x%02x\n",addr,value);
}

static bool mapper0errorprint = false;
int  Mapper0::Read (u16 addr)
{
	if(addr<0x8000)
	{
		if (!mapper0errorprint)
		{
			mapper0errorprint = true;
			printf("Unhandled read from mapper0 addr 0x%04x",addr);
		}
		return -1; // 0xA5
	}
	else {
		return PROM[addr&prom_mask];
	}
}

void Mapper0::WritePPU(u16 addr, u8 value)
{
	if(addr<0x2000)
	{
		if(vrom_banks>0) 
		{
			//printf("WARNING: Write to VROM[0x%04x]=0x%02x, IGNORED.\n",addr,value);
			//VROM[addr]=value;
		}
		else
		{
			VROM[addr]=value;
		}
	}
	else
	{
		int addr_base = addr&0x0fff;
		int whichbank = (addr_base>>10);
		int whichoffset = addr_base&0x3ff;

		int vaddr = bank_offsets0[header.mirror][whichbank] + whichoffset;

		ppu->WriteVRAM(vaddr,value);
	}
}

int Mapper0::ReadPPU(u16 addr)
{
	if(addr<0x2000)
	{
		return VROM[addr&vrom_mask];
	}
	else // 4KB VRAM (0x1000)
	{
		int whichbank   = (addr&0xC00)>>10;
		int whichoffset = (addr&0x3ff);

		int vaddr = bank_offsets0[header.mirror][whichbank] + whichoffset;

		return ppu->ReadVRAM(vaddr);
	}
}

void Mapper0::Init()
{
}

void Mapper0::Reset()
{
}

void Mapper0::SoftReset()
{
}

void Mapper0::Close()
{
}

void Mapper0::Emulate(u32 clocks)
{
}

int Mapper0::VMode()
{
	return header.is_pal;
}

void Mapper0::PPUHBlank()
{
}