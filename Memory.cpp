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
	if(addr<0x2000) // 0000h-07FFh   Internal 2K Work RAM (mirrored to 800h-1FFFh)
	{
		return WorkRam[addr&0x7ff];
	}
	else if(addr<0x4000) // 2000h-2007h   Internal PPU Registers (mirrored to 2008h-3FFFh)
	{
		return ppu->Read(addr&0x7);
	}
	else if(addr<0x4018) // 4000h-4017h   Internal APU Registers
	{
		if(addr==0x4014)
			return ppu->Read(addr);

		if(addr==0x4016)
			return pad->Poll(0);

		if(addr==0x4017)
			return pad->Poll(1);

		return apu->Read(addr);
	}
	else
	{
		// 4018h-5FFFh   Cartridge Expansion Area almost 8K
		// 6000h-7FFFh   Cartridge SRAM Area 8K
		// 8000h-FFFFh   Cartridge PRG-ROM Area 32K
		return mapper->Read(addr);
	}
}

void Memory::Reset()
{
}
void Memory::Init()
{
	Reset();
}

void Memory::Close()
{
}

