#include "StdAfx.h"
#include "Emu.h"
#include "Mapper.h"

Mapper1::Mapper1(NesROMHeader hdr, FILE* rom_file)
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

	prom_mode=3;
	vrom_mode=0;

	mirror_select=0;

	which_bit=0;
	value_buffer=0;

	prom_offset=0;
	vrom_offset0=0;
	vrom_offset1=1;

	prom_swap256_type=0;
	prom_swap256=0;
	prom_swap512=0;

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

Mapper1::~Mapper1(void)
{
	delete PROM;
	delete VROM;
}

void Mapper1::Write(u16 addr, u8 value)
{
	if(addr>=0x8000)
	{
		if(value&0x80)
		{
			which_bit=0;
			value_buffer=0;
		}

		value_buffer |= (value&1)<<(which_bit++);

		if(which_bit>=5)
		{
			value_buffer &= 0x1f;
			switch((addr>>13)&3)
			{
			case 0:
				mirror_select = value_buffer&3;
				prom_mode = (value_buffer>>2)&3;
				vrom_mode = value_buffer>>4;

				if(prom_banks>32)
				{
					vrom_mode=0;
					prom_swap256_type = value_buffer>>4;
				}

				break;
			case 1:

				vrom_offset0 = value_buffer;

				if(prom_banks>16)
				{
					vrom_offset0 = value_buffer&0xF;
					prom_swap256 = value_buffer>>4;
				}

				break;
			case 2:

				vrom_offset1 = value_buffer;

				if(prom_banks>32)
				{
					vrom_offset1 = value_buffer&0xF;
					prom_swap512 = value_buffer>>4;
				}

				break;
			case 3:
				prom_offset = value_buffer&0x1F;
				break;
			}

		}
	}
}

static bool mapper1errorprint = false;
int Mapper1::Read (u16 addr)
{
	if(addr<0x8000)
	{
		if (!mapper1errorprint)
		{
			mapper1errorprint = true;
			printf("Unhandled read from mapper1 addr 0x%04x", addr);
		}
		return -1; // 0xA5;
	}
	else 
	{
		int prom_base = prom_offset<<14;
		int addr_base = addr&0x3fff;

		if(prom_banks>32)
		{
			if(prom_swap256_type)
			{
				prom_base += (prom_swap256<<18);
				prom_base += (prom_swap512<<19);
			}
			else
			{
				prom_base += (prom_swap256<<19); // ???
			}

		}
		else if(prom_banks>16)
		{
			prom_base += (prom_swap256<<18);
		}

		switch(prom_mode)
		{
		case 2:
			if(addr<0xC000)
			{
				return PROM[addr_base+prom_base]; // fixed
			}
			else
			{
				return PROM[addr_base];
			}
		case 3:
			if(addr<0xC000)
			{
				return PROM[addr_base];
			}
			else
			{
				return PROM[addr_base+prom_size-0x4000]; // fixed
			}
		}

		return PROM[(addr-0x8000)&prom_mask+(prom_base<<0)];
	}
}

void Mapper1::WritePPU(u16 addr, u8 value)
{
	/*if((addr<0x2000)&&(vrom_banks>0))
	{
	printf("WARNING: Write to VROM[0x%04x]=0x%02x\n",addr,value);
	}
	else*/
	if((addr<0x2000)&&(vrom_banks==0))
	{
		VROM[addr]=value;
	}
	else
	{
		int whichbank = ((addr-0x2000)>>10)&3;
		int whichoffset = addr&0x3ff;

		if(mirror_select<2)
		{
			whichbank=0;
		}

		int vaddr = bank_offsets0[mirror_select&1][whichbank] + whichoffset;

		ppu->WriteVRAM(vaddr,value);
	}
}

int Mapper1::ReadPPU(u16 addr)
{
	if((addr<0x2000) /*&&(vrom_banks>0)*/)
	{
		if(!vrom_banks)
			return VROM[addr&vrom_mask];

		if(!vrom_mode)
			return VROM[(addr+(vrom_offset0<<13))& vrom_mask];

		if(addr<0x1000)
			return VROM[(addr+(vrom_offset0<<12))& vrom_mask];

		return VROM[(addr-0x1000+(vrom_offset1<<12))&vrom_mask];
	}
	else
	{
		int whichbank = ((addr-0x2000)>>10)&3;
		int whichoffset = addr&0x3ff;

		if(mirror_select<2)
		{
			whichbank=0;
		}

		int vaddr = bank_offsets0[mirror_select&1][whichbank] + whichoffset;

		return ppu->ReadVRAM(vaddr);
	}
}

void Mapper1::Init()
{
}

void Mapper1::Reset()
{
}

void Mapper1::SoftReset()
{
}

void Mapper1::Close()
{
}

void Mapper1::Emulate(u32 clocks)
{
}

int Mapper1::VMode() 
{
	return header.is_pal;
}

void Mapper1::PPUHBlank()
{
}