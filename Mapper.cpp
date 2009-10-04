#include "StdAfx.h"
#include "Emu.h"
#include "Mapper.h"

Mapper::~Mapper(void)
{
}

class Mapper0: public Mapper
{
private:
	u8 PROM[65536];
	u8 VROM[65536]; // if no VROM present, the mapper will have VRAM

	NesROMHeader header;

	int prom_banks;
	int prom_size;
	int prom_mask;

	int vrom_banks;
	int vrom_size;
	int vrom_mask;

public:

	Mapper0(NesROMHeader hdr, FILE* rom_file)
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

	virtual ~Mapper0(void)
	{
	}

	virtual void Write(u16 addr, u8 value)
	{
		printf("WARNING: Write to PROM[0x%04x]=0x%02x\n",addr,value);
	}

	virtual u8   Read (u16 addr)
	{
		if(addr<0x8000)
		{
			printf("Unhandled read from mapper0 addr 0x%04x",addr);
			return 0xA5;
		}
		else {
			return PROM[addr&prom_mask];
		}
	}

	virtual void WritePPU(u16 addr, u8 value)
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

	virtual u8   ReadPPU(u16 addr)
	{
		if(addr<0x2000)
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

Mapper  *LoadROM(char *fileName)
{

	FILE *from = fopen(fileName,"rb");

	if(!from)
	{
		MessageBoxEx(hMainWnd,_T("Cannot open ROM file."),_T("ERROR"),MB_OK,0);
		return NULL;
	}

	NesROMHeader rom;

	if(fread(&rom,16,1,from)!=1)
	{
		MessageBoxEx(hMainWnd,_T("Error reading ROM header."),_T("ERROR"),MB_OK,0);
		fclose(from);
		return NULL;
	}

	if(memcmp(rom.fileID,"NES\x1a",4)!=0)
	{
		MessageBoxEx(hMainWnd,_T("Invalid ROM header."),_T("ERROR"),MB_OK,0);
		fclose(from);
		return NULL;
	}

	int mapperId = (rom.mapper_hi<<4) | rom.mapper_lo;

	switch(mapperId)
	{
	case 0:
		return new Mapper0(rom,from);
	case 1:
		return new Mapper1(rom,from);
	}

	MessageBoxEx(hMainWnd,_T("Unknown or unimplemented Mappter type."),_T("ERROR"),MB_OK,0);
	fclose(from);
	return NULL;
}
