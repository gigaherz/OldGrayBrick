#include "StdAfx.h"
#include "Emu.h"

int ShowDebug = 1;

#define SET_FLAG(f,v)	SetFlag(f,v)

#define SET_FLAG_C(v)   SET_FLAG(0,v)
#define SET_FLAG_Z(v)   SET_FLAG(1,v)
#define SET_FLAG_I(v)   SET_FLAG(2,v)
#define SET_FLAG_D(v)   SET_FLAG(3,v)
#define SET_FLAG_V(v)   SET_FLAG(6,v)
#define SET_FLAG_N(v)   SET_FLAG(7,v)

#define FLAG(f)  ((P>>f)&1)
#define FLAG_C   FLAG(0)
#define FLAG_Z   FLAG(1)
#define FLAG_I   FLAG(2)
#define FLAG_D   FLAG(3)
#define FLAG_B   FLAG(4)
#define FLAG_V   FLAG(6)
#define FLAG_N   FLAG(7)

#define CALC_FLAG_N(r) SET_FLAG_N((r)>>7)
#define CALC_FLAG_Z(r) SET_FLAG_Z(((r)&0xFF)==0)
#define CALC_FLAG_C(r) SET_FLAG_C(((r)>>8)^((r)>>15))
#define CALC_FLAG_CB(r) SET_FLAG_C(~((r)>>15))
#define CALC_FLAG_V(r) SET_FLAG_V(((r) > 127) || ((r) < -128))

FILE* cpuLog = nullptr;

void __forceinline Cpu::SetFlag(int f, u8 v)
{
    P &= ~(1 << f);
    P |= ((v & 1) << f);
}

Cpu::Cpu(void)
{
    stall = 0;
    totalCycles = 0;
}

Cpu::~Cpu(void)
{
}

void Cpu::Init()
{
    Reset();
    cpuLog = stdout; // fopen("i:\\yanese_cpu.log", "w");
}

void Cpu::Close()
{
    //if (cpuLog) fclose(cpuLog);
}

// resets the register contents to the initial state
void Cpu::Reset()
{
    A = 0;
    X = 0;
    Y = 0;
    P = 0x24;
    S = 0xFD; // 0xFF;
    PC = 0xC000; //*/ memory->Read(0xFFFC) | (memory->Read(0xFFFD) << 8);
    SET_FLAG_I(1);
}

void Cpu::Push(u8 value)
{
    memory->Write(0x100 + S, value);
    S--;
}

u8   Cpu::Pop()
{
    S++;
    return memory->Read(0x100 + S);
}

u8   Cpu::Fetch()
{
    u8 value = memory->Read(PC);
    PC += 1;
    return value;
}

void Cpu::Stall(int cycles)
{
    stall += cycles;
}

void Cpu::Interrupt()
{
    IRQ++;
}

void Cpu::NMI()
{
    Push(PC >> 8);
    Push((u8)PC);
    Push(P & 0xDF);
    PC = memory->Read(0xFFFA) | (memory->Read(0xFFFB) << 8);;
    SET_FLAG_I(1);
}

// returns the number of clock cycles used by the step
int Cpu::Step()
{

    if (stall > 0)
    {
        stall = 0;
        return stall;
    }

    if (IRQ && (!FLAG_I))
    {
        IRQ--;
        Push(PC >> 8);
        Push((u8)PC);
        Push(P & 0xDF);
        PC = memory->Read(0xFFFE) | (memory->Read(0xFFFF) << 8);
        SET_FLAG_I(1);
    }

    //printf("PREV.: PC=%04x; A=%02x; X=%02x; Y=%02x; P=%02x\n",PC,A,X,Y,P);

    int old_PC = PC;
    int old_M0 = memory->Read(PC + 0);
    int old_M1 = memory->Read(PC + 1);
    int old_M2 = memory->Read(PC + 2);

    if (ShowDebug >= 1)
    {
        PrintInstructionInfo1(cpuLog, old_PC, old_M0, old_M1, old_M2);
    }

    u8 opcode = Fetch();
    s32 new_A = A;
    s32 new_X = X;
    s32 new_Y = Y;
    s32 new_T = 0; //temp var

    u8  p1;
    u16 a1;
    u16 a2;
    s8  d1;

    bool cond = false;

    int cycles = 2;

#define SIGN_EXTEND(a) ((s32)(s32)(a))

#define ADDR_ZERO_PAGE() a1 = Fetch()
#define ADDR_ZERO_PAGE_X() a1 = (Fetch() + X) & 0xFF
#define ADDR_ZERO_PAGE_Y() a1 = (Fetch() + Y) & 0xFF
#define ADDR_ABSOLUTE() a1 = Fetch() | (((u16)Fetch()) << 8)
#define ADDR_INDIRECT_X() a1 = Fetch() + X; a2 = memory->Read(a1 & 0xFF) | (((u16)memory->Read((a1 + 1) & 0xFF)) << 8)
#define ADDR_INDIRECT() a1 = Fetch(); a2 = memory->Read(a1) | (((u16)memory->Read(a1 + 1)) << 8)

#define FETCH_IMMEDIATE() p1 = Fetch()
#define FETCH_ZERO_PAGE()   ADDR_ZERO_PAGE(); p1 = memory->Read(a1)
#define FETCH_ZERO_PAGE_X() ADDR_ZERO_PAGE_X(); p1 = memory->Read(a1)
#define FETCH_ZERO_PAGE_Y() ADDR_ZERO_PAGE_Y(); p1 = memory->Read(a1)
#define FETCH_ABSOLUTE()    ADDR_ABSOLUTE(); p1 = memory->Read(a1)
#define FETCH_ABSOLUTE_X()  ADDR_ABSOLUTE(); p1 = memory->Read(a1 + X)
#define FETCH_ABSOLUTE_Y()  ADDR_ABSOLUTE(); p1 = memory->Read(a1 + Y)
#define FETCH_INDIRECT_X()  ADDR_INDIRECT_X(); p1 = memory->Read(a2)
#define FETCH_INDIRECT_Y()  ADDR_INDIRECT(); p1 = memory->Read(a2 + Y)

#define STORE_A() memory->Write(a1, A)
#define STORE_T() memory->Write(a1, (u8)new_T)

    switch (opcode)
    {
        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        // CPU Memory and Register Transfers

        // Register to Register Transfer

    case 0x98: // TYA         Transfer Y to Accumulator    
        new_A = Y;
        cycles = 2; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xA8: // TAY         Transfer Accumulator to Y    
        new_Y = A;
        cycles = 2; //  
        CALC_FLAG_N(new_Y); CALC_FLAG_Z(new_Y);
        break;

    case 0x8A: // TXA         Transfer X to Accumulator    
        new_A = X;
        cycles = 2; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x9A: // TXS         Transfer X to Stack pointer  
        S = X;
        cycles = 2; //  
        break;
    case 0xAA: // TAX         Transfer Accumulator to X    
        new_X = A;
        cycles = 2; //  
        CALC_FLAG_N(new_X); CALC_FLAG_Z(new_X);
        break;
    case 0xBA: // TSX         Transfer Stack pointer to X  
        new_X = S;
        cycles = 2; //  
        CALC_FLAG_N(new_X); CALC_FLAG_Z(new_X);
        break;

        // Load Register from Memory

    case 0xA9: // LDA #nn     Load A with Immediate     A=nn
        FETCH_IMMEDIATE(); // #nn
        new_A = p1;
        cycles = 2; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xA5: // LDA nn      Load A with Zero Page     A=[nn]
        FETCH_ZERO_PAGE(); // nn
        new_A = p1;
        cycles = 3; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xB5: // LDA nn,X    Load A with Zero Page,X   A=[nn+X]
        FETCH_ZERO_PAGE_X(); // nn,X
        new_A = p1;
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xAD: // LDA nnnn    Load A with Absolute      A=[nnnn]
        FETCH_ABSOLUTE();
        new_A = p1;
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xBD: // LDA nnnn,X  Load A with Absolute,X    A=[nnnn+X]
        FETCH_ABSOLUTE_X();
        new_A = p1;
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + X) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xB9: // LDA nnnn,Y  Load A with Absolute,Y    A=[nnnn+Y]
        FETCH_ABSOLUTE_Y();
        new_A = p1;
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xA1: // LDA (nn,X)  Load A with (Indirect,X)  A=[WORD[nn+X]]
        FETCH_INDIRECT_X();
        new_A = p1;
        cycles = 6; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xB1: // LDA (nn),Y  Load A with (Indirect),Y  A=[WORD[nn]+Y]
        FETCH_INDIRECT_Y();
        new_A = p1;
        cycles = 5; // *
        if ((a2 >> 8) < ((a2 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xA2: // LDX #nn     Load X with Immediate     X=nn
        FETCH_IMMEDIATE(); // #nn
        new_X = p1;
        cycles = 2; //  
        CALC_FLAG_N(new_X); CALC_FLAG_Z(new_X);
        break;
    case 0xA6: // LDX nn      Load X with Zero Page     X=[nn]
        FETCH_ZERO_PAGE(); // nn
        new_X = p1;
        cycles = 3; //  
        CALC_FLAG_N(new_X); CALC_FLAG_Z(new_X);
        break;
    case 0xB6: // LDX nn,Y    Load X with Zero Page,Y   X=[nn+Y]
        FETCH_ZERO_PAGE_Y(); // nn,Y
        new_X = p1;
        cycles = 4; //  
        CALC_FLAG_N(new_X); CALC_FLAG_Z(new_X);
        break;
    case 0xAE: // LDX nnnn    Load X with Absolute      X=[nnnn]
        FETCH_ABSOLUTE();
        new_X = p1;
        cycles = 4; //  
        CALC_FLAG_N(new_X); CALC_FLAG_Z(new_X);
        break;
    case 0xBE: // LDX nnnn,Y  Load X with Absolute,Y    X=[nnnn+Y]
        FETCH_ABSOLUTE_Y();
        new_X = p1;
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_X); CALC_FLAG_Z(new_X);
        break;
    case 0xA0: // LDY #nn     Load Y with Immediate     Y=nn
        FETCH_IMMEDIATE(); // #nn
        new_Y = p1;
        cycles = 2; //  
        CALC_FLAG_N(new_Y); CALC_FLAG_Z(new_Y);
        break;
    case 0xA4: // LDY nn      Load Y with Zero Page     Y=[nn]
        FETCH_ZERO_PAGE(); // nn
        new_Y = p1;
        cycles = 3; //  
        CALC_FLAG_N(new_Y); CALC_FLAG_Z(new_Y);
        break;
    case 0xB4: // LDY nn,X    Load Y with Zero Page,X   Y=[nn+X]
        FETCH_ZERO_PAGE_X(); // nn,X
        new_Y = p1;
        cycles = 4; //  
        CALC_FLAG_N(new_Y); CALC_FLAG_Z(new_Y);
        break;
    case 0xAC: // LDY nnnn    Load Y with Absolute      Y=[nnnn]
        FETCH_ABSOLUTE();
        new_Y = p1;
        cycles = 4; //  
        CALC_FLAG_N(new_Y); CALC_FLAG_Z(new_Y);
        break;
    case 0xBC: // LDY nnnn,X  Load Y with Absolute,X    Y=[nnnn+X]
        FETCH_ABSOLUTE_X();
        new_Y = p1;
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + X) >> 8)) cycles++;
        CALC_FLAG_N(new_Y); CALC_FLAG_Z(new_Y);
        break;

        // * Add one cycle if indexing crosses a page boundary.

        // Store Register in Memory

    case 0x85: // STA nn      Store A in Zero Page     [nn]=A
        ADDR_ZERO_PAGE();
        STORE_A(); // nn
        cycles = 3; //  
        break;
    case 0x95: // STA nn,X    Store A in Zero Page,X   [nn+X]=A
        ADDR_ZERO_PAGE_X();
        STORE_A(); // nn,X
        cycles = 4; //  
        break;
    case 0x8D: // STA nnnn    Store A in Absolute      [nnnn]=A
        ADDR_ABSOLUTE(); // nnnn
        STORE_A();
        cycles = 4; //  
        break;
    case 0x9D: // STA nnnn,X  Store A in Absolute,X    [nnnn+X]=A
        ADDR_ABSOLUTE(); // nnnn,X
        memory->Write(a1 + X, A);
        cycles = 5; //  
        break;
    case 0x99: // STA nnnn,Y  Store A in Absolute,Y    [nnnn+Y]=A
        ADDR_ABSOLUTE(); // nnnn,Y
        memory->Write(a1 + Y, A);
        cycles = 5; //  
        break;
    case 0x81: // STA (nn,X)  Store A in (Indirect,X)  [[nn+x]]=A
        ADDR_INDIRECT_X();
        memory->Write(a2, A);
        cycles = 6; //  
        break;
    case 0x91: // STA (nn),Y  Store A in (Indirect),Y  [[nn]+y]=A
        ADDR_INDIRECT();
        memory->Write(a2 + Y, A);
        cycles = 6; //  
        break;
    case 0x86: // STX nn      Store X in Zero Page     [nn]=X
        ADDR_ZERO_PAGE();
        memory->Write(a1, X); // nn
        cycles = 3; //  
        break;
    case 0x96: // STX nn,Y    Store X in Zero Page,Y   [nn+Y]=X
        ADDR_ZERO_PAGE_Y();
        memory->Write(a1, X); // nn,Y
        cycles = 4; //  
        break;
    case 0x8E: // STX nnnn    Store X in Absolute      [nnnn]=X
        ADDR_ABSOLUTE(); // nnnn
        memory->Write(a1, X);
        cycles = 4; //  
        break;
    case 0x84: // STY nn      Store Y in Zero Page     [nn]=Y
        ADDR_ZERO_PAGE();
        memory->Write(a1, Y); // nn
        cycles = 3; //  
        break;
    case 0x94: // STY nn,X    Store Y in Zero Page,X   [nn+X]=Y
        ADDR_ZERO_PAGE_X();
        memory->Write(a1, Y); // nn,X
        cycles = 4; //  
        break;
    case 0x8C: // STY nnnn    Store Y in Absolute      [nnnn]=Y
        ADDR_ABSOLUTE(); // nnnn
        memory->Write(a1, Y);
        cycles = 4; //  
        break;

        // Push/Pull

    case 0x48: // PHA         Push accumulator on stack        [S]=A
        Push(A);
        cycles = 3; //  
        break;
    case 0x08: // PHP         Push processor status on stack   [S]=P
        Push(P | 0x30);
        cycles = 3; //  
        break;
    case 0x68: // PLA         Pull accumulator from stack      A=[S]
        new_A = Pop();
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x28: // PLP         Pull processor status from stack P=[S]
        P = (Pop() & 0xCF) | 0x20;
        cycles = 4; //  
        break;

        // Notes: PLA sets Z and N according to content of A. The B-flag and unused flags cannot be changed by PLP, these flags are always written as "1" by PHP.


        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        // CPU Arithmetic/Logical Operations

        // Add memory to accumulator with carry

