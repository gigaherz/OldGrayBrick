#pragma once

#include "Types.H"

#include <windows.h>

class Display
{
public:
	virtual bool Init(HWND displayWindow, bool fullScreen)=0;
	virtual bool Render()=0;
	virtual bool PretendRender()=0;
	virtual void Close()=0;
	virtual void SetScanline(int line, int *data)=0;
};

class Core
{
	bool isInit;
	bool running;
	bool hadVSync;

	u8 keyStates[256];

public:
	Core(void);
	~Core(void);

	void Init(char *romFile);
	void Close();
	void Reset();
    void SoftReset();
	void Start();
	void Stop();

	void Emulate();

	void DoVSync();

	void SetKey(int which, u8 bit);
	u8   GetKey(int which);

};

class Memory
{
private:
	u8 WorkRam[0x800];

public:
	Memory(void);
	~Memory(void);

	void Init();
	void Reset();
    void SoftReset();
	void Close();

	void Write(u16 addr, u8 value);
	u8   Read (u16 addr);
};

class Mapper
{
public:
	virtual ~Mapper(void)=0;
	virtual void Write(u16 addr, u8 value)=0;
	virtual u8   Read (u16 addr)=0;

	virtual void WritePPU(u16 addr, u8 value)=0;
	virtual u8   ReadPPU (u16 addr)=0;

	virtual void Init()=0;
	virtual void Reset()=0;
	virtual void Close()=0;
    virtual void SoftReset() = 0;

	virtual void Emulate(u32 clocks)=0;

	virtual int VMode()=0;
};

class Cpu
{
public:

	//Bits  Name  Expl.
	u16     PC;// Program Counter

	u8      A; // Accumulator

	u8      X; // Index Register X
	u8      Y; // Index Register Y

	u8      S; // Stack Pointer (see below)

	u8      P; // Processor Status Register (see below)

    int totalCycles;

private:

	// Stack management
	void Push(u8 value);
	u8   Pop();

	// Reads a byte at PC and increments it.
	u8   Fetch();

	int IRQ;

	int stall;

	void __forceinline SetFlag(int f, u8 v);

    void PrintInstructionInfo1(FILE* f, u16 addr, u8 op, u8 byte2, u8 byte3);

public:
	Cpu(void);
	virtual ~Cpu(void);

	void Init();
	void Reset();
    void SoftReset();
	void Close();

	void Stall(int cycles);

	// returns the number of clock cycles used by the step
	int Step();

	void NMI();
	void Interrupt();

    u8 MemoryRead8(int addr);
    void MemoryWrite8(int addr, u8 value);
};

class Ppu
{
	u8 vram_latch;
	u8 vram[2048];

	u8 pal_ram[0x20]; // I know it uses LESS than that

	u8  spr[256];
	u32 spr_addr;
	u32 ctrl1;
	u32 ctrl2;
	u32 status;

	u32 scroll1;
	u32 scroll2;
	u32 vram_addr;

	u32 write_num;

	int vmode;

	float tClocks;

	int scanline;
	int frame;

	u8 latch_2005;
	u8 latch_2006;

public:
	Ppu(void);
	~Ppu(void);

	void Init(int mode);
	void Reset();
    void SoftReset();
	void Close();

	void Emulate(u32 clocks);

	void Write(u16 addr, u8 value);
	u8   Read (u16 addr);

	void WritePPU(u16 addr, u8 value);
	u8   ReadPPU (u16 addr);
	void WriteVRAM(u16 addr, u8 value);
	u8   ReadVRAM (u16 addr);
};

class Apu
{
public:
	Apu(void);
	~Apu(void);

	void Init(int vmode, s32 output_sample_rate);
	void Reset();
    void SoftReset();
	void Close();

	void Emulate(u32 clocks); //APU clock rate = 1789800Hz (DDR'd)

	void Write(u16 addr, u8 value);
	u8   Read (u16 addr);

private:
	void Frame_IRQ();
	void DMC_IRQ();
	u8   DMA_Read(u16 addr);
	void Write_Out(s32 sample);

};

class Pad
{
	u8 buffer[2];
	u8 bit;
	u8 strobe;
public:
	Pad(void);
	~Pad(void);

	void Init();
	void Close();

	void Fetch();
	void Strobe(u8 value);
	u8   Poll(u8 which);
};

Display *GetGDIDisplay();
Mapper  *LoadROM(char *fileName);

// Emulator parts
extern Display*display;
extern Core   *core;
extern Memory *memory;
extern Mapper *mapper;
extern Cpu    *cpu;
extern Ppu    *ppu;
extern Apu    *apu;
extern Pad    *pad;

#define WIDTH 256
#define HEIGHT 240

extern HWND hMainWnd;