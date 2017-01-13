#include "StdAfx.h"
#include "Emu.h"
#include "Mapper.h"

Mapper::~Mapper(void)
{
}

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