#define ADC() (new_A=SIGN_EXTEND(A)+SIGN_EXTEND(p1)+FLAG_C)

    case 0x69: // ADC #nn     Add Immediate           A=A+C+nn
        FETCH_IMMEDIATE(); // #nn
        ADC();
        cycles = 2; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_C(new_A); CALC_FLAG_V(new_A);
        break;
    case 0x65: // ADC nn      Add Zero Page           A=A+C+[nn]
        FETCH_ZERO_PAGE(); // nn
        ADC();
        cycles = 3; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_C(new_A); CALC_FLAG_V(new_A);
        break;
    case 0x75: // ADC nn,X    Add Zero Page,X         A=A+C+[nn+X]
        FETCH_ZERO_PAGE_X(); // nn,X
        ADC();
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_C(new_A); CALC_FLAG_V(new_A);
        break;
    case 0x6D: // ADC nnnn    Add Absolute            A=A+C+[nnnn]
        FETCH_ABSOLUTE();
        ADC();
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_C(new_A); CALC_FLAG_V(new_A);
        break;
    case 0x7D: // ADC nnnn,X  Add Absolute,X          A=A+C+[nnnn+X]
        FETCH_ABSOLUTE_X();
        ADC();
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + X) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_C(new_A); CALC_FLAG_V(new_A);
        break;
    case 0x79: // ADC nnnn,Y  Add Absolute,Y          A=A+C+[nnnn+Y]
        FETCH_ABSOLUTE_Y();
        ADC();
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_C(new_A); CALC_FLAG_V(new_A);
        break;
    case 0x61: // ADC (nn,X)  Add (Indirect,X)        A=A+C+[[nn+X]]
        FETCH_INDIRECT_X();
        ADC();
        cycles = 6; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_C(new_A); CALC_FLAG_V(new_A);
        break;
    case 0x71: // ADC (nn),Y  Add (Indirect),Y        A=A+C+[[nn]+Y]
        FETCH_INDIRECT_Y();
        ADC();
        cycles = 5; // *
        if ((a2 >> 8) < ((a2 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_C(new_A); CALC_FLAG_V(new_A);
        break;

        // * Add one cycle if indexing crosses a page boundary.

        // Subtract memory from accumulator with borrow

#define SBC() (new_A=SIGN_EXTEND(A)-SIGN_EXTEND(p1)-1+FLAG_C)

    case 0xE9: // SBC #nn     Subtract Immediate      A=A+C-1-nn
        FETCH_IMMEDIATE(); // #nn
        SBC();
        cycles = 2; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_CB(new_A); CALC_FLAG_V(new_A);
        break;
    case 0xE5: // SBC nn      Subtract Zero Page      A=A+C-1-[nn]
        FETCH_ZERO_PAGE(); // nn
        SBC();
        cycles = 3; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_CB(new_A); CALC_FLAG_V(new_A);
        break;
    case 0xF5: // SBC nn,X    Subtract Zero Page,X    A=A+C-1-[nn+X]
        FETCH_ZERO_PAGE_X(); // nn,X
        SBC();
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_CB(new_A); CALC_FLAG_V(new_A);
        break;
    case 0xED: // SBC nnnn    Subtract Absolute       A=A+C-1-[nnnn]
        FETCH_ABSOLUTE();
        SBC();
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_CB(new_A); CALC_FLAG_V(new_A);
        break;
    case 0xFD: // SBC nnnn,X  Subtract Absolute,X     A=A+C-1-[nnnn+X]
        FETCH_ABSOLUTE_X();
        SBC();
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + X) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_CB(new_A); CALC_FLAG_V(new_A);
        break;
    case 0xF9: // SBC nnnn,Y  Subtract Absolute,Y     A=A+C-1-[nnnn+Y]
        FETCH_ABSOLUTE_Y();
        SBC();
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_CB(new_A); CALC_FLAG_V(new_A);
        break;
    case 0xE1: // SBC (nn,X)  Subtract (Indirect,X)   A=A+C-1-[[nn+X]]
        FETCH_INDIRECT_X();
        SBC();
        cycles = 6; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_CB(new_A); CALC_FLAG_V(new_A);
        break;
    case 0xF1: // SBC (nn),Y  Subtract (Indirect),Y   A=A+C-1-[[nn]+Y]
        FETCH_INDIRECT_Y();
        SBC();
        cycles = 5; // *
        if ((a2 >> 8) < ((a2 + X) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_CB(new_A); CALC_FLAG_V(new_A);
        break;

        // * Add one cycle if indexing crosses a page boundary.
        // Note: Compared with normal 80x86 and Z80 CPUs, incoming and resulting Carry Flag are reversed.

        // Logical AND memory with accumulator

#define AND() (new_A = A&p1)

    case 0x29: // AND #nn     AND Immediate      A=A AND nn
        FETCH_IMMEDIATE(); // #nn
        AND();
        cycles = 2; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x25: // AND nn      AND Zero Page      A=A AND [nn]
        FETCH_ZERO_PAGE(); // nn
        AND();
        cycles = 3; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x35: // AND nn,X    AND Zero Page,X    A=A AND [nn+X]
        FETCH_ZERO_PAGE_X(); // nn,X
        AND();
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x2D: // AND nnnn    AND Absolute       A=A AND [nnnn]
        FETCH_ABSOLUTE();
        AND();
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x3D: // AND nnnn,X  AND Absolute,X     A=A AND [nnnn+X]
        FETCH_ABSOLUTE_X();
        AND();
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + X) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x39: // AND nnnn,Y  AND Absolute,Y     A=A AND [nnnn+Y]
        FETCH_ABSOLUTE_Y();
        AND();
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x21: // AND (nn,X)  AND (Indirect,X)   A=A AND [[nn+X]]
        FETCH_INDIRECT_X();
        AND();
        cycles = 6; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x31: // AND (nn),Y  AND (Indirect),Y   A=A AND [[nn]+Y]
        FETCH_INDIRECT_Y();
        AND();
        cycles = 5; // *
        if ((a2 >> 8) < ((a2 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;

        // * Add one cycle if indexing crosses a page boundary.

        // Exclusive-OR memory with accumulator

    case 0x49: // EOR #nn     XOR Immediate      A=A XOR nn
        FETCH_IMMEDIATE(); // #nn
        new_A = A^p1;
        cycles = 2; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x45: // EOR nn      XOR Zero Page      A=A XOR [nn]
        FETCH_ZERO_PAGE(); // nn
        new_A = A^p1;
        cycles = 3; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x55: // EOR nn,X    XOR Zero Page,X    A=A XOR [nn+X]
        FETCH_ZERO_PAGE_X(); // nn,X
        new_A = A^p1;
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x4D: // EOR nnnn    XOR Absolute       A=A XOR [nnnn]
        FETCH_ABSOLUTE();
        new_A = A^p1;
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x5D: // EOR nnnn,X  XOR Absolute,X     A=A XOR [nnnn+X]
        FETCH_ABSOLUTE_X();
        new_A = A^p1;
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + X) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x59: // EOR nnnn,Y  XOR Absolute,Y     A=A XOR [nnnn+Y]
        FETCH_ABSOLUTE_Y();
        new_A = A^p1;
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x41: // EOR (nn,X)  XOR (Indirect,X)   A=A XOR [[nn+X]]
        FETCH_INDIRECT_X();
        new_A = A^p1;
        cycles = 6; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x51: // EOR (nn),Y  XOR (Indirect),Y   A=A XOR [[nn]+Y]
        FETCH_INDIRECT_Y();
        new_A = A^p1;
        cycles = 5; // *
        if ((a2 >> 8) < ((a2 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;

        // * Add one cycle if indexing crosses a page boundary.

        // Logical OR memory with accumulator

    case 0x09: // ORA #nn     OR Immediate       A=A OR nn
        FETCH_IMMEDIATE(); // #nn
        new_A = A | p1;
        cycles = 2; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x05: // ORA nn      OR Zero Page       A=A OR [nn]
        FETCH_ZERO_PAGE(); // nn
        new_A = A | p1;
        cycles = 3; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x15: // ORA nn,X    OR Zero Page,X     A=A OR [nn+X]
        FETCH_ZERO_PAGE_X(); // nn,X
        new_A = A | p1;
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x0D: // ORA nnnn    OR Absolute        A=A OR [nnnn]
        FETCH_ABSOLUTE();
        new_A = A | p1;
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x1D: // ORA nnnn,X  OR Absolute,X      A=A OR [nnnn+X]
        FETCH_ABSOLUTE_X();
        new_A = A | p1;
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + X) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x19: // ORA nnnn,Y  OR Absolute,Y      A=A OR [nnnn+Y]
        FETCH_ABSOLUTE_Y();
        new_A = A | p1;
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x01: // ORA (nn,X)  OR (Indirect,X)    A=A OR [[nn+X]]
        FETCH_INDIRECT_X();
        new_A = A | p1;
        cycles = 6; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0x11: // ORA (nn),Y  OR (Indirect),Y    A=A OR [[nn]+Y]
        FETCH_INDIRECT_Y();
        new_A = A | p1;
        cycles = 5; // *
        if ((a2 >> 8) < ((a2 + X) >> 8)) cycles++;
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;

        // * Add one cycle if indexing crosses a page boundary.

        // Compare

    case 0xC9: // CMP #nn     Compare A with Immediate     A-nn
        FETCH_IMMEDIATE(); // #nn
        new_T = A - p1;
        cycles = 2; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xC5: // CMP nn      Compare A with Zero Page     A-[nn]
        FETCH_ZERO_PAGE(); // nn
        new_T = A - p1;
        cycles = 3; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xD5: // CMP nn,X    Compare A with Zero Page,X   A-[nn+X]
        FETCH_ZERO_PAGE_X(); // nn,X
        new_T = A - p1;
        cycles = 4; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xCD: // CMP nnnn    Compare A with Absolute      A-[nnnn]
        FETCH_ABSOLUTE();
        new_T = A - p1;
        cycles = 4; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xDD: // CMP nnnn,X  Compare A with Absolute,X    A-[nnnn+X]
        FETCH_ABSOLUTE_X();
        new_T = A - p1;
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + X) >> 8)) cycles++;
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xD9: // CMP nnnn,Y  Compare A with Absolute,Y    A-[nnnn+Y]
        FETCH_ABSOLUTE_Y();
        new_T = A - p1;
        cycles = 4; // *
        if ((a1 >> 8) < ((a1 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xC1: // CMP (nn,X)  Compare A with (Indirect,X)  A-[[nn+X]]
        FETCH_INDIRECT_X();
        new_T = A - p1;
        cycles = 6; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xD1: // CMP (nn),Y  Compare A with (Indirect),Y  A-[[nn]+Y]
        FETCH_INDIRECT_Y();
        new_T = A - p1;
        cycles = 5; // *
        if ((a2 >> 8) < ((a2 + Y) >> 8)) cycles++;
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xE0: // CPX #nn     Compare X with Immediate     X-nn
        FETCH_IMMEDIATE(); // #nn
        new_T = X - p1;
        cycles = 2; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xE4: // CPX nn      Compare X with Zero Page     X-[nn]
        FETCH_ZERO_PAGE(); // nn
        new_T = X - p1;
        cycles = 3; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xEC: // CPX nnnn    Compare X with Absolute      X-[nnnn]
        FETCH_ABSOLUTE();
        new_T = X - p1;
        cycles = 4; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xC0: // CPY #nn     Compare Y with Immediate     Y-nn
        FETCH_IMMEDIATE(); // #nn
        new_T = X - p1;
        cycles = 2; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xC4: // CPY nn      Compare Y with Zero Page     Y-[nn]
        FETCH_ZERO_PAGE(); // nn
        new_T = X - p1;
        cycles = 3; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;
    case 0xCC: // CPY nnnn    Compare Y with Absolute      Y-[nnnn]
        FETCH_ABSOLUTE();
        new_T = X - p1;
        cycles = 4; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_CB(new_T);
        break;

        // * Add one cycle if indexing crosses a page boundary.
        // Note: Compared with normal 80x86 and Z80 CPUs, resulting Carry Flag is reversed.

        // Bit Test

    case 0x24: // BIT nn      Bit Test   A AND [nn], N=[nn].7, V=[nn].6
        FETCH_ZERO_PAGE(); // nn
        new_T = p1&A;
        cycles = 3; //  
        SET_FLAG_N((p1 >> 7) & 1); SET_FLAG_V((p1 >> 6) & 1); CALC_FLAG_Z(new_T);
        break;
    case 0x2C: // BIT nnnn    Bit Test   A AND [..], N=[..].7, V=[..].6
        FETCH_ABSOLUTE();
        new_T = p1&A;
        cycles = 4; //  
        SET_FLAG_N((p1 >> 7) & 1); SET_FLAG_V((p1 >> 6) & 1); CALC_FLAG_Z(new_T);
        break;


        // Increment by one

    case 0xE6: // INC nn      Increment Zero Page    [nn]=[nn]+1
        FETCH_ZERO_PAGE();
        p1 = memory->Read(a1); // nn
        new_T = p1 + 1;
        STORE_T();
        cycles = 5; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T);
        break;
    case 0xF6: // INC nn,X    Increment Zero Page,X  [nn+X]=[nn+X]+1
        FETCH_ZERO_PAGE_X();
        p1 = memory->Read(a1); // nn,X
        new_T = p1 + 1;
        STORE_T();
        cycles = 6; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T);
        break;
    case 0xEE: // INC nnnn    Increment Absolute     [nnnn]=[nnnn]+1
        FETCH_ABSOLUTE();
        new_T = p1 + 1;
        STORE_T();
        cycles = 6; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T);
        break;
    case 0xFE: // INC nnnn,X  Increment Absolute,X   [nnnn+X]=[nnnn+X]+1
        FETCH_ABSOLUTE_X(); // nnnn,X
        new_T = p1 + 1;
        STORE_T();
        cycles = 7; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T);
        break;
    case 0xE8: // INX         Increment X            X=X+1
        new_X = X + 1;
        cycles = 2; //  
        CALC_FLAG_N(new_X); CALC_FLAG_Z(new_X);
        break;
    case 0xC8: // INY         Increment Y            Y=Y+1
        new_Y = Y + 1;
        cycles = 2; //  
        CALC_FLAG_N(new_Y); CALC_FLAG_Z(new_Y);
        break;


        // Decrement by one

    case 0xC6: // DEC nn      Decrement Zero Page    [nn]=[nn]-1
        FETCH_ZERO_PAGE();
        new_T = p1 - 1;
        STORE_T();
        cycles = 5; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T);
        break;
    case 0xD6: // DEC nn,X    Decrement Zero Page,X  [nn+X]=[nn+X]-1
        FETCH_ZERO_PAGE_X();
        new_T = p1 - 1;
        STORE_T();
        cycles = 6; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T);
        break;
    case 0xCE: // DEC nnnn    Decrement Absolute     [nnnn]=[nnnn]-1
        FETCH_ABSOLUTE();
        new_T = p1 - 1;
        STORE_T();
        cycles = 6; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T);
        break;
    case 0xDE: // DEC nnnn,X  Decrement Absolute,X   [nnnn+X]=[nnnn+X]-1
        FETCH_ABSOLUTE_X(); // nnnn,X
        new_T = p1 - 1;
        STORE_T();
        cycles = 7; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T);
        break;
    case 0xCA: // DEX         Decrement X            X=X-1
        new_X = X - 1;
        cycles = 2; //  
        CALC_FLAG_N(new_X); CALC_FLAG_Z(new_X);
        break;
    case 0x88: // DEY         Decrement Y            Y=Y-1
        new_Y = Y - 1;
        cycles = 2; //  
        CALC_FLAG_N(new_Y); CALC_FLAG_Z(new_Y);
        break;



        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        // CPU Rotate and Shift Instructions

        // Shift Left

    case 0x0A: // ASL A       Shift Left Accumulator   SHL A
        new_A = A << 1; // A
        cycles = 2; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_C(new_A);
        break;
    case 0x06: // ASL nn      Shift Left Zero Page     SHL [nn]
        FETCH_ZERO_PAGE();
        new_T = p1 << 1;
        STORE_T();
        cycles = 5; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;
    case 0x16: // ASL nn,X    Shift Left Zero Page,X   SHL [nn+X]
        FETCH_ZERO_PAGE_X();
        new_T = p1 << 1;
        STORE_T();
        cycles = 6; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;
    case 0x0E: // ASL nnnn    Shift Left Absolute      SHL [nnnn]
        FETCH_ABSOLUTE();
        new_T = p1 << 1;
        STORE_T();
        cycles = 6; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;
    case 0x1E: // ASL nnnn,X  Shift Left Absolute,X    SHL [nnnn+X]
        FETCH_ABSOLUTE_X(); // nnnn,X
        new_T = p1 << 1;
        STORE_T();
        cycles = 7; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;


        // Shift Right

    case 0x4A: // LSR A       Shift Right Accumulator  SHR A
        p1 = A; // A
        new_A = A >> 1;
        cycles = 2; //  
        SET_FLAG_N(0); CALC_FLAG_Z(new_A); SET_FLAG_C(p1);
        break;
    case 0x46: // LSR nn      Shift Right Zero Page    SHR [nn]
        FETCH_ZERO_PAGE();
        new_T = p1 >> 1;
        STORE_T();
        cycles = 5; //  
        SET_FLAG_N(0); CALC_FLAG_Z(new_T); SET_FLAG_C(p1);
        break;
    case 0x56: // LSR nn,X    Shift Right Zero Page,X  SHR [nn+X]
        FETCH_ZERO_PAGE_X();
        new_T = p1 >> 1;
        STORE_T();
        cycles = 6; //  
        SET_FLAG_N(0); CALC_FLAG_Z(new_T); SET_FLAG_C(p1);
        break;
    case 0x4E: // LSR nnnn    Shift Right Absolute     SHR [nnnn]
        FETCH_ABSOLUTE();
        new_T = p1 >> 1;
        STORE_T();
        cycles = 6; //  
        SET_FLAG_N(0); CALC_FLAG_Z(new_T); SET_FLAG_C(p1);
        break;
    case 0x5E: // LSR nnnn,X  Shift Right Absolute,X   SHR [nnnn+X]
        ADDR_ABSOLUTE(); // nnnn,X
        a1 += X;
        p1 = memory->Read(a1);
        new_T = p1 >> 1;
        STORE_T();
        cycles = 7; //  
        SET_FLAG_N(0); CALC_FLAG_Z(new_T); SET_FLAG_C(p1);
        break;


        // Rotate Left through Carry

    case 0x2A: // ROL A       Rotate Left Accumulator  RCL A
        new_A = (A << 1) | FLAG_C;
        cycles = 2; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A); CALC_FLAG_C(new_A);
        break;
    case 0x26: // ROL nn      Rotate Left Zero Page    RCL [nn]
        a1 = Fetch();
        p1 = memory->Read(a1); // nn
        new_T = (p1 << 1) | FLAG_C;
        STORE_T();
        cycles = 5; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;
    case 0x36: // ROL nn,X    Rotate Left Zero Page,X  RCL [nn+X]
        a1 = (Fetch() + X) & 0xFF;
        p1 = memory->Read(a1); // nn,X
        new_T = (p1 << 1) | FLAG_C;
        STORE_T();
        cycles = 6; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;
    case 0x2E: // ROL nnnn    Rotate Left Absolute     RCL [nnnn]
        FETCH_ABSOLUTE();
        new_T = (p1 << 1) | FLAG_C;
        STORE_T();
        cycles = 6; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;
    case 0x3E: // ROL nnnn,X  Rotate Left Absolute,X   RCL [nnnn+X]
        ADDR_ABSOLUTE(); // nnnn,X
        a1 += X;
        p1 = memory->Read(a1);
        new_T = (p1 << 1) | FLAG_C;
        STORE_T();
        cycles = 7; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;


        // Rotate Right through Carry

    case 0x6A: // ROR A       Rotate Right Accumulator RCR A
        p1 = A; // A
        new_A = (p1 >> 1) | (FLAG_C << 7);
        cycles = 2; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;
    case 0x66: // ROR nn      Rotate Right Zero Page   RCR [nn]
        a1 = Fetch();
        p1 = memory->Read(a1); // nn
        new_T = (p1 >> 1) | (FLAG_C << 7);
        STORE_T();
        cycles = 5; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;
    case 0x76: // ROR nn,X    Rotate Right Zero Page,X RCR [nn+X]
        a1 = (Fetch() + X) & 0xFF;
        p1 = memory->Read(a1); // nn,X
        new_T = (p1 >> 1) | (FLAG_C << 7);
        STORE_T();
        cycles = 6; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;
    case 0x6E: // ROR nnnn    Rotate Right Absolute    RCR [nnnn]
        FETCH_ABSOLUTE();
        new_T = (p1 >> 1) | (FLAG_C << 7);
        STORE_T();
        cycles = 6; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;
    case 0x7E: // ROR nnnn,X  Rotate Right Absolute,X  RCR [nnnn+X]
        ADDR_ABSOLUTE(); // nnnn,X
        a1 += X;
        p1 = memory->Read(a1);
        new_T = (p1 >> 1) | (FLAG_C << 7);
        STORE_T();
        cycles = 7; //  
        CALC_FLAG_N(new_T); CALC_FLAG_Z(new_T); CALC_FLAG_C(new_T);
        break;


        // Notes:
        // ROR instruction is available on MCS650X microprocessors after June, 1976.
        // ROL and ROL rotate an 8bit value through carry (rotates 9bits in total).


        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        // CPU Jump and Control Instructions

        // Normal Jumps

    case 0x4C: // JMP nnnn    Jump Absolute              PC=nnnn
        PC = Fetch() | (((u16)Fetch()) << 8); // nnnn
        cycles = 3; //  

        break;
    case 0x6C: // JMP (nnnn)  Jump Indirect              PC=WORD[nnnn]
        a1 = Fetch(); // (nnnn)
        a2 = ((u16)Fetch()) << 8;
        PC = memory->Read(a2 | a1) | (memory->Read(a2 | ((a1 + 1) & 0xFF)) << 8);
        cycles = 5; //  

        break;
    case 0x20: // JSR nnnn    Jump and Save Return Addr. [S]=PC+2,PC=nnnn
        new_T = Fetch(); // nnnn
        Push(PC >> 8);
        Push(PC & 0xFF);
        PC = new_T | (((u16)Fetch()) << 8);
        cycles = 6; //  

        break;
    case 0x40: // RTI         Return from BRK/IRQ/NMI    P=[S], PC=[S]
        P = (Pop() & 0xCF) | 0x30;
        PC = Pop() | (Pop() << 8);
        cycles = 6; //  
        break;
    case 0x60: // RTS         Return from Subroutine     PC=[S]+1
        PC = Pop() | (Pop() << 8);
        PC++;
        cycles = 6; //  
        break;

        // Note: RTI cannot modify the B-Flag or the unused flag.
        // Glitch: For JMP [nnnn] the operand word cannot cross page boundaries, ie. JMP [03FFh] would fetch the MSB from [0300h] instead of [0400h]. Very simple workaround would be to place a ALIGN 2 before the data word.

        // Conditional Branches

    case 0x10: // BPL disp    Branch on result plus     if N=0 PC=PC+/-nn
        d1 = Fetch(); // disp
        cond = (!FLAG_N);

        cycles = 2; // *
        if (cond) {
            cycles++;
            if ((PC >> 8) < ((PC + d1) >> 8)) cycles++;
            PC += d1;
        }
        break;
    case 0x30: // BMI disp    Branch on result minus    if N=1 PC=PC+/-nn
        d1 = Fetch(); // disp
        cond = (FLAG_N);

        cycles = 2; // *
        if (cond) {
            cycles++;
            if ((PC >> 8) < ((PC + d1) >> 8)) cycles++;
            PC += d1;
        }
        break;
    case 0x50: // BVC disp    Branch on overflow clear  if V=0 PC=PC+/-nn
        d1 = Fetch(); // disp
        cond = (!FLAG_V);

        cycles = 2; // *
        if (cond) {
            cycles++;
            if ((PC >> 8) < ((PC + d1) >> 8)) cycles++;
            PC += d1;
        }
        break;
    case 0x70: // BVS disp    Branch on overflow set    if V=1 PC=PC+/-nn
        d1 = Fetch(); // disp
        cond = (FLAG_V);

        cycles = 2; // *
        if (cond) {
            cycles++;
            if ((PC >> 8) < ((PC + d1) >> 8)) cycles++;
            PC += d1;
        }
        break;
    case 0x90: // BCC disp    Branch on carry clear     if C=0 PC=PC+/-nn
        d1 = Fetch(); // disp
        cond = (!FLAG_C);

        cycles = 2; // *
        if (cond) {
            cycles++;
            if ((PC >> 8) < ((PC + d1) >> 8)) cycles++;
            PC += d1;
        }
        break;
    case 0xB0: // BCS disp    Branch on carry set       if C=1 PC=PC+/-nn
        d1 = Fetch(); // disp
        cond = (FLAG_C);

        cycles = 2; // *
        if (cond) {
            cycles++;
            if ((PC >> 8) < ((PC + d1) >> 8)) cycles++;
            PC += d1;
        }
        break;
    case 0xD0: // BNE disp    Branch on result not zero if Z=0 PC=PC+/-nn
        d1 = Fetch(); // disp
        cond = (!FLAG_Z);

        cycles = 2; // *
        if (cond) {
            cycles++;
            if ((PC >> 8) < ((PC + d1) >> 8)) cycles++;
            PC += d1;
        }
        break;
    case 0xF0: // BEQ disp    Branch on result zero     if Z=1 PC=PC+/-nn
        d1 = Fetch(); // disp
        cond = (FLAG_Z);

        cycles = 2; // *
        if (cond) {
            cycles++;
            if ((PC >> 8) != ((PC + d1) >> 8)) cycles++;
            PC += d1;
        }
        break;

        // ** The execution time is 2 cycles if the condition is false (no branch executed). Otherwise, 3 cycles if the destination is in the same memory page, or 4 cycles if it crosses a page boundary (see below for exact info).
        // Note: After subtractions (SBC or CMP) carry=set indicates above-or-equal, unlike as for 80x86 and Z80 CPUs. Obviously, this still applies even when using 80XX-style syntax.

        // Interrupts, Exceptions, Breakpoints

    case 0x00: // BRK         Force Break B=1 [S]=PC+1,[S]=P,I=1,PC=[FFFE]
        Push((u8)PC);
        Push(PC >> 8);
        PC = memory->Read(0xFFFE) | (memory->Read(0xFFFF) << 8);
        Push(P | 0x10);
        cycles = 7; //  
        SET_FLAG_I(1);
        break;
        // --        ---1--  ??  IRQ         Interrupt   B=0 [S]=PC,[S]=P,I=1,PC=[FFFE]
        // --        ---1--  ??  NMI         NMI         B=0 [S]=PC,[S]=P,I=1,PC=[FFFA]
        // --        ---1--  T+6 RESET       Reset       PC=[FFFC],I=1

        // Notes: IRQs can be disabled by setting the I-flag, a BRK command, a NMI, and a /RESET signal cannot be masked by setting I.
        // BRK/IRQ/NMI first change the B-flag, then write P to stack, and then set the I-flag, the D-flag is NOT changed and should be cleared by software.
        // The same vector is shared for BRK and IRQ, software can separate between BRK and IRQ by examining the pushed B-flag only.
        // The RTI opcode can be used to return from BRK/IRQ/NMI, note that using the return address from BRK skips one dummy/parameter byte following after the BRK opcode.
        // Software or hardware must take care to acknowledge or reset /IRQ or /NMI signals after processing it.

        // IRQs are executed whenever "/IRQ=LOW AND I=0".
        // NMIs are executed whenever "/NMI changes from HIGH to LOW".

        // If /IRQ is kept LOW then same (old) interrupt is executed again as soon as setting I=0. If /NMI is kept LOW then no further NMIs can be executed.

        // CPU Control

    case 0x18: // CLC         Clear carry flag            C=0
        cycles = 2; //  
        SET_FLAG_C(0);
        break;
    case 0x58: // CLI         Clear interrupt disable bit I=0
        cycles = 2; //  
        SET_FLAG_I(0);
        break;
    case 0xD8: // CLD         Clear decimal mode          D=0
        cycles = 2; //  
        SET_FLAG_D(0);
        break;
    case 0xB8: // CLV         Clear overflow flag         V=0
        cycles = 2; //  
        SET_FLAG_V(0);
        break;
    case 0x38: // SEC         Set carry flag              C=1
        cycles = 2; //  
        SET_FLAG_C(1);
        break;
    case 0x78: // SEI         Set interrupt disable bit   I=1
        cycles = 2; //  
        SET_FLAG_I(1);
        break;
    case 0xF8: // SED         Set decimal mode            D=1
        cycles = 2; //  
        SET_FLAG_D(1);
        break;


        // No Operation

    case 0xEA: // NOP         No operation                No operation
        cycles = 2; //  

        break;


        // Conditional Branch Page Crossing
        // The branch opcode with parameter takes up two bytes, causing the PC to get incremented twice (PC=PC+2), without any extra boundary cycle. The signed parameter is then added to the PC (PC+disp), the extra clock cycle occurs if the addition crosses a page boundary (next or previous 100h-page).


        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        // CPU Illegal Opcodes

        // SAX and LAX

    case 0x87: // SAX nn      STA+STX  [nn]=A AND X
        a1 = Fetch();
        memory->Write(a1, A&X); // nn
        cycles = 3; //  
        break;
    case 0x97: // SAX nn,Y    STA+STX  [nn+Y]=A AND X
        a1 = (Fetch() + Y) & 0xFF;
        memory->Write(a1, A&X); // nn
        cycles = 4; //  
        break;
    case 0x8F: // SAX nnnn    STA+STX  [nnnn]=A AND X
        ADDR_ABSOLUTE(); // nnnn
        memory->Write(a1, A&X); // nn
        cycles = 4; //  
        break;
    case 0x83: // SAX (nn,X)  STA+STX  [WORD[nn+X]]=A AND X
        a1 = (Fetch() + X) & 0xFF; // (nn,X)
        a2 = memory->Read(a1) | (((u16)memory->Read(a1 + 1)) << 8);
        memory->Write(a2, A&X); // nn
        cycles = 6; //  
        break;
    case 0xA7: // LAX nn      LDA+LDX  A,X=[nn]
        FETCH_ZERO_PAGE(); // nn
        new_A = new_X = p1;
        cycles = 3; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xB7: // LAX nn,Y    LDA+LDX  A,X=[nn+Y]
        p1 = memory->Read((Fetch() + Y) & 0xFF); // nn,Y
        new_A = new_X = p1;
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xAF: // LAX nnnn    LDA+LDX  A,X=[nnnn]
        FETCH_ABSOLUTE();
        new_A = new_X = p1;
        cycles = 4; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xBF: // LAX nnnn,X  LDA+LDX  A,X=[nnnn+X]
        FETCH_ABSOLUTE_X();
        new_A = new_X = p1;
        cycles = 4; // *
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xA3: // LAX (nn,X)  LDA+LDX  A,X=[WORD[nn+X]]
        a1 = Fetch() + X; // (nn,X)
        a2 = memory->Read(a1) | (((u16)memory->Read(a1 + 1)) << 8);
        p1 = memory->Read(a2);
        new_A = new_X = p1;
        cycles = 6; //  
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;
    case 0xB3: // LAX (nn),Y  LDA+LDX  A,X=[WORD[nn]+Y]
        a1 = Fetch(); // (nn),Y
        a2 = memory->Read(a1) | (((u16)memory->Read(a1 + 1)) << 8);
        p1 = memory->Read(a2 + Y);
        new_A = new_X = p1;
        cycles = 5; // *
        CALC_FLAG_N(new_A); CALC_FLAG_Z(new_A);
        break;


        //if(opcode bleh

    // For SAX, both A and X are output to databus, LOW-bits are stronger than HIGH-bits, resulting in a "forceful" AND operation.
    // For LAX, the same value is written to both A and X.

    // Combined ALU-Opcodes
    // Opcode high-bits, flags, commands:

    // 00+yy        nzc---  SLO op       ASL+ORA   op=op SHL 1 // A=A OR op
    // 20+yy        nzc---  RLA op       ROL+AND   op=op RCL 1 // A=A AND op
    // 40+yy        nzc---  SRE op       LSR+EOR   op=op SHR 1 // A=A XOR op
    // 60+yy        nzc--v  RRA op       ROR+ADC   op=op RCR 1 // A=A ADC op
    // C0+yy        nzc---  DCP op       DEC+CMP   op=op-1     // A-op
    // E0+yy        nzc--v  ISC op       INC+SBC   op=op+1     // A=A-op cy?

    // Opcode low-bits, clock cycles, operands:

    // 07+xx nn        5    nn       [nn]
    // 17+xx nn        6    nn,X     [nn+X]
    // 03+xx nn        8    (nn,X)   [WORD[nn+X]]
    // 13+xx nn        8    (nn),Y   [WORD[nn]+Y]
    // 0F+xx nn nn     6    nnnn     [nnnn]
    // 1F+xx nn nn     7    nnnn,X   [nnnn+X]
    // 1B+xx nn nn     7    nnnn,Y   [nnnn+Y]


    // Other Illegal Opcodes

    // <  0B nn     nzc---  2  ANC #nn          AND+ASL  A=A AND nn
    // <  2B nn     nzc---  2  ANC #nn          AND+ROL  A=A AND nn
    // <  4B nn     nzc---  2  ALR #nn          AND+LSR  A=(A AND nn)*2  MUL2???
    // <  6B nn     nzc--v  2  ARR #nn          AND+ROR  A=(A AND nn)/2
    // <  8B nn     nz----  2  XAA #nn    ((2)) TXA+AND  A=X AND nn
    // <  AB nn     nz----  2  LAX #nn    ((2)) LDA+TAX  A,X=nn
    // <  CB nn     nzc---  2  AXS #nn          CMP+DEX  X=A AND X -nn  cy?
    // <  EB nn     nzc--v  2  SBC #nn          SBC+NOP  A=A-nn         cy?
    // <  93 nn     ------  6  AHX (nn),Y ((1))          [WORD[nn]+Y] = A AND X AND H

    case 0x9C: // <  9C nn nn  ------  5  SHY nnnn,X ((1))          [nnnn+X] = Y AND H
        ADDR_ABSOLUTE(); // nnnn,X
        a1 += X;
        new_T = Y & (a1 >> 8);
        STORE_T();
        cycles = 5; //  
        break;
    case 0x9E: // <  9E nn nn  ------  5  SHX nnnn,Y ((1))          [nnnn+Y] = X AND H
        ADDR_ABSOLUTE(); // nnnn,X
        a1 += Y;
        new_T = X & (a1 >> 8);
        STORE_T();
        cycles = 5; //  
        break;
    case 0x9F: // <  9F nn nn  ------  5  AHX nnnn,Y ((1))          [nnnn+Y] = A AND X AND H
        ADDR_ABSOLUTE(); // nnnn,X
        a1 += Y;
        new_T = A & X & (a1 >> 8);
        STORE_T();
        cycles = 5; //  
        break;

        // <  9B nn nn  ------  5  TAS nnnn,Y ((1)) STA+TXS  S=A AND X  // [nnnn+Y]=S AND H
        // <  BB nn nn  nz----  4* LAS nnnn,Y       LDA+TSX  A,X,S = [nnnn+Y] AND S


        // NUL/NOP and KIL/JAM/HLT

        // <  xx        ------  2   NOP        (xx=1A,3A,5A,7A,DA,FA)
    case 0x1A:
    case 0x3A:
    case 0x5A:
    case 0x7A:
    case 0xDA:
    case 0xFA:
        cycles = 2;
        break;

        // <  xx nn     ------  2   NOP #nn    (xx=80,82,89,C2,E2)
    case 0x80:
    case 0x82:
    case 0x89:
    case 0xC2:
    case 0xE2:
        Fetch();
        cycles = 2;
        break;

        // <  xx nn     ------  3   NOP nn     (xx=04,44,64)
    case 0x04:
    case 0x44:
    case 0x64:
        Fetch();
        cycles = 3;
        break;

        // <  xx nn     ------  4   NOP nn,X   (xx=14,34,54,74,D4,F4)
    case 0x14:
    case 0x34:
    case 0x54:
    case 0x74:
    case 0xD4:
    case 0xF4:
        Fetch();
        cycles = 4;
        break;

        // <  xx nn nn  ------  4   NOP nnnn   (xx=0C)
    case 0x0C:
        Fetch(); Fetch();
        cycles = 4;
        break;

        // <  xx nn nn  ------  4*  NOP nnnn,X (xx=1C,3C,5C,7C,DC,FC)
    case 0x1C:
    case 0x3C:
    case 0x5C:
    case 0x7C:
    case 0xDC:
    case 0xFC:
        Fetch(); Fetch();
        cycles = 4;
        break;

        // <  xx        ------  -   KIL        (xx=02,12,22,32,42,52,62,72,92,B2,D2,F2)
    case 0x02:
    case 0x12:
    case 0x22:
    case 0x32:
    case 0x42:
    case 0x52:
    case 0x62:
    case 0x72:
    case 0x92:
    case 0xB2:
    case 0xD2:
    case 0xF2:
        core->Stop();
        cycles = 1;
        MessageBoxEx(hMainWnd, _T("KIL-type instruction found. Execution halted."), _T("ALERT"), MB_OK, 0);
        break;
    default:

        //else
    {
        printf("Unimplemented opcode: %02x\n", opcode);
    }
    break;

    }

    A = (u8)new_A;
    X = (u8)new_X;
    Y = (u8)new_Y;

    if (opcode != 0xEA)
    {
        if (ShowDebug >= 2) printf("\t\t\tOP %02x: PC=%04x; A=%02x; X=%02x; Y=%02x; P=%02x(Z=%d/C=%d/N=%d/V=%d)\n", opcode, PC, A, X, Y, P, FLAG_Z, FLAG_C, FLAG_N, FLAG_V);
    }

    this->totalCycles += cycles * 3;
    return cycles;
}

