#include <stdafx.h>
#define _WIN32_DCOM

#include <dsound.h>
#include <strsafe.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <conio.h>
#include <assert.h>

#pragma comment(lib,"dsound.lib")

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef   signed char  s8;
typedef   signed short s16;
typedef   signed int   s32;

#define OUTPUT_NULL		0
#define OUTPUT_DSOUND	1

const int OutputModule = OUTPUT_DSOUND;
const s32 CurBufferSize = 2048;

const s32 MaxBufferCount = 5;
const s32 CurBufferCount = MaxBufferCount;

const s32 LimitMode = 1;

s32 SampleRate;

#define pcmlog
extern FILE *wavelog;

class DSound
{
private:
#	define PI 3.14159265f

#	define BufferSize      (CurBufferSize<<1)
#	define BufferSizeBytes (BufferSize<<1)

#	define TBufferSize     (BufferSize*CurBufferCount)

#	define WRITE_RANGE 3

	s16* lbuffer;
	s16* tbuffer;

	u16 writecursor;

	FILE *voicelog;

	int channel;

	int read_cursor;
	int write_cursor;
#	define CBUFFER(x) (lbuffer + (x))

	bool dsound_running;
	HANDLE thread;
	DWORD tid;

#	define MAX_BUFFER_COUNT 3

	IDirectSound8* dsound;
	IDirectSoundBuffer8* buffer;
	IDirectSoundNotify8* buffer_notify;
	HANDLE buffer_events[MAX_BUFFER_COUNT];

	WAVEFORMATEX wfx;

	HANDLE waitEvent;
	bool coreIsWaiting;
	bool audioIsLooping;

#	define STRFY(x) #x

#	define verifyc(x) if(Verifyc(x,STRFY(x))) return -1;

	int __inline Verifyc(HRESULT hr,const char* fn)
	{
		if(FAILED(hr))
		{
			return -1;
		}
		return 0;
	}

	static DWORD CALLBACK RThread(DSound*obj)
	{
		return obj->Thread();
	}

	DWORD CALLBACK Thread()
	{
		while( dsound_running )
		{
			u32 rv = WaitForMultipleObjects(MAX_BUFFER_COUNT,buffer_events,FALSE,400);
	 
			LPVOID p1,p2;
			DWORD s1,s2;
	 
			for(int i=0;i<MAX_BUFFER_COUNT;i++)
			{
				if (rv==WAIT_OBJECT_0+i)
				{
					u32 poffset=BufferSizeBytes * i;

					verifyc(buffer->Lock(poffset,BufferSizeBytes,&p1,&s1,&p2,&s2,0));
					CopyMemory(p1,CBUFFER(read_cursor),BufferSizeBytes);
					ZeroMemory(CBUFFER(read_cursor),BufferSizeBytes);
					verifyc(buffer->Unlock(p1,s1,p2,s2));

					int usedspace1=(TBufferSize+write_cursor-read_cursor)%TBufferSize;

					read_cursor=(read_cursor+BufferSize)%TBufferSize;

					int usedspace2=(TBufferSize+write_cursor-read_cursor)%TBufferSize;

					if(usedspace2>usedspace1)
					{
						audioIsLooping=true;
					}
					if(usedspace2<(2*BufferSize))
					{
						audioIsLooping=false;
					}

					if(coreIsWaiting)
					{
						int usedspace=(TBufferSize+write_cursor-read_cursor)%TBufferSize;
						if(usedspace<=(WRITE_RANGE*BufferSize))
						{
							PulseEvent(waitEvent);
						}

						//if core is waiting, audio can NOT be looping :P
						audioIsLooping=false;
					}
				}
			}
		}
		return 0;
	}

public:
	s32 Init()
	{
		if(dsound_running)
			return 0;

		//
		// Initialize DSound
		//
		verifyc(DirectSoundCreate8(NULL,&dsound,NULL));
	 
		verifyc(dsound->SetCooperativeLevel(GetDesktopWindow(),DSSCL_PRIORITY));
		IDirectSoundBuffer* buffer_;
 		DSBUFFERDESC desc; 
	 
		// Set up WAV format structure. 
	 
		memset(&wfx, 0, sizeof(WAVEFORMATEX)); 
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.nSamplesPerSec = SampleRate;
		wfx.nChannels=2;
		wfx.wBitsPerSample = 16;
		wfx.nBlockAlign = 2*2;
		wfx.nAvgBytesPerSec = SampleRate * 2 * 2;
		wfx.cbSize=0;
	 
		// Set up DSBUFFERDESC structure. 
	 
		memset(&desc, 0, sizeof(DSBUFFERDESC)); 
		desc.dwSize = sizeof(DSBUFFERDESC); 
		desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY;// _CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY; 
		desc.dwBufferBytes = BufferSizeBytes * MAX_BUFFER_COUNT; 
		desc.lpwfxFormat = &wfx; 
	 
		desc.dwFlags |=DSBCAPS_LOCSOFTWARE;
		desc.dwFlags|=DSBCAPS_GLOBALFOCUS;
	 
		verifyc(dsound->CreateSoundBuffer(&desc,&buffer_,0));
		verifyc(buffer_->QueryInterface(IID_IDirectSoundBuffer8,(void**)&buffer));
		buffer_->Release();
	 
		verifyc(buffer->QueryInterface(IID_IDirectSoundNotify8,(void**)&buffer_notify));

		DSBPOSITIONNOTIFY not[MAX_BUFFER_COUNT];
	 
		for(int i=0;i<MAX_BUFFER_COUNT;i++)
		{
			buffer_events[i]=CreateEvent(NULL,FALSE,FALSE,NULL);
			not[i].dwOffset=(wfx.nBlockAlign*10 + BufferSizeBytes*(i+1))%desc.dwBufferBytes;
			not[i].hEventNotify=buffer_events[i];
		}
	 
		buffer_notify->SetNotificationPositions(MAX_BUFFER_COUNT,not);
	 
		LPVOID p1=0,p2=0;
		DWORD s1=0,s2=0;
	 
		verifyc(buffer->Lock(0,desc.dwBufferBytes,&p1,&s1,&p2,&s2,0));
		assert(p2==0);
		memset(p1,0,s1);
		verifyc(buffer->Unlock(p1,s1,p2,s2));
	 
		lbuffer=(s16*)malloc(BufferSizeBytes*MaxBufferCount);
		tbuffer=(s16*)malloc(BufferSizeBytes);
		if (!lbuffer || !tbuffer) {
			dsound->Release();
			if(lbuffer) free(lbuffer);
			if(tbuffer) free(tbuffer);
			return -1;
		}

		ZeroMemory(lbuffer,BufferSizeBytes*MaxBufferCount);

		read_cursor=0;
		write_cursor=0;
		writecursor=0;

		//Play the buffer !
		verifyc(buffer->Play(0,0,DSBPLAY_LOOPING));

		waitEvent=CreateEvent(NULL,FALSE,FALSE,NULL);
		coreIsWaiting=false;
		audioIsLooping=false;

		// Start Thread
		dsound_running=true;
			thread=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RThread,this,0,&tid);
		SetThreadPriority(thread,THREAD_PRIORITY_TIME_CRITICAL);

