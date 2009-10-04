#include "StdAfx.h"
#include "Emu.h"
#include "Mapper.h"

class Mapper4: public Mapper
{
private:
	u8 PROM[65536];
	u8 VROM[65536];

	NesROMHeader header;

	int prom_banks;
	int prom_size;
	int prom_mask;

	int vrom_banks;
	int vrom_size;
	int vrom_mask;

	u8 idx_ctrl;
	u16 xor_prg; 
	u16 xor_ppu;

	u8 mirror_select;
public:

	Mapper4(NesROMHeader hdr, FILE* rom_file)
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

	virtual ~Mapper4(void)
	{
	}

	virtual void Write(u16 addr, u8 value)
	{

		switch(addr)
		{
		case 0x8000:
			idx_ctrl = value;

			xor_prg = 0;
			xor_ppu = 0;
			if(value&0x80)	xor_ppu |= 0x1000;
			if(value&0x40)	xor_prg |= 0x4000;

			switch(value&0x7)
			{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7: ;
			}

			break;
		default:
			printf("WARNING: Write to PROM[0x%04x]=0x%02x\n",addr,value);
		}
	}

	virtual u8   Read (u16 addr)
	{
		if(addr<0x8000)
		{
			printf("Unhandled read from mapper0 addr 0x%04x",addr);
			return 0xA5;
		}
		else {

			switch(addr)
			{
			case 0x8000:
				break;
			default:
				return PROM[addr&prom_mask];
			}
		}
	}

	virtual void WritePPU(u16 addr, u8 value)
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

			int vaddr = bank_offsets0[mirror_select][whichbank] + whichoffset;

			ppu->WriteVRAM(vaddr,value);
		}
	}

	virtual u8   ReadPPU(u16 addr)
	{
		if((addr<0x2000) /*&&(vrom_banks>0)*/)
		{
			return VROM[addr&vrom_mask];
		}
		else
		{
			int whichbank = ((addr-0x2000)>>10)&3;
			int whichoffset = addr&0x3ff;

			int vaddr = bank_offsets0[header.mirror][whichbank] + whichoffset;

			return ppu->ReadVRAM(vaddr);
		}
	}

	virtual void Init()
	{
	}

	virtual void Reset()
	{
	}

	virtual void Close()
	{
	}

	virtual void Emulate(u32 clocks)
	{
	}

	virtual int VMode() {
		return header.is_pal;
	}

};