struct OpInfo {

    int opcode;
    int bytes;
    int cycles;
    int addrmode;
    char* opname;
    char* flags;
    char* desc;

} opinfo[] = {

{ 0x00, 0, 0x0000, 0, "x00", "------ ", "Unknown instruction info" },
{ 0x01, 2, 0x0006, 7, "ORA", "nz---- ", "OR (Indirect,X)    A=A OR [[nn+X]]" },
{ 0x00, 0, 0x0000, 0, "x02", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x03", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x04", "------ ", "Unknown instruction info" },
{ 0x05, 2, 0x0003, 2, "ORA", "nz---- ", "OR Zero Page       A=A OR [nn]" },
{ 0x06, 2, 0x0005, 2, "ASL", "nzc--- ", "Shift Left Zero Page     SHL [nn]" },
{ 0x00, 0, 0x0000, 0, "x07", "------ ", "Unknown instruction info" },
{ 0x08, 1, 0x0003, 0, "PHP", "------ ", "Push processor status on stack   [S]=P" },
{ 0x09, 2, 0x0002, 1, "ORA", "nz---- ", "OR Immediate       A=A OR nn" },
{ 0x0A, 1, 0x0002, 0, "ASL", "nzc--- ", "Shift Left Accumulator   SHL A" },
{ 0x0B, 2, 0x0002, 1, "ANC", "nzc--- ", "     AND+ASL  A=A AND nn" },
{ 0x00, 0, 0x0000, 0, "x0C", "------ ", "Unknown instruction info" },
{ 0x0D, 3, 0x0004, 4, "ORA", "nz---- ", "OR Absolute        A=A OR [nnnn]" },
{ 0x0E, 3, 0x0006, 4, "ASL", "nzc--- ", "Shift Left Absolute      SHL [nnnn]" },
{ 0x00, 0, 0x0000, 0, "x0F", "------ ", "Unknown instruction info" },
{ 0x10, 1, 0xC002, 9, "BPL", "------ ", "Branch on result plus     if N=0 PC=PC+/-nn" },
{ 0x11, 2, 0x8005, 8, "ORA", "nz---- ", "OR (Indirect),Y    A=A OR [[nn]+Y]" },
{ 0x00, 0, 0x0000, 0, "x12", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x13", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x14", "------ ", "Unknown instruction info" },
{ 0x15, 2, 0x0004, 3, "ORA", "nz---- ", "OR Zero Page,X     A=A OR [nn+X]" },
{ 0x16, 2, 0x0006, 3, "ASL", "nzc--- ", "Shift Left Zero Page,X   SHL [nn+X]" },
{ 0x00, 0, 0x0000, 0, "x17", "------ ", "Unknown instruction info" },
{ 0x18, 1, 0x0002, 0, "CLC", "--0--- ", "Clear carry flag            C=0" },
{ 0x19, 3, 0x8004, 6, "ORA", "nz---- ", "OR Absolute,Y      A=A OR [nnnn+Y]" },
{ 0x00, 0, 0x0000, 0, "x1A", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x1B", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x1C", "------ ", "Unknown instruction info" },
{ 0x1D, 3, 0x8004, 5, "ORA", "nz---- ", "OR Absolute,X      A=A OR [nnnn+X]" },
{ 0x1E, 3, 0x0007, 5, "ASL", "nzc--- ", "Shift Left Absolute,X    SHL [nnnn+X]" },
{ 0x00, 0, 0x0000, 0, "x1F", "------ ", "Unknown instruction info" },
{ 0x20, 3, 0x0006, 4, "JSR", "------ ", "Jump and Save Return Addr. [S]=PC+2,PC=nnnn" },
{ 0x21, 2, 0x0006, 7, "AND", "nz---- ", "AND (Indirect,X)   A=A AND [[nn+X]]" },
{ 0x00, 0, 0x0000, 0, "x22", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x23", "------ ", "Unknown instruction info" },
{ 0x24, 2, 0x0003, 2, "BIT", "xz---x ", "Bit Test   A AND [nn], N=[nn].7, V=[nn].6" },
{ 0x25, 2, 0x0003, 2, "AND", "nz---- ", "AND Zero Page      A=A AND [nn]" },
{ 0x26, 2, 0x0005, 2, "ROL", "nzc--- ", "Rotate Left Zero Page    RCL [nn]" },
{ 0x00, 0, 0x0000, 0, "x27", "------ ", "Unknown instruction info" },
{ 0x28, 1, 0x0004, 0, "PLP", "nzcidv ", "Pull processor status from stack P=[S]" },
{ 0x29, 2, 0x0002, 1, "AND", "nz---- ", "AND Immediate      A=A AND nn" },
{ 0x2A, 1, 0x0002, 0, "ROL", "nzc--- ", "Rotate Left Accumulator  RCL A" },
{ 0x2B, 2, 0x0002, 1, "ANC", "nzc--- ", "     AND+ROL  A=A AND nn" },
{ 0x2C, 3, 0x0004, 4, "BIT", "xz---x ", "Bit Test   A AND [..], N=[..].7, V=[..].6" },
{ 0x2D, 3, 0x0004, 4, "AND", "nz---- ", "AND Absolute       A=A AND [nnnn]" },
{ 0x2E, 3, 0x0006, 4, "ROL", "nzc--- ", "Rotate Left Absolute     RCL [nnnn]" },
{ 0x00, 0, 0x0000, 0, "x2F", "------ ", "Unknown instruction info" },
{ 0x30, 1, 0xC002, 9, "BMI", "------ ", "Branch on result minus    if N=1 PC=PC+/-nn" },
{ 0x31, 2, 0x8005, 8, "AND", "nz---- ", "AND (Indirect),Y   A=A AND [[nn]+Y]" },
{ 0x00, 0, 0x0000, 0, "x32", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x33", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x34", "------ ", "Unknown instruction info" },
{ 0x35, 2, 0x0004, 3, "AND", "nz---- ", "AND Zero Page,X    A=A AND [nn+X]" },
{ 0x36, 2, 0x0006, 3, "ROL", "nzc--- ", "Rotate Left Zero Page,X  RCL [nn+X]" },
{ 0x00, 0, 0x0000, 0, "x37", "------ ", "Unknown instruction info" },
{ 0x38, 1, 0x0002, 0, "SEC", "--1--- ", "Set carry flag              C=1" },
{ 0x39, 3, 0x8004, 6, "AND", "nz---- ", "AND Absolute,Y     A=A AND [nnnn+Y]" },
{ 0x00, 0, 0x0000, 0, "x3A", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x3B", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x3C", "------ ", "Unknown instruction info" },
{ 0x3D, 3, 0x8004, 5, "AND", "nz---- ", "AND Absolute,X     A=A AND [nnnn+X]" },
{ 0x3E, 3, 0x0007, 5, "ROL", "nzc--- ", "Rotate Left Absolute,X   RCL [nnnn+X]" },
{ 0x00, 0, 0x0000, 0, "x3F", "------ ", "Unknown instruction info" },
{ 0x40, 1, 0x0006, 0, "RTI", "nzcidv ", "Return from BRK/IRQ/NMI    P=[S], PC=[S]" },
{ 0x41, 2, 0x0006, 7, "EOR", "nz---- ", "XOR (Indirect,X)   A=A XOR [[nn+X]]" },
{ 0x00, 0, 0x0000, 0, "x42", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x43", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x44", "------ ", "Unknown instruction info" },
{ 0x45, 2, 0x0003, 2, "EOR", "nz---- ", "XOR Zero Page      A=A XOR [nn]" },
{ 0x46, 2, 0x0005, 2, "LSR", "0zc--- ", "Shift Right Zero Page    SHR [nn]" },
{ 0x00, 0, 0x0000, 0, "x47", "------ ", "Unknown instruction info" },
{ 0x48, 1, 0x0003, 0, "PHA", "------ ", "Push accumulator on stack        [S]=A" },
{ 0x49, 2, 0x0002, 1, "EOR", "nz---- ", "XOR Immediate      A=A XOR nn" },
{ 0x4A, 1, 0x0002, 0, "LSR", "0zc--- ", "Shift Right Accumulator  SHR A" },
{ 0x4B, 2, 0x0002, 1, "ALR", "nzc--- ", "     AND+LSR  A=(A AND nn)*2  MUL2???" },
{ 0x4C, 3, 0x0003, 4, "JMP", "------ ", "Jump Absolute              PC=nnnn" },
{ 0x4D, 3, 0x0004, 4, "EOR", "nz---- ", "XOR Absolute       A=A XOR [nnnn]" },
{ 0x4E, 3, 0x0006, 4, "LSR", "0zc--- ", "Shift Right Absolute     SHR [nnnn]" },
{ 0x00, 0, 0x0000, 0, "x4F", "------ ", "Unknown instruction info" },
{ 0x50, 1, 0xC002, 9, "BVC", "------ ", "Branch on overflow clear  if V=0 PC=PC+/-nn" },
{ 0x51, 2, 0x8005, 8, "EOR", "nz---- ", "XOR (Indirect),Y   A=A XOR [[nn]+Y]" },
{ 0x00, 0, 0x0000, 0, "x52", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x53", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x54", "------ ", "Unknown instruction info" },
{ 0x55, 2, 0x0004, 3, "EOR", "nz---- ", "XOR Zero Page,X    A=A XOR [nn+X]" },
{ 0x56, 2, 0x0006, 3, "LSR", "0zc--- ", "Shift Right Zero Page,X  SHR [nn+X]" },
{ 0x00, 0, 0x0000, 0, "x57", "------ ", "Unknown instruction info" },
{ 0x58, 1, 0x0002, 0, "CLI", "---0-- ", "Clear interrupt disable bit I=0" },
{ 0x59, 3, 0x8004, 6, "EOR", "nz---- ", "XOR Absolute,Y     A=A XOR [nnnn+Y]" },
{ 0x00, 0, 0x0000, 0, "x5A", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x5B", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x5C", "------ ", "Unknown instruction info" },
{ 0x5D, 3, 0x8004, 5, "EOR", "nz---- ", "XOR Absolute,X     A=A XOR [nnnn+X]" },
{ 0x5E, 3, 0x0007, 5, "LSR", "0zc--- ", "Shift Right Absolute,X   SHR [nnnn+X]" },
{ 0x00, 0, 0x0000, 0, "x5F", "------ ", "Unknown instruction info" },
{ 0x60, 1, 0x0006, 0, "RTS", "------ ", "Return from Subroutine     PC=[S]+1" },
{ 0x61, 2, 0x0006, 7, "ADC", "nzc--v ", "Add (Indirect,X)        A=A+C+[[nn+X]]" },
{ 0x00, 0, 0x0000, 0, "x62", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x63", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x64", "------ ", "Unknown instruction info" },
{ 0x65, 2, 0x0003, 2, "ADC", "nzc--v ", "Add Zero Page           A=A+C+[nn]" },
{ 0x66, 2, 0x0005, 2, "ROR", "nzc--- ", "Rotate Right Zero Page   RCR [nn]" },
{ 0x00, 0, 0x0000, 0, "x67", "------ ", "Unknown instruction info" },
{ 0x68, 1, 0x0004, 0, "PLA", "nz---- ", "Pull accumulator from stack      A=[S]" },
{ 0x69, 2, 0x0002, 1, "ADC", "nzc--v ", "Add Immediate           A=A+C+nn" },
{ 0x6A, 1, 0x0002, 0, "ROR", "nzc--- ", "Rotate Right Accumulator RCR A" },
{ 0x6B, 2, 0x0002, 1, "ARR", "nzc--v ", "     AND+ROR  A=(A AND nn)/2" },
{ 0x6C, 3, 0x0005, 0, "JMP", "------ ", "Jump Indirect              PC=WORD[nnnn]" },
{ 0x6D, 3, 0x0004, 4, "ADC", "nzc--v ", "Add Absolute            A=A+C+[nnnn]" },
{ 0x6E, 3, 0x0006, 4, "ROR", "nzc--- ", "Rotate Right Absolute    RCR [nnnn]" },
{ 0x00, 0, 0x0000, 0, "x6F", "------ ", "Unknown instruction info" },
{ 0x70, 1, 0xC002, 9, "BVS", "------ ", "Branch on overflow set    if V=1 PC=PC+/-nn" },
{ 0x71, 2, 0x8005, 8, "ADC", "nzc--v ", "Add (Indirect),Y        A=A+C+[[nn]+Y]" },
{ 0x00, 0, 0x0000, 0, "x72", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x73", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x74", "------ ", "Unknown instruction info" },
{ 0x75, 2, 0x0004, 3, "ADC", "nzc--v ", "Add Zero Page,X         A=A+C+[nn+X]" },
{ 0x76, 2, 0x0006, 3, "ROR", "nzc--- ", "Rotate Right Zero Page,X RCR [nn+X]" },
{ 0x00, 0, 0x0000, 0, "x77", "------ ", "Unknown instruction info" },
{ 0x78, 1, 0x0002, 0, "SEI", "---1-- ", "Set interrupt disable bit   I=1" },
{ 0x79, 3, 0x8004, 6, "ADC", "nzc--v ", "Add Absolute,Y          A=A+C+[nnnn+Y]" },
{ 0x00, 0, 0x0000, 0, "x7A", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x7B", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x7C", "------ ", "Unknown instruction info" },
{ 0x7D, 3, 0x8004, 5, "ADC", "nzc--v ", "Add Absolute,X          A=A+C+[nnnn+X]" },
{ 0x7E, 3, 0x0007, 5, "ROR", "nzc--- ", "Rotate Right Absolute,X  RCR [nnnn+X]" },
{ 0x00, 0, 0x0000, 0, "x7F", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "x80", "------ ", "Unknown instruction info" },
{ 0x81, 2, 0x0006, 7, "STA", "------ ", "Store A in (Indirect,X)  [[nn+x]]=A" },
{ 0x00, 0, 0x0000, 0, "x82", "------ ", "Unknown instruction info" },
{ 0x83, 2, 0x0006, 7, "SAX", "------ ", "STA+STX  [WORD[nn+X]]=A AND X" },
{ 0x84, 2, 0x0003, 2, "STY", "------ ", "Store Y in Zero Page     [nn]=Y" },
{ 0x85, 2, 0x0003, 2, "STA", "------ ", "Store A in Zero Page     [nn]=A" },
{ 0x86, 2, 0x0003, 2, "STX", "------ ", "Store X in Zero Page     [nn]=X" },
{ 0x87, 2, 0x0003, 2, "SAX", "------ ", "STA+STX  [nn]=A AND X" },
{ 0x88, 1, 0x0002, 0, "DEY", "nz---- ", "Decrement Y            Y=Y-1" },
{ 0x00, 0, 0x0000, 0, "x89", "------ ", "Unknown instruction info" },
{ 0x8A, 1, 0x0002, 0, "TXA", "nz---- ", "Transfer X to Accumulator    A=X" },
{ 0x8B, 2, 0x0002, 1, "XAA", "nz---- ", "(2)) TXA+AND  A=X AND nn" },
{ 0x8C, 3, 0x0004, 4, "STY", "------ ", "Store Y in Absolute      [nnnn]=Y" },
{ 0x8D, 3, 0x0004, 4, "STA", "------ ", "Store A in Absolute      [nnnn]=A" },
{ 0x8E, 3, 0x0004, 4, "STX", "------ ", "Store X in Absolute      [nnnn]=X" },
{ 0x8F, 3, 0x0004, 4, "SAX", "------ ", "STA+STX  [nnnn]=A AND X" },
{ 0x90, 1, 0xC002, 9, "BCC", "------ ", "Branch on carry clear     if C=0 PC=PC+/-nn" },
{ 0x91, 2, 0x0006, 8, "STA", "------ ", "Store A in (Indirect),Y  [[nn]+y]=A" },
{ 0x00, 0, 0x0000, 0, "x92", "------ ", "Unknown instruction info" },
{ 0x93, 2, 0x0006, 8, "AHX", "------ ", "(1))          [WORD[nn]+Y] = A AND X AND H" },
{ 0x94, 2, 0x0004, 3, "STY", "------ ", "Store Y in Zero Page,X   [nn+X]=Y" },
{ 0x95, 2, 0x0004, 3, "STA", "------ ", "Store A in Zero Page,X   [nn+X]=A" },
{ 0x96, 2, 0x0004, 0, "STX", "------ ", "Store X in Zero Page,Y   [nn+Y]=X" },
{ 0x97, 2, 0x0004, 0, "SAX", "------ ", "STA+STX  [nn+Y]=A AND X" },
{ 0x98, 1, 0x0002, 0, "TYA", "nz---- ", "Transfer Y to Accumulator    A=Y" },
{ 0x99, 3, 0x0005, 6, "STA", "------ ", "Store A in Absolute,Y    [nnnn+Y]=A" },
{ 0x9A, 1, 0x0002, 0, "TXS", "------ ", "Transfer X to Stack pointer  S=X" },
{ 0x9B, 3, 0x0005, 6, "TAS", "------ ", "(1)) STA+TXS  S=A AND X  // [nnnn+Y]=S AND H" },
{ 0x9C, 3, 0x0005, 5, "SHY", "------ ", "(1))          [nnnn+X] = Y AND H" },
{ 0x9D, 3, 0x0005, 5, "STA", "------ ", "Store A in Absolute,X    [nnnn+X]=A" },
{ 0x9E, 3, 0x0005, 6, "SHX", "------ ", "(1))          [nnnn+Y] = X AND H" },
{ 0x9F, 3, 0x0005, 6, "AHX", "------ ", "(1))          [nnnn+Y] = A AND X AND H" },
{ 0xA0, 2, 0x0002, 1, "LDY", "nz---- ", "Load Y with Immediate     Y=nn" },
{ 0xA1, 2, 0x0006, 7, "LDA", "nz---- ", "Load A with (Indirect,X)  A=[WORD[nn+X]]" },
{ 0xA2, 2, 0x0002, 1, "LDX", "nz---- ", "Load X with Immediate     X=nn" },
{ 0xA3, 2, 0x0006, 7, "LAX", "nz---- ", "LDA+LDX  A,X=[WORD[nn+X]]" },
{ 0xA4, 2, 0x0003, 2, "LDY", "nz---- ", "Load Y with Zero Page     Y=[nn]" },
{ 0xA5, 2, 0x0003, 2, "LDA", "nz---- ", "Load A with Zero Page     A=[nn]" },
{ 0xA6, 2, 0x0003, 2, "LDX", "nz---- ", "Load X with Zero Page     X=[nn]" },
{ 0xA7, 2, 0x0003, 2, "LAX", "nz---- ", "LDA+LDX  A,X=[nn]" },
{ 0xA8, 1, 0x0002, 0, "TAY", "nz---- ", "Transfer Accumulator to Y    Y=A" },
{ 0xA9, 2, 0x0002, 1, "LDA", "nz---- ", "Load A with Immediate     A=nn" },
{ 0xAA, 1, 0x0002, 0, "TAX", "nz---- ", "Transfer Accumulator to X    X=A" },
{ 0xAB, 2, 0x0002, 1, "LAX", "nz---- ", "(2)) LDA+TAX  A,X=nn" },
{ 0xAC, 3, 0x0004, 4, "LDY", "nz---- ", "Load Y with Absolute      Y=[nnnn]" },
{ 0xAD, 3, 0x0004, 4, "LDA", "nz---- ", "Load A with Absolute      A=[nnnn]" },
{ 0xAE, 3, 0x0004, 4, "LDX", "nz---- ", "Load X with Absolute      X=[nnnn]" },
{ 0xAF, 3, 0x0004, 4, "LAX", "nz---- ", "LDA+LDX  A,X=[nnnn]" },
{ 0xB0, 1, 0xC002, 9, "BCS", "------ ", "Branch on carry set       if C=1 PC=PC+/-nn" },
{ 0xB1, 2, 0x8005, 8, "LDA", "nz---- ", "Load A with (Indirect),Y  A=[WORD[nn]+Y]" },
{ 0x00, 0, 0x0000, 0, "xB2", "------ ", "Unknown instruction info" },
{ 0xB3, 2, 0x8005, 8, "LAX", "nz---- ", "LDA+LDX  A,X=[WORD[nn]+Y]" },
{ 0xB4, 2, 0x0004, 3, "LDY", "nz---- ", "Load Y with Zero Page,X   Y=[nn+X]" },
{ 0xB5, 2, 0x0004, 3, "LDA", "nz---- ", "Load A with Zero Page,X   A=[nn+X]" },
{ 0xB6, 2, 0x0004, 0, "LDX", "nz---- ", "Load X with Zero Page,Y   X=[nn+Y]" },
{ 0xB7, 2, 0x0004, 0, "LAX", "nz---- ", "LDA+LDX  A,X=[nn+Y]" },
{ 0xB8, 1, 0x0002, 0, "CLV", "-----0 ", "Clear overflow flag         V=0" },
{ 0xB9, 3, 0x8004, 6, "LDA", "nz---- ", "Load A with Absolute,Y    A=[nnnn+Y]" },
{ 0xBA, 1, 0x0002, 0, "TSX", "nz---- ", "Transfer Stack pointer to X  X=S" },
{ 0xBB, 3, 0x8004, 6, "LAS", "nz---- ", "     LDA+TSX  A,X,S = [nnnn+Y] AND S" },
{ 0xBC, 3, 0x8004, 5, "LDY", "nz---- ", "Load Y with Absolute,X    Y=[nnnn+X]" },
{ 0xBD, 3, 0x8004, 5, "LDA", "nz---- ", "Load A with Absolute,X    A=[nnnn+X]" },
{ 0xBE, 3, 0x8004, 6, "LDX", "nz---- ", "Load X with Absolute,Y    X=[nnnn+Y]" },
{ 0xBF, 3, 0x8004, 5, "LAX", "nz---- ", "LDA+LDX  A,X=[nnnn+X]" },
{ 0xC0, 2, 0x0002, 1, "CPY", "nzc--- ", "Compare Y with Immediate     Y-nn" },
{ 0xC1, 2, 0x0006, 7, "CMP", "nzc--- ", "Compare A with (Indirect,X)  A-[[nn+X]]" },
{ 0x00, 0, 0x0000, 0, "xC2", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "xC3", "------ ", "Unknown instruction info" },
{ 0xC4, 2, 0x0003, 2, "CPY", "nzc--- ", "Compare Y with Zero Page     Y-[nn]" },
{ 0xC5, 2, 0x0003, 2, "CMP", "nzc--- ", "Compare A with Zero Page     A-[nn]" },
{ 0xC6, 2, 0x0005, 2, "DEC", "nz---- ", "Decrement Zero Page    [nn]=[nn]-1" },
{ 0x00, 0, 0x0000, 0, "xC7", "------ ", "Unknown instruction info" },
{ 0xC8, 1, 0x0002, 0, "INY", "nz---- ", "Increment Y            Y=Y+1" },
{ 0xC9, 2, 0x0002, 1, "CMP", "nzc--- ", "Compare A with Immediate     A-nn" },
{ 0xCA, 1, 0x0002, 0, "DEX", "nz---- ", "Decrement X            X=X-1" },
{ 0xCB, 2, 0x0002, 1, "AXS", "nzc--- ", "     CMP+DEX  X=A AND X -nn  cy?" },
{ 0xCC, 3, 0x0004, 4, "CPY", "nzc--- ", "Compare Y with Absolute      Y-[nnnn]" },
{ 0xCD, 3, 0x0004, 4, "CMP", "nzc--- ", "Compare A with Absolute      A-[nnnn]" },
{ 0xCE, 3, 0x0006, 4, "DEC", "nz---- ", "Decrement Absolute     [nnnn]=[nnnn]-1" },
{ 0x00, 0, 0x0000, 0, "xCF", "------ ", "Unknown instruction info" },
{ 0xD0, 1, 0xC002, 9, "BNE", "------ ", "Branch on result not zero if Z=0 PC=PC+/-nn" },
{ 0xD1, 2, 0x8005, 8, "CMP", "nzc--- ", "Compare A with (Indirect),Y  A-[[nn]+Y]" },
{ 0x00, 0, 0x0000, 0, "xD2", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "xD3", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "xD4", "------ ", "Unknown instruction info" },
{ 0xD5, 2, 0x0004, 3, "CMP", "nzc--- ", "Compare A with Zero Page,X   A-[nn+X]" },
{ 0xD6, 2, 0x0006, 3, "DEC", "nz---- ", "Decrement Zero Page,X  [nn+X]=[nn+X]-1" },
{ 0x00, 0, 0x0000, 0, "xD7", "------ ", "Unknown instruction info" },
{ 0xD8, 1, 0x0002, 0, "CLD", "----0- ", "Clear decimal mode          D=0" },
{ 0xD9, 3, 0x8004, 6, "CMP", "nzc--- ", "Compare A with Absolute,Y    A-[nnnn+Y]" },
{ 0x00, 0, 0x0000, 0, "xDA", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "xDB", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "xDC", "------ ", "Unknown instruction info" },
{ 0xDD, 3, 0x8004, 5, "CMP", "nzc--- ", "Compare A with Absolute,X    A-[nnnn+X]" },
{ 0xDE, 3, 0x0007, 5, "DEC", "nz---- ", "Decrement Absolute,X   [nnnn+X]=[nnnn+X]-1" },
{ 0x00, 0, 0x0000, 0, "xDF", "------ ", "Unknown instruction info" },
{ 0xE0, 2, 0x0002, 1, "CPX", "nzc--- ", "Compare X with Immediate     X-nn" },
{ 0xE1, 2, 0x0006, 7, "SBC", "nzc--v ", "Subtract (Indirect,X)   A=A+C-1-[[nn+X]]" },
{ 0x00, 0, 0x0000, 0, "xE2", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "xE3", "------ ", "Unknown instruction info" },
{ 0xE4, 2, 0x0003, 2, "CPX", "nzc--- ", "Compare X with Zero Page     X-[nn]" },
{ 0xE5, 2, 0x0003, 2, "SBC", "nzc--v ", "Subtract Zero Page      A=A+C-1-[nn]" },
{ 0xE6, 2, 0x0005, 2, "INC", "nz---- ", "Increment Zero Page    [nn]=[nn]+1" },
{ 0x00, 0, 0x0000, 0, "xE7", "------ ", "Unknown instruction info" },
{ 0xE8, 1, 0x0002, 0, "INX", "nz---- ", "Increment X            X=X+1" },
{ 0xE9, 2, 0x0002, 1, "SBC", "nzc--v ", "Subtract Immediate      A=A+C-1-nn" },
{ 0xEA, 0, 0x0000, 0, "NOP", "------ ", "NOP" },
{ 0xEB, 2, 0x0002, 1, "SBC", "nzc--v ", "     SBC+NOP  A=A-nn         cy?" },
{ 0xEC, 3, 0x0004, 4, "CPX", "nzc--- ", "Compare X with Absolute      X-[nnnn]" },
{ 0xED, 3, 0x0004, 4, "SBC", "nzc--v ", "Subtract Absolute       A=A+C-1-[nnnn]" },
{ 0xEE, 3, 0x0006, 4, "INC", "nz---- ", "Increment Absolute     [nnnn]=[nnnn]+1" },
{ 0x00, 0, 0x0000, 0, "xEF", "------ ", "Unknown instruction info" },
{ 0xF0, 1, 0xC002, 9, "BEQ", "------ ", "Branch on result zero     if Z=1 PC=PC+/-nn" },
{ 0xF1, 2, 0x8005, 8, "SBC", "nzc--v ", "Subtract (Indirect),Y   A=A+C-1-[[nn]+Y]" },
{ 0x00, 0, 0x0000, 0, "xF2", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "xF3", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "xF4", "------ ", "Unknown instruction info" },
{ 0xF5, 2, 0x0004, 3, "SBC", "nzc--v ", "Subtract Zero Page,X    A=A+C-1-[nn+X]" },
{ 0xF6, 2, 0x0006, 3, "INC", "nz---- ", "Increment Zero Page,X  [nn+X]=[nn+X]+1" },
{ 0x00, 0, 0x0000, 0, "xF7", "------ ", "Unknown instruction info" },
{ 0xF8, 1, 0x0002, 0, "SED", "----1- ", "Set decimal mode            D=1" },
{ 0xF9, 3, 0x8004, 6, "SBC", "nzc--v ", "Subtract Absolute,Y     A=A+C-1-[nnnn+Y]" },
{ 0x00, 0, 0x0000, 0, "xFA", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "xFB", "------ ", "Unknown instruction info" },
{ 0x00, 0, 0x0000, 0, "xFC", "------ ", "Unknown instruction info" },
{ 0xFD, 3, 0x8004, 5, "SBC", "nzc--v ", "Subtract Absolute,X     A=A+C-1-[nnnn+X]" },
{ 0xFE, 3, 0x0007, 5, "INC", "nz---- ", "Increment Absolute,X   [nnnn+X]=[nnnn+X]+1" },
{ 0x00, 0, 0x0000, 0, "xFF", "------ ", "Unknown instruction info" },

};

void Cpu::PrintInstructionInfo1(FILE* f, u16 addr, u8 op, u8 byte2, u8 byte3)
{
    OpInfo opi = opinfo[op];

    int c = totalCycles % 341;

    switch (opi.addrmode)
    {
    case 0: fprintf(f, "%04X  %02X        %s                             A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d\n", addr, op, opi.opname, A, X, Y, P, S, c); break;
    case 1: fprintf(f, "%04X  %02X %02X     %s #$%02X                        A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d\n", addr, op, byte2, opi.opname, byte2, A, X, Y, P, S, c); break;
    case 2: fprintf(f, "%04X  %02X %02X     %s $%02X = %02X                    A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d\n", addr, op, byte2, opi.opname, byte2, memory->Read(byte2), A, X, Y, P, S, c); break;
    case 3: fprintf(f, "%04X  %02X %02X     %s $%02X,%02X                      A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d\n", addr, op, byte2, opi.opname, byte2, X, A, X, Y, P, S, c); break;
    case 4: fprintf(f, "%04X  %02X %02X %02X  %s $%02X%02X                       A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d\n", addr, op, byte2, byte3, opi.opname, byte3, byte2, A, X, Y, P, S, c); break;
    case 5: fprintf(f, "%04X  %02X %02X %02X  %s $%02X%02X,%02X                    A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d\n", addr, op, byte2, byte3, opi.opname, byte3, byte2, X, A, X, Y, P, S, c); break;
    case 6: fprintf(f, "%04X  %02X %02X %02X  %s $%02X%02X,%02X                    A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d\n", addr, op, byte2, byte3, opi.opname, byte3, byte2, Y, A, X, Y, P, S, c); break;
    case 7: fprintf(f, "%04X  %02X %02X     %s ($%02X,%02X)                    A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d\n", addr, op, byte2, opi.opname, byte2, X, A, X, Y, P, S, c); break;
    case 8: fprintf(f, "%04X  %02X %02X     %s ($%02X),%02X                    A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d\n", addr, op, byte2, opi.opname, byte2, Y, A, X, Y, P, S, c); break;
    case 9: fprintf(f, "%04X  %02X %02X     %s $%04X                       A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d\n", addr, op, byte2, opi.opname, addr + 2 + byte2, A, X, Y, P, S, c); break;
    }
}

void PrintInstructionInfo2(u16 addr, u8 op, u8 byte2, u8 byte3)
{
    OpInfo opi = opinfo[op];

    switch (opi.addrmode)
    {
    case 0: printf("%04X  %02X = %s            %s\n", addr, op, opi.opname, opi.desc); break;
    case 1: printf("%04X  %02X = %s #%02X        %s\n", addr, op, opi.opname, byte2, opi.desc); break;
    case 2: printf("%04X  %02X = %s %02X         %s\n", addr, op, opi.opname, byte2, opi.desc); break;
    case 3: printf("%04X  %02X = %s %02X,X       %s\n", addr, op, opi.opname, byte2, opi.desc); break;
    case 4: printf("%04X  %02X = %s %02X%02X       %s\n", addr, op, opi.opname, byte3, byte2, opi.desc); break;
    case 5: printf("%04X  %02X = %s %02X%02X,X     %s\n", addr, op, opi.opname, byte3, byte2, opi.desc); break;
    case 6: printf("%04X  %02X = %s %02X%02X,Y     %s\n", addr, op, opi.opname, byte3, byte2, opi.desc); break;
    case 7: printf("%04X  %02X = %s (%02X,X)     %s\n", addr, op, opi.opname, byte2, opi.desc); break;
    case 8: printf("%04X  %02X = %s (%02X),Y     %s\n", addr, op, opi.opname, byte2, opi.desc); break;
    case 9: printf("%04X  %02X = %s %04X     %s\n", addr, op, opi.opname, addr + byte2, opi.desc); break;
    }
}