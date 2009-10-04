#include "StdAfx.h"
#include "Emu.h"

Apu::Apu(void)
{
}

Apu::~Apu(void)
{
}

s32  SndInit(s32 sampleRate);						// Return Value: 0 if OK, -1 if FAILED
void SndClose();					// Return Value: none
s32  SndWrite(s32 ValL, s32 ValR);	// Return Value: 0 = SENT, -1 = IGNORED(buffer full)
s32  SndTest();
bool SndGetStats(u32 *written, u32 *played);

extern s32 SampleRate;

// duty cycles taken from wikipedia, to be used WITHOUT interpolation
const s32 Rcycle[4][8] = {
	{0xF,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
	{0xF,0xF,0x0,0x0,0x0,0x0,0x0,0x0},
	{0xF,0xF,0xF,0xF,0x0,0x0,0x0,0x0},
	{0xF,0xF,0xF,0xF,0xF,0xF,0x0,0x0},
};

const s32 TriangleCycle[32] = {
	0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,
	0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,
	0xF,0xE,0xD,0xC,0xB,0xA,0x9,0x8,
	0x7,0x6,0x5,0x4,0x3,0x2,0x1,0x0,
};

const s32 NoiseFreqs[16] = {
	0x002,0x004,0x008,0x010,0x020,0x030,0x040,0x050,
	0x065,0x07F,0x0BE,0x0FE,0x17D,0x1FC,0x3F9,0x7F2,
};

const s32 DMCFreqs[16] = {
	0xD60,0xBE0,0xAA0,0xA00,0x8F0,0x7F0,0x710,0x6B0,
	0x5F0,0x500,0x470,0x400,0x350,0x2A8,0x240,0x1B0,
};

const s32 LengthCounts[32] = {
	0x05,0x7f,
	0x0A,0x01,
	0x14,0x02,
	0x28,0x03,
	0x50,0x04,
	0x1E,0x05,
	0x07,0x06,
	0x0D,0x07,
	0x06,0x08,
	0x0C,0x09,
	0x18,0x0A,
	0x30,0x0B,
	0x60,0x0C,
	0x24,0x0D,
	0x08,0x0E,
	0x10,0x0F,
};

struct /*NESaudio*/
{
	// Ch0: Rectangle 1
	// Ch1: Rectangle 2
	// Ch2: Triangle
	// Ch3: Noise: Outputs randomly 0 or Volume

	struct /*ChannelInfo */
	{
		s32 Enabled;

		// waveform position
		s32 Position;

		s32 FreqCounter; 
		s32 SweepCounter;
		s32 LengthCounter;
		s32 DecayCounter;
		s32 LinearCounter;

		//F = 1.79MHz/(N+1)/16 for Rectangle channels << AAAH: that means they can do 111.875KHz!
		//F = 1.79MHz/(N+1)/32 for Triangle channel
		s32 Frequency;

		// Only on Rectangle Channels
		s32 SweepShift;
		s32 SweepDirection;
		s32 SweepRate;
		s32 SweepEnable;

		// Only on Rectangle and Noise (0,1,3)
		s32 Volume;
		s32 DecayEnable;
		s32 DecayRate;
		s32 DecayLoop;

		// The 7bit counter value is decremented once per frame (PAL=48Hz, or NTSC=60Hz) the counter and sound output are stopped when reaching a value of zero.
		s32 LengthCountEnable; //Disable stops decrementing but doesn't change the value!

		// Only on Rectangle channels (0 and 1)
		s32 Duty;

		// Noise State
		s32 NoiseReg;
		s32 ShortNoise;

	} Channels[4];

	s32 Channel5_Enabled;
	s32 Channel5_IrqEnable;
	s32 Channel5_Loop;
	s32 Channel5_Frequency;
	s32 Channel5_Delta;
	s32 Channel5_DMAStart;
	s32 Channel5_DMALength;
	s32 Channel5_FreqCounter; 

	s32 Channel5_DMAAddr;
	s32 Channel5_Data;
	s32 Channel5_Bit;

	//Status Register
	s32 StatusReg;

	//Some weird stuff :/
	s32 FrameCount;
	s32 FrameIRQEnable;

	s32 Is_PAL;

	s32 SampleRate;

	s32 ResetCounter;

	float ClockAccum;
	float OutAccum;

} State;

s32 snd_ok;

__int64 TotalPlayedClocks;

void Apu::Reset()
{
	memset(&State,0,sizeof(State));
	State.Channels[3].NoiseReg=1;
	State.ResetCounter=1024;
}

void Apu::Init(int vmode, s32 output_sample_rate)
{
	Reset();
	State.Is_PAL = vmode;
	State.SampleRate=output_sample_rate;
	snd_ok=0;
	if(!SndInit(output_sample_rate))
		snd_ok=1;

}

void Apu::Close()
{
	SndClose();
}

void Apu::Frame_IRQ()
{
	State.StatusReg|=0x40;
	cpu->Interrupt();
}

void Apu::DMC_IRQ()
{
	State.StatusReg|=0x80;
	cpu->Interrupt();
}

u8 Apu::DMA_Read(u16 addr)
{
	return memory->Read(addr);
}

void Apu::Write_Out(s32 sample)
{
	static s32 value;
	static s32 count;
	static s32 center=0;
	static s32 center_target = 0;
	//TODO: output it! :P

	value+=sample;
	count++;
	if(count>1000)
	{
		center_target=(value/1000);
		value=0;
		count=0;
	}

	center=(center * 99 + center_target*1)/100;
	sample-=center;

	if(snd_ok)
		SndWrite(sample<<7,sample<<7);
}

void Apu::Write(u16 addr, u8 value)
{
	int ch=(addr>>2)&3;
	int index=0;

	//if(State.ResetCounter>0) return;

	switch(addr)
	{
	case 0x4000: // APU Volume/Decay Channel 1 (Rectangle)
	case 0x4004: // APU Volume/Decay Channel 2 (Rectangle)
	case 0x400C: // APU Volume/Decay Channel 4 (Noise)

		// Length Counter uses this
		State.Channels[ch].DecayRate=value&0xF;

		if(value&0x10) 
		{
			State.Channels[ch].DecayEnable=0;
			State.Channels[ch].Volume=(value&0xF);
		}
		else
		{
			State.Channels[ch].DecayEnable=1;
		}

		if(value&0x20)
		{
			State.Channels[ch].LengthCountEnable=0;
			State.Channels[ch].DecayLoop=1;
		}
		else
		{
			State.Channels[ch].LengthCountEnable=1;
			State.Channels[ch].DecayLoop=0;
		}

		State.Channels[ch].Duty = (value>>6);

		break;
	case 0x4008: // APU Linear Counter Channel 3 (Triangle)

		State.Channels[2].LinearCounter = (value&0x7f) * 14915;

		break;

	case 0x4001: // APU Sweep Channel 1 (Rectangle)
	case 0x4005: // APU Sweep Channel 2 (Rectangle)


		State.Channels[ch].SweepShift     = (value   )&7;
		State.Channels[ch].SweepDirection = (value>>3)&1;
		State.Channels[ch].SweepRate      = (value>>4)&7;
		State.Channels[ch].SweepEnable    = (value>>7)&1;

		break;

	case 0x4002: // APU Frequency Channel 1 (Rectangle)
	case 0x4006: // APU Frequency Channel 2 (Rectangle)
	case 0x400A: // APU Frequency Channel 3 (Triangle)

		State.Channels[ch].Frequency = (State.Channels[ch].Frequency&0x700)|value;

		break;
	case 0x400E: //APU Frequency Channel 4 (Noise)

		State.Channels[3].Frequency = NoiseFreqs[value&15];
		State.Channels[3].ShortNoise = (value>>7)&1;

		break;
	case 0x4003: // APU Length Channel 1 (Rectangle)
	case 0x4007: // APU Length Channel 2 (Rectangle)
	case 0x400B: // APU Length Channel 3 (Triangle)
	case 0x400F: // APU Length Channel 4 (Noise)

		if(ch<3)
		{
			State.Channels[ch].Frequency = (State.Channels[ch].Frequency&0xFF)|(((s32)(value&7))<<8);
			State.Channels[ch].FreqCounter=(State.Channels[ch].Frequency+1);
			if(ch<2)
			{
				State.Channels[ch].FreqCounter<<=1;
				State.Channels[ch].Position=0;
			}
		}

		if(State.Channels[ch].Enabled)
		{
			index=(value>>3);
			State.Channels[ch].LengthCounter=(LengthCounts[index])*29830;
		}

		if(State.Channels[ch].DecayEnable)
			State.Channels[ch].Volume=0xF;

		break;

	case 0x4010: // DMC Play mode and DMA frequency

		State.Channel5_IrqEnable = (value>>7)&1;
		State.Channel5_Loop      = (value>>6)&1;
		State.Channel5_Frequency = DMCFreqs[value&15];

		break;
	case 0x4011: // DMC Delta counter load register

		State.Channel5_Delta = value&0x7f;
		State.Channel5_FreqCounter = (State.Channel5_Frequency+1)/8;

		break;
	case 0x4012: // DMC address load register

		State.Channel5_DMAStart = ((0xC000 + value * 0x40) | 0x8000)&0xFFFF;
		State.Channel5_DMAAddr = State.Channel5_DMAStart;
		State.Channel5_Bit=8;

		break;
	case 0x4013: // DMC length register

		State.Channel5_DMALength = 0x10 * value + 1;

		break;
	case 0x4015: // DMC/IRQ/length counter status/channel enable register

		for(int i=0;i<4;i++)
		{
			State.Channels[i].Enabled = (value>>i)&1;
			if(!State.Channels[i].Enabled)
			{
				State.Channels[i].LengthCounter=0;
			}
		}

		if(((value>>4)&1)&&(!State.Channel5_Enabled))
		{
			State.Channel5_DMAAddr = State.Channel5_DMAStart;
			State.Channel5_Bit=8;
		}
		State.Channel5_Enabled    = (value>>4)&1;

		// DMC IRQ ACK
		State.StatusReg&=0x7F; //clear DMC irq flag

		break;

	case 0x4017: // APU Low frequency timer control (W)
		ch=State.FrameIRQEnable;
		index=State.Is_PAL;
		State.FrameIRQEnable=((value>>6)&1)^1;
		State.Is_PAL=((value>>7)&1)^1;
		
		if(State.FrameIRQEnable&&(!ch))
		{
			State.FrameCount = 29830;
		}

		//State.StatusReg|=0x40;

		if((State.Is_PAL)&&(!index))
		//if(value&0x80)
		{
			State.StatusReg&=0xBF;
			for(int i=0;i<4;i++)
			{
				if(State.Channels[i].LengthCountEnable)
					State.Channels[i].LengthCounter=0;
			}
			State.Channel5_DMALength=0;
		}
		
		if(!State.FrameIRQEnable)
		{
			State.StatusReg&=0xBF;
		}
		break;

	default:
		printf("APU: Unhandled Memory Write: [0x%04x]=0x%02x\n",addr,value);
		break;

	}
}

u8 Apu::Read(u16 addr)
{
	u8 ret=0;

	//if(State.ResetCounter>0) return 0;

	switch(addr)
	{
	case 0x4015:
		ret |= ((State.Channels[0].LengthCounter>0)&1);
		ret |= ((State.Channels[1].LengthCounter>0)&1)<<1;
		ret |= ((State.Channels[2].LengthCounter>0)&1)<<2;
		ret |= ((State.Channels[3].LengthCounter>0)&1)<<3;
		ret |= ((State.Channel5_DMALength>0)&1)<<4;
		ret |= State.StatusReg;

		// Frame IRQ ACK
		State.StatusReg&=0xBF; //clear frame irq flag
		//State.FrameCount = 29830;

		break;
	default:
		printf("APU: Unhandled Memory Read: [0x%04x]\n",addr);
	}
	return ret;
}

s32 mix_max=0;
s32 mix_min=0;

void Apu::Emulate(u32 clocks) //APU clock rate = 1789800Hz (DDR'd)
{
	// clock rate = 1789800Hz (DDR'd)
	// frame rate = clock rate * 2 / 14915 = 240Hz

	// After 2A03 reset, the sound channels are unavailable for playback during the first 2048 CPU clocks.

	// The rectangle channel(s) frequency in the range of 54.6 Hz to 12.4 KHz.
	// The triangle wave channel range of 27.3 Hz to 55.9 KHz.
	// The random wavelength channel range anywhere from 29.3 Hz to 447 KHz. << base time

	// PWM max bit frequency = ~ 447KHz = clock rate / 4
	// triangle max frequency = PWM max / 8 = clock rate /32
	// triangle max frequency = PWM max / 8 = clock rate /32

	//if(sound_mute) return;

	TotalPlayedClocks +=clocks;

	if(clocks==0) return;

	if(State.Is_PAL)
		State.ClockAccum += clocks * 4.0f/5.0f;
	else
		State.ClockAccum += clocks;


	float cps = 1789800.0f/(State.SampleRate);

//	if(State.Is_PAL)
//		cps*=5.0f/4.0f;

	while(State.ClockAccum>=1)
	{
		static s32 ch[5]={0,0,0,0,0}; // 0..15 each

		if(State.ResetCounter>0)
		{
			State.ResetCounter-=2;
		}
		else
		{
			//frame IRQ
			if((State.FrameCount>0)&&(State.FrameIRQEnable))
			{
				State.FrameCount--; //~60hz on NTSC mode
				if(State.FrameCount<=0)
				{
					Apu::Frame_IRQ();
					State.FrameCount = 29830;
				}
			}

			for(int i=0;i<2;i++)
			{
				if((State.Channels[i].Enabled)&&(State.Channels[i].LengthCounter>0))
				{
					State.Channels[i].FreqCounter --;

					if(State.Channels[i].LengthCountEnable)
					{
						State.Channels[i].LengthCounter--;
					}

					if(State.Channels[i].Volume>0)
					{
						if((State.Channels[i].SweepEnable)&&(State.Channels[i].SweepShift>0)&&(State.Channels[i].SweepRate>0))
						{
							State.Channels[i].SweepCounter --;

							if(State.Channels[i].SweepCounter<=0)
							{
								s32 f=(State.Channels[i].Frequency>>State.Channels[i].SweepShift);
								if(!State.Channels[i].SweepDirection)
								{
									f=(~f)+i;
								}
								State.Channels[i].Frequency +=f;
								State.Channels[i].SweepCounter+=State.Channels[i].SweepRate*14915;

								if(State.Channels[i].Frequency<0)
								{
									State.Channels[i].Frequency=0;
								}
							}
						}

						if((State.Channels[i].DecayEnable)&&(State.Channels[i].DecayRate>0))
						{
							State.Channels[i].DecayCounter-=2;

							if(State.Channels[i].DecayCounter<=0)
							{
								State.Channels[i].Volume--;
								State.Channels[i].DecayCounter+=State.Channels[i].DecayRate*4*14915;

								if((State.Channels[i].Volume<=0)&&(State.Channels[i].DecayLoop))
								{
									State.Channels[i].Volume=0xF;
								}
							}
						}

						if((State.Channels[i].FreqCounter<=0)&&(((State.Channels[i].Frequency+1))>0))
						{
							State.Channels[i].FreqCounter += (State.Channels[i].Frequency+1);
							State.Channels[i].Position=(State.Channels[i].Position+1)%8;
						}

						ch[i]=0;
						if((State.Channels[i].Frequency<=0x7FF)&&(State.Channels[i].Frequency>=0x8))
						{
							ch[i]= Rcycle[State.Channels[i].Duty][State.Channels[i].Position]?State.Channels[i].Volume:0;
						}
					}
				}
			}

			// get value for channel 3 (0..15)
			if((State.Channels[2].Enabled)&&(State.Channels[2].LengthCounter>0)&&(State.Channels[2].LinearCounter>0))
			{
				State.Channels[2].FreqCounter --;

				if(State.Channels[2].LengthCountEnable)
				{
					State.Channels[2].LengthCounter --;
				}
				else
				{
					State.Channels[2].LinearCounter -= 2;
				}

				if((State.Channels[2].FreqCounter<=0)&&(((State.Channels[2].Frequency+1)/4)>0))
				{
					State.Channels[2].FreqCounter += (State.Channels[2].Frequency+1);
					State.Channels[2].Position=(State.Channels[2].Position+1)%32;
				}

				ch[2]= TriangleCycle[State.Channels[2].Position];
			}

			// get value for channel 4 (0..15)
			if((State.Channels[3].Enabled)&&(State.Channels[3].LengthCounter>0))
			{
				State.Channels[3].FreqCounter --;

				if(State.Channels[3].LengthCountEnable)
				{
					State.Channels[3].LengthCounter --;
				}

				if((State.Channels[3].DecayEnable)&&(State.Channels[3].DecayRate>0))
				{
					State.Channels[3].DecayCounter -=2;

					if(State.Channels[3].DecayCounter<=0)
					{
						State.Channels[3].Volume--;
						State.Channels[3].DecayCounter+=State.Channels[3].DecayRate*14915;

						if((State.Channels[3].Volume<=0)&&(State.Channels[3].DecayLoop))
						{
							State.Channels[3].Volume=0xF;
						}
					}
				}

				if((State.Channels[3].FreqCounter<=0)&&(((State.Channels[3].Frequency+1)/8)>0))
				{
					State.Channels[3].FreqCounter += (State.Channels[3].Frequency+1)/2;

					s32 t = State.Channels[3].NoiseReg>>14;

					if(State.Channels[3].ShortNoise)
						t=t ^ ((State.Channels[3].NoiseReg>>8)&1);
					else
						t=t ^ ((State.Channels[3].NoiseReg>>13)&1);

					State.Channels[3].NoiseReg=((State.Channels[3].NoiseReg<<1)&0x7fff)|t;
				}

				ch[3]= (State.Channels[3].NoiseReg&1)?State.Channels[3].Volume:0;
			}

			// get value for channel 5 (0..15)
			if((State.Channel5_Enabled)&&(State.Channel5_DMALength>0))
			{
				State.Channel5_FreqCounter --;

				if((State.Channel5_FreqCounter<=0)&&(State.Channel5_Frequency>0))
				{
					State.Channel5_FreqCounter += (State.Channel5_Frequency+1)/8;

					if(State.Channel5_Bit>=8)
					{
						State.Channel5_Data = Apu::DMA_Read(State.Channel5_DMAAddr);
						State.Channel5_Bit=0;
						State.Channel5_DMAAddr = ((State.Channel5_DMAAddr+1)&0x7fff)|0x8000;
					}

					if(State.Channel5_Data&1)
					{
						State.Channel5_Delta+=2;
						if(State.Channel5_Delta>0x7e)
							State.Channel5_Delta=0x7e;
					}
					else
					{
						State.Channel5_Delta-=2;
						if(State.Channel5_Delta<0)
							State.Channel5_Delta=0;
					}

					State.Channel5_Data>>=1;
					State.Channel5_Bit++;
				}

				ch[4]=State.Channel5_Delta;
			}
		}

		State.OutAccum++;

		if(State.OutAccum>=cps)
		{
			//if(!sound_mute)
			{

				// mix them
				s32 mix12 = (ch[0]+ch[1]);
				s32 mix12_vol = 4096 - 55 * mix12;
				s32 mix12_out = mix12 * mix12_vol / 20;     // (0..6144)

				s32 mix35 = (ch[2]*8+ch[3]*8+ch[4]);
				s32 mix35_vol = 4096 - 7 * mix35;
				s32 mix35_out = mix35 * mix35_vol / 96;		// (0..15360)

				s32 mix15 = mix12_out + mix35_out;			// (0..21504)
				s32 mix15_out = (mix15 * 10)-32256;		// (-32256..32255)

				if(mix15_out>mix_max) mix_max=mix15_out;
				if(mix15_out<mix_min) mix_min=mix15_out;

				Write_Out(mix15_out);
			}
			/*else
			{
				Write_Out(0);
			}*/
			State.OutAccum-=cps;
		}

		State.ClockAccum-=1;
	}
}
