#include "StdAfx.h"
#include "Emu.h"

Pad::Pad(void)
{
}

Pad::~Pad(void)
{
}

void Pad::Init()
{
	bit  = 8;
	strobe = 0;
}

void Pad::Close()
{
}

void Pad::Fetch()
{
	if(strobe)
	{
		bit=0;
		u8 reg = 0;
		reg = (reg<<1) | core->GetKey(VK_RIGHT);
		reg = (reg<<1) | core->GetKey(VK_LEFT);
		reg = (reg<<1) | core->GetKey(VK_DOWN);
		reg = (reg<<1) | core->GetKey(VK_UP);
		reg = (reg<<1) | core->GetKey('B');
		reg = (reg<<1) | core->GetKey('D');
		reg = (reg<<1) | core->GetKey('X');
		reg = (reg<<1) | core->GetKey('Z');
		buffer[0] = reg;
		buffer[1] = 0;
	}
}

void Pad::Strobe(u8 value)
{
	strobe = value&1;
	Fetch();
}

u8   Pad::Poll (u8 which)
{
	Fetch();

	u8 cbit=0;
	if(bit>7)
		cbit=1;
	else
		cbit = (buffer[which]>>(bit++))&1;

	return cbit;
}
