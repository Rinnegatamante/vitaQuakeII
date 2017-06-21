/*
Copyright (C) 2017 Felipe Izzo
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "../client/client.h"
#include "../client/snd_loc.h"
#include <vitasdk.h>

#define SAMPLE_RATE   	48000
#define BUFFER_SIZE 	32768

static volatile int sound_initialized = 0;
static byte *audio_buffer;
static int stop_audio = false;
static SceRtcTick initial_tick;
static float tickRate;

static int audio_thread(int args, void *argp)
{
	int chn = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN, BUFFER_SIZE / 4, SAMPLE_RATE, SCE_AUDIO_OUT_MODE_MONO);
	sceAudioOutSetConfig(chn, -1, -1, -1);
	int vol[] = {32767, 32767};
    sceAudioOutSetVolume(chn, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vol);
	
    while (!stop_audio){
		sceAudioOutOutput(chn, audio_buffer);
	}
	free(audio_buffer);
	sceAudioOutReleasePort(chn);

    sceKernelExitDeleteThread(0);
    return 0;
}

qboolean SNDDMA_Init(void)
{
	sound_initialized = 0;

    //Force Quake to use our settings
    Cvar_SetValue( "s_khz", 48 );
	Cvar_SetValue( "s_loadas8bit", false );

    audio_buffer = malloc(BUFFER_SIZE);
	
	/* Fill the audio DMA information block */
	dma.samplebits = 16;
	dma.speed = SAMPLE_RATE;
	dma.channels = 2;
	dma.samples = BUFFER_SIZE / 4;
	dma.samplepos = 0;
	dma.submission_chunk = 1;
	dma.buffer = audio_buffer;
	
	tickRate = 1.0f / sceRtcGetTickResolution();
	
	SceUID audiothread = sceKernelCreateThread("Sound Thread", (void*)&audio_thread, 0x10000100, 0x800000, 0, 0, NULL);
	int res = sceKernelStartThread(audiothread, 0, NULL);
	if (res != 0){
		Sys_Error("Failed to init audio thread (0x%x)", res);
		return false;
	}
	
	sceRtcGetCurrentTick(&initial_tick);
	sound_initialized = 1;

	return true;
}

int SNDDMA_GetDMAPos(void)
{
	if(!sound_initialized)
		return 0;

	SceRtcTick tick;
	sceRtcGetCurrentTick(&tick);
	const unsigned int deltaTick  = tick.tick - initial_tick.tick;
	const float deltaSecond = deltaTick * tickRate;
	dma.samplepos = deltaSecond * SAMPLE_RATE * 2;
	return dma.samplepos;
}

void SNDDMA_Shutdown(void)
{
	if(!sound_initialized)
		return;

	stop_audio = true;

	sound_initialized = 0;
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{

}

void SNDDMA_BeginPainting(void)
{    
}