#include "StdAfx.h"
#include "Emu.h"
#include "Mapper.h"

Mapper4::Mapper4(NesROMHeader hdr, FILE* rom_file)
{
	prom_banks = hdr.romBanks;
	vrom_banks = hdr.vromBanks;

	prom_size = 16384 * prom_banks;
	vrom_size = 8192 * vrom_banks;

	prom_mask = prom_size - 1;
	vrom_mask = vrom_size - 1;

	int read = 0;
	if ((read = fread(PROM, 1, prom_size, rom_file)) != prom_size)
	{
		printf("WARNING: Error reading PROM from ROM\n");
	}

	if (vrom_size > 0)
	{
		if ((read = fread(VROM, 1, vrom_size, rom_file)) != vrom_size)
		{
			printf("WARNING: Error reading VROM from ROM\n");
		}
	}
	header = hdr;
}

Mapper4::~Mapper4(void)
{
}

void Mapper4::Write(u16 addr, u8 value)
{
	if (addr < 0x8000)
	{
		if (addr >= 0x6000 && PRAM != nullptr)
		{
			PRAM[addr & 0x1fff] = value;
		}
	}
	else
	{
		// The MMC3 has 4 pairs of registers at 
		// $8000-$9FFF, 
		// $A000-$BFFF, 
		// $C000-$DFFF, 
		// and $E000-$FFFF
		// - even addresses ($8000, $8002, etc.) select the low register and
		// odd addresses ($8001, $8003, etc.) select the high register in each pair. 
		// 
		// These can be broken into two independent functional units: memory mapping ($8000, $8001, $A000, $A001)
		// and scanline counting ($C000, $C001, $E000, $E001). 

		int pair = (addr >> 12) & 6;
		int reg = addr & 1;
		int idx = pair | reg;

		switch (idx)
		{
		case 0: // bank select ctrl
			break;
		case 1: // bank select data
			break;
		case 2: // nametable ctrl
			break;
		case 3: // prg ram protect
			break;
		case 4: // irq latch
			break;
		case 5: // irq reload
			break;
		case 6: // irq disable
			break;
		case 7: // irq enable
			break;
		}
	}
}

static bool mapper4errorprint = false;
int Mapper4::Read(u16 addr)
{
	if (addr < 0x8000)
	{
		if (addr >= 0x6000 && PRAM != nullptr)
		{
			return PRAM[addr & 0x1fff];
		}
		if (!mapper4errorprint)
		{
			mapper4errorprint = true;
			printf("Unhandled read from mapper1 addr 0x%04x", addr);
		}
		return -1; // 0xA5;
	}
	else 
	{
		if (addr < 0xA000)
		{
			return PROM_AREA0[addr & 0x1fff];
		}
		else if (addr < 0xC000)
		{
			return PROM_AREA1[addr & 0x1fff];
		}
		else if (addr < 0xE000)
		{
			return PROM_AREA2[addr & 0x1fff];
		}
		else
		{
			return PROM_AREA3[addr & 0x1fff];
		}
	}
}

void Mapper4::WritePPU(u16 addr, u8 value)
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

int Mapper4::ReadPPU(u16 addr)
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

void Mapper4::Init()
{
}

void Mapper4::Reset()
{
}

void Mapper4::Close()
{
}

void Mapper4::Emulate(u32 clocks)
{
}

int Mapper4::VMode() {
	return header.is_pal;
}