#include "StdAfx.h"
#include "Emu.h"

Core::Core(void)
{
	isInit = false;
	running  = false;
	hadVSync = false;
}

Core::~Core(void)
{
}


void Core::Init(char *romFile)
{
	Close(); // make sure it's unloaded first

	if((mapper = LoadROM(romFile))==NULL)
		return;

	if((display = GetGDIDisplay())==NULL)
		return;

	memory = new Memory();
	cpu = new Cpu();
	ppu = new Ppu();
	apu = new Apu();
	pad = new Pad();

	memset(keyStates,0,sizeof(keyStates));

	display->Init(hMainWnd, false);
	mapper->Init();
	memory->Init();
	cpu->Init();
	ppu->Init(mapper->VMode());
	apu->Init(mapper->VMode(),48000);
	pad->Init();
	isInit=true;
}

void Core::Close()
{
	if(display) display->Close();
	if(mapper) { mapper->Close(); delete mapper; }
	if(memory) { memory->Close(); delete memory; }
	if(cpu) { cpu->Close(); delete cpu; }
	if(ppu) { ppu->Close(); delete ppu; }
	if(apu) { apu->Close(); delete apu; }
	if(pad) { pad->Close(); delete pad; }

	display=NULL;
	memory=NULL;
	mapper=NULL;
	cpu=NULL;
	ppu=NULL;
	apu=NULL;
	pad=NULL;
	isInit=false;
}


void Core::Start()
{
	if(isInit)
		running=true;
	else
	{
		MessageBoxEx(hMainWnd,_T("There's no ROM loaded."),_T("ERROR"),MB_OK,0);
	}
}

void Core::Reset()
{
	if(isInit)
	{
		memory->Reset();
		mapper->Reset();
		cpu->Reset();
		ppu->Reset();
		apu->Reset();
	}
}

void Core::Stop()
{
	running=false;
}

void Core::Emulate()
{
	if(!running)
	{
		SleepEx(100,TRUE);
		return;
	}

	int maxCycles = 10000;
	while(maxCycles>0)
	{
		int cycles = cpu->Step();
		ppu->Emulate(cycles);
		//apu->Emulate(cycles);

		maxCycles -= cycles;

		if(hadVSync)
		{
			hadVSync = false;
			break;
		}

		if(!running)
		{
			printf("Core stopped.");
			break;
		}
	}
}

void Core::DoVSync()
{
	display->Render();
	//display->PretendRender();

	// other things
}

void Core::SetKey(int which, u8 bit)
{
	keyStates[which]=bit;
}
u8   Core::GetKey(int which)
{
	return keyStates[which];
}
