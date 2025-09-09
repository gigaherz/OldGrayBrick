#pragma once

//Byte     Contents
//---------------------------------------------------------------------------
//0-3      String "NES^Z" used to recognize .NES files.
//4        Number of 16kB ROM banks.
//5        Number of 8kB VROM banks.
//6        bit 0     1 for vertical mirroring, 0 for horizontal mirroring.
//         bit 1     1 for battery-backed RAM at $6000-$7FFF.
//         bit 2     1 for a 512-byte trainer at $7000-$71FF.
//         bit 3     1 for a four-screen VRAM layout. 
//         bit 4-7   Four lower bits of ROM Mapper Type.
//7        bit 0     1 for VS-System cartridges.
//         bit 1-3   Reserved, must be zeroes!
//         bit 4-7   Four higher bits of ROM Mapper Type.
//8        Number of 8kB RAM banks. For compatibility with the previous
//         versions of the .NES format, assume 1x8kB RAM page when this
//         byte is zero.
//9        bit 0     1 for PAL cartridges, otherwise assume NTSC.
//         bit 1-7   Reserved, must be zeroes!
//10-15    Reserved, must be zeroes!
//---------------------------------------------------------------------------
//16-...   ROM banks, in ascending order. If a trainer is present, its
//         512 bytes precede the ROM bank contents.
//...-EOF  VROM banks, in ascending order.
//---------------------------------------------------------------------------
#pragma pack(1)
struct NesROMHeader {

	char fileID[4];
	u8 romBanks; // 16k each
	u8 vromBanks; // 8k each

	u8 mirror:1;
	u8 has_battery:1;
	u8 has_trainer:1;
	u8 four_screen:1;
	u8 mapper_lo:4;

	u8 vs_system:1;
	u8 reserved:3;
	u8 mapper_hi:4;

	u8 ramBanks;

	u8 is_pal:1;
	u8 reserved2:7;

	u8 reserved3[6];
};

C_ASSERT(sizeof(NesROMHeader)==16);

const int bank_offsets0[2][4] = {
	{0x0000,0x0000,0x0400,0x0400}, // hmirror
	{0x0000,0x0400,0x0000,0x0400}, // vmirror
};

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

	Mapper0(NesROMHeader hdr, FILE* rom_file);
	virtual ~Mapper0(void);
	virtual void Write(u16 addr, u8 value);
	virtual u8   Read (u16 addr);
	virtual void WritePPU(u16 addr, u8 value);
	virtual u8   ReadPPU(u16 addr);
	virtual void Init();
	virtual void Reset();
    virtual void SoftReset();
	virtual void Close();
	virtual void Emulate(u32 clocks);
	virtual int VMode();

};


class Mapper1: public Mapper
{
private:
	u8 *PROM;
	u8 *VROM;

	NesROMHeader header;

	int prom_banks;
	int prom_size;
	int prom_mask;

	int vrom_banks;
	int vrom_size;
	int vrom_mask;

	u8 mirror_select;

	u8 prom_mode;
	u8 vrom_mode;

	u8 which_bit;
	u8 value_buffer;

	u8 prom_offset;
	u8 vrom_offset0;
	u8 vrom_offset1;

	u8 prom_swap256_type;
	u8 prom_swap256;
	u8 prom_swap512;

public:

	Mapper1(NesROMHeader hdr, FILE* rom_file);
	virtual ~Mapper1(void);
	virtual void Write(u16 addr, u8 value);
	virtual u8   Read (u16 addr);
	virtual void WritePPU(u16 addr, u8 value);
	virtual u8   ReadPPU(u16 addr);
	virtual void Init();
	virtual void Reset();
    virtual void SoftReset();
	virtual void Close();
	virtual void Emulate(u32 clocks);
	virtual int VMode();

};

class Mapper2 : public Mapper
{
private:
	u8* PROM;
	u8* VROM;

	u8* PROM_AREA0;
	u8* PROM_AREA1;

	NesROMHeader header;

	int prom_banks;
	int prom_size;
	int prom_mask;

	int vrom_banks;
	int vrom_size;
	int vrom_mask;

	u8 which_bank;
public:

	Mapper2(NesROMHeader hdr, FILE* rom_file);
	virtual ~Mapper2(void);
	virtual void Write(u16 addr, u8 value);
	virtual u8   Read(u16 addr);
	virtual void WritePPU(u16 addr, u8 value);
	virtual u8   ReadPPU(u16 addr);
	virtual void Init();
	virtual void Reset();
	virtual void SoftReset();
	virtual void Close();
	virtual void Emulate(u32 clocks);
	virtual int VMode();

};


class Mapper3 : public Mapper
{
private:
	u8* PRAM;
	u8* PROM;
	u8* VROM;

	u8* VROM_AREA0;

	NesROMHeader header;

	int pram_mask;

	int prom_banks;
	int prom_size;
	int prom_mask;

	int vrom_banks;
	int vrom_size;
	int vrom_mask;

	u8 which_bank;
public:

	Mapper3(NesROMHeader hdr, FILE* rom_file);
	virtual ~Mapper3(void);
	virtual void Write(u16 addr, u8 value);
	virtual u8   Read(u16 addr);
	virtual void WritePPU(u16 addr, u8 value);
	virtual u8   ReadPPU(u16 addr);
	virtual void Init();
	virtual void Reset();
	virtual void SoftReset();
	virtual void Close();
	virtual void Emulate(u32 clocks);
	virtual int VMode();

};

class Mapper11 : public Mapper
{
private:
	u8* PROM;
	u8* VROM;

	u8* PROM_AREA0;
	u8* VROM_AREA0;

	NesROMHeader header;

	int prom_banks;
	int prom_size;
	int prom_mask;

	int vrom_banks;
	int vrom_size;
	int vrom_mask;

	u8 p_which_bank;
	u8 v_which_bank;
public:

	Mapper11(NesROMHeader hdr, FILE* rom_file);
	virtual ~Mapper11(void);
	virtual void Write(u16 addr, u8 value);
	virtual u8   Read(u16 addr);
	virtual void WritePPU(u16 addr, u8 value);
	virtual u8   ReadPPU(u16 addr);
	virtual void Init();
	virtual void Reset();
	virtual void SoftReset();
	virtual void Close();
	virtual void Emulate(u32 clocks);
	virtual int VMode();

};

