#include "StdAfx.h"
#include "Emu.h"
#include "Mapper.h"

Mapper4::Mapper4(NesROMHeader hdr, FILE* rom_file)
{
	pram_banks = hdr.romBanks;
	prom_banks = hdr.romBanks;
	vrom_banks = hdr.vromBanks;

	pram_size = 8192 * pram_banks;
	prom_size = 16384 * prom_banks;
	vrom_size = 8192 * vrom_banks;

	PRAM = new u8[pram_size];
	PROM = new u8[prom_size];

	if (vrom_size == 0)
		VROM = new u8[8192];
	else
		VROM = new u8[vrom_size];

	pram_mask = pram_size - 1;
	prom_mask = prom_size - 1;
	vrom_mask = vrom_size - 1;

	PROM_AREA0 = PROM;
	PROM_AREA1 = PROM;
	PROM_AREA2 = PROM + ((prom_size - 16384) & prom_size);
	PROM_AREA3 = PROM + ((prom_size - 8192) & prom_size);

	VROM_AREA0 = VROM;
	VROM_AREA1 = VROM;
	VROM_AREA2 = VROM;
	VROM_AREA3 = VROM;
	VROM_AREA4 = VROM;
	VROM_AREA5 = VROM;

	BankSel = 0;
	PRAMEnable = 0; // MMC6 only
	PROMConfig = 0;
	VROMConfig = 0;
	PRAMWriteMask = 0;
	PRAMReadMask = 0;
	PRAMWriteMask2 = 0; // MMC6 only
	PRAMReadMask2 = 0;
	IRQLatch = 0;
	IRQValue = 0;
	IRQPending = 0;
	IRQEnable = 0;
	NametableCtrl = 0;

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
		if (addr >= 0x6000 && PRAM != nullptr && !PRAMWriteMask)
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
		{
			BankSel = value & 0x03;
			PRAMEnable = (value >> 5) & 1;
			PROMConfig = (value >> 6) & 1;
			VROMConfig = (value >> 7) & 1;
			break;
		}
		case 1: // bank select data
		{
			switch (BankSel)
			{
			case 0: // 2kb area 1
				VROM_AREA0 = VROM + (((value & 0xFE) * 1024) & vrom_size);
				break;
			case 1: // 2kb area 2
				VROM_AREA1 = VROM + (((value & 0xFE) * 1024) & vrom_size);
				break;
			case 2: // 1kb area 1
				VROM_AREA2 = VROM + ((value * 1024) & vrom_size);
				break;
			case 3: // 1kb area 2
				VROM_AREA3 = VROM + ((value * 1024) & vrom_size);
				break;
			case 4: // 1kb area 3
				VROM_AREA4 = VROM + ((value * 1024) & vrom_size);
				break;
			case 5: // 1kb area 4
				VROM_AREA5 = VROM + ((value * 1024) & vrom_size);
				break;
			case 6:
				PROM_AREA0 = PROM + ((value * 8192) & prom_size);
				break;
			case 7:
				PROM_AREA1 = PROM + ((value * 8192) & prom_size);
				break;
			}
			break;
		}
		case 2: // nametable ctrl
			NametableCtrl = value & 1;
			break;
		case 3: // prg ram protect
			PRAMWriteMask = value >> 7;
			PRAMReadMask = (value >> 6) & 1;
			PRAMWriteMask2 = (value >> 5) & 1; // MMC6 only
			PRAMReadMask2 = (value >> 4) & 1; // MMC6 only
			break;
		case 4: // irq latch
			IRQLatch = value;
			break;
		case 5: // irq reload
			IRQValue = 0;
			break;
		case 6: // irq disable
			IRQEnable = false;
			IRQPending = false;
			break;
		case 7: // irq enable
			IRQEnable = true;
			break;
		}
	}
}

static bool mapper4errorprint = false;
int Mapper4::Read(u16 addr)
{
	if (addr < 0x8000)
	{
		if (addr >= 0x6000 && PRAM != nullptr && !PRAMReadMask)
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
			return (PROMConfig ? PROM_AREA2 : PROM_AREA0)[addr & 0x1fff];
		}
		else if (addr < 0xC000)
		{
			return PROM_AREA1[addr & 0x1fff];
		}
		else if (addr < 0xE000)
		{
			return (PROMConfig ? PROM_AREA0 : PROM_AREA2)[addr & 0x1fff];
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

int Mapper4::ReadPPU(u16 addr)
{
	if (addr < 0x2000)
	{
		if (VROMConfig)
			addr ^= 0x1000;
		if (addr < 0x0800)
		{
			return VROM_AREA0[addr & 0x07ff];
		}
		else if (addr < 0x1000)
		{
			return VROM_AREA1[addr & 0x07ff];
		}
		else if (addr < 0x1400)
		{
			return VROM_AREA2[addr & 0x03ff];
		}
		else if (addr < 0x1800)
		{
			return VROM_AREA3[addr & 0x03ff];
		}
		else if (addr < 0x1C00)
		{
			return VROM_AREA4[addr & 0x03ff];
		}
		else
		{
			return VROM_AREA5[addr & 0x03ff];
		}
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

void Mapper4::SoftReset()
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

void Mapper4::PPUHBlank()
{
	IRQValue--;
	if (IRQValue <= 0)
	{
		IRQValue = IRQLatch;
		if (IRQEnable)
		{
			IRQPending = true;
		}
	}

	if (IRQPending)
		cpu->Interrupt();
}