		return 0;
	}

	void Close()
	{
		if(!dsound_running)
			return;

		// Stop Thread
		dsound_running=false;
		WaitForSingleObject(thread,INFINITE);
		CloseHandle(thread);

		//
		// Clean up
		//
		buffer->Stop();
	 
		for(int i=0;i<MAX_BUFFER_COUNT;i++)
			CloseHandle(buffer_events[i]);
	 
		buffer_notify->Release();
		buffer->Release();
		dsound->Release();

		CloseHandle(waitEvent);

		free(tbuffer);
		free(lbuffer);
		tbuffer=NULL;
		lbuffer=NULL;
	}

	void AddBuffer(s16*pbuffer)
	{
		if((write_cursor+BufferSize)>TBufferSize)
		{
			int copy1=TBufferSize-write_cursor;
			int copy2= BufferSize-copy1;
			CopyMemory(CBUFFER(write_cursor),pbuffer,copy1<<1);
			CopyMemory(CBUFFER(0),pbuffer+copy1,copy2<<1);
		}
		else
		{
			CopyMemory(CBUFFER(write_cursor),pbuffer,BufferSizeBytes);
		}
		write_cursor=(write_cursor+BufferSize)%TBufferSize;

		if(LimitMode!=0)
		{
			int usedspace=(TBufferSize+write_cursor-read_cursor)%TBufferSize;
			if(usedspace>(WRITE_RANGE*BufferSize))
			{
				if(HIWORD(GetKeyState(VK_RCONTROL))!=0) //Run As Fast AS You Can
				{
					audioIsLooping=true;
					return;
				}

				coreIsWaiting=true;
				for(;;)
				{
					if(WaitForSingleObject(waitEvent,1000)!=WAIT_TIMEOUT)
					{
						break;
					}
				}
				coreIsWaiting=false;
			}
		}
	}

	s32  Write(s32 ValL, s32 ValR)
	{
		tbuffer[writecursor++]=ValL;
		tbuffer[writecursor++]=ValR;

		if (writecursor>=BufferSize) 
		{	
			AddBuffer(tbuffer);

			writecursor=0;
		}
		return 0;
	}

	s32  WriteSamples(s32 samples, s16* data)
	{
		while(samples>0)
		{
			Write(*data,*data);
			data++;
		}
		return 0;
	}

} DS;

s32 SndInit(int srate)
{
	SampleRate = srate;

	return DS.Init();
}
void SndClose()
{
	DS.Close();
}

s32 SndWrite(s32 ValL, s32 ValR)
{
	return DS.Write(ValL>>8,ValR>>8);
}

s32 SndWriteSamples(s32 samples, s16* data)
{
	return DS.WriteSamples(samples,data);
}
