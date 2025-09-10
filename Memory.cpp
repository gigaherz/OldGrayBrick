#include "StdAfx.h"
#include "Emu.h"

Memory::Memory(void)
{
    memset(WorkRam, 0, sizeof(WorkRam));
}

Memory::~Memory(void)
{
}



void Memory::Write(u16 addr, u8 value)
{
	OpenBusValue = value;
	OpenBusLifetime = 20;

	if(addr<0x2000) // 0000h-07FFh   Internal 2K Work RAM (mirrored to 800h-1FFFh)
	{
		WorkRam[addr&0x7ff] = value;
	}
	else if(addr<0x4000) // 2000h-2007h   Internal PPU Registers (mirrored to 2008h-3FFFh)
	{
		ppu->Write(addr&0x7,value); // wtf only 8 regs?
	}
	else if(addr<0x4018) // 4000h-4017h   Internal APU Registers
	{
		if(addr==0x4014)
			return ppu->Write(addr,value);

		if(addr==0x4016)
			return pad->Strobe(value);

		apu->Write(addr,value);
	}
	else
	{
		// 4018h-5FFFh   Cartridge Expansion Area almost 8K
		// 6000h-7FFFh   Cartridge SRAM Area 8K
		// 8000h-FFFFh   Cartridge PRG-ROM Area 32K
		mapper->Write(addr,value);
	}
}

u8   Memory::Read (u16 addr)
{
	int ret = -1;
	if(addr<0x2000) // 0000h-07FFh   Internal 2K Work RAM (mirrored to 800h-1FFFh)
	{
		ret = WorkRam[addr&0x7ff];
	}
	else if(addr<0x4000) // 2000h-2007h   Internal PPU Registers (mirrored to 2008h-3FFFh)
	{
		ret = ppu->Read(addr&0x7);
	}
	else if(addr<0x4018) // 4000h-4017h   Internal APU Registers
	{
		switch (addr)
		{
		case 0x4014:
			ret = ppu->Read(addr);
			break;
		case 0x4016:
			ret = pad->Poll(0);
			break;
		case 0x4017:
			ret = pad->Poll(1);
			break;
		default:
			ret = apu->Read(addr);
			break;
		}
	}
	else
	{
		// 4018h-5FFFh   Cartridge Expansion Area almost 8K
		// 6000h-7FFFh   Cartridge SRAM Area 8K
		// 8000h-FFFFh   Cartridge PRG-ROM Area 32K
		return mapper->Read(addr);
	}

	if (ret < 0)
	{
		if (OpenBusLifetime > 0)
		{
			OpenBusLifetime--;
			ret = OpenBusValue;
		}
		else
		{
			ret = 0;
		}
	}
	else
	{
		OpenBusValue = ret;
		OpenBusLifetime = 20;
	}
	return ret;
}

void Memory::Reset()
{
}

void Memory::SoftReset()
{
}

void Memory::Init()
{
	Reset();
}

void Memory::Close()
{
}

