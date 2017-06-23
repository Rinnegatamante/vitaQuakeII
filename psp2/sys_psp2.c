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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "../qcommon/qcommon.h"
#include "../client/keys.h"
#include "../client/qmenu.h"
#include "errno.h"

#include <vitasdk.h>
#include <vita2d.h>

int _newlib_heap_size_user = 192 * 1024 * 1024;
int	curtime;
unsigned	sys_frame_time;

int		hunkcount;
int vita2d_console = 1;

static byte	*membase;
static int		hunkmaxsize;
static int		cursize;

void *GetGameAPI (void *import);

vita2d_pgf* fnt;
int y = 20;
void vita2d_printf(const char *format, ...){
	
	if (!vita2d_console) return;
	
	char str[512] = { 0 };
	va_list va;

	va_start(va, format);
	vsnprintf(str, 512, format, va);
	va_end(va);
	
	int i;
	for (i=0;i<3;i++){
		vita2d_start_drawing();
		vita2d_pgf_draw_text(fnt, 2, y, RGBA8(0xFF, 0xFF, 0xFF, 0xFF), 1.0, str);
		vita2d_end_drawing();
		vita2d_wait_rendering_done();
		vita2d_swap_buffers();
	}
	y += 15;
	if (y > 540){
		y = 20;
		for (i=0;i<3;i++){
			vita2d_start_drawing();
			vita2d_clear_screen();
			vita2d_end_drawing();
			vita2d_wait_rendering_done();
			vita2d_swap_buffers();
		}
	}

}

void Sys_Error (char *error, ...)
{
	char str[512] = { 0 };
	va_list		argptr;

	va_start (argptr,error);
	vsnprintf (str,512, error, argptr);
	va_end (argptr);
	vita2d_printf(str);

	while(1)
	{
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if (pad.buttons & SCE_CTRL_START)
				break;
	}

}

void Sys_Quit (void)
{
	vita2d_fini();
	sceKernelExitDeleteThread(0);
}

void Sys_UnloadGame (void)
{

} 

void *Sys_GetGameAPI (void *parms)
{
	return GetGameAPI (parms);
}


char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_ConsoleOutput (char *string)
{
	vita2d_printf("%s",string);
}

void utf2ascii(char* dst, uint16_t* src){
	if(!src || !dst)return;
	while(*src)*(dst++)=(*(src++))&0xFF;
	*dst=0x00;
}

void Sys_DefaultConfig(void)
{

	//Cbuf_AddText("viewsize 80\n");
	//Cbuf_AddText("sw_mipcap 1\n");

	Cbuf_AddText ("bind PADUP \"invuse\"\n");
	Cbuf_AddText ("bind PADDOWN \"inven\"\n");
	Cbuf_AddText ("bind PADLEFT \"invprev\"\n");
	Cbuf_AddText ("bind PADRIGHT \"invnext\"\n");

	Cbuf_AddText ("bind CROSS \"+moveup\"\n");
	Cbuf_AddText ("bind CIRCLE \"+movedown\"\n");
	Cbuf_AddText ("bind SQUARE \"+attack\"\n");
	Cbuf_AddText ("bind TRIANGLE \"weapnext\"\n");
	Cbuf_AddText ("bind LTRIGGER \"+speed\"\n");
	Cbuf_AddText ("bind RTRIGGER \"+attack\"\n");

	Cbuf_AddText ("bind SELECT \"+showscores\"\n");

	Cbuf_AddText ("lookstrafe \"1.000000\"\n");
	Cbuf_AddText ("lookspring \"0.000000\"\n");
	Cbuf_AddText ("gamma \"0.700000\"\n");
}

extern menufield_s s_maxclients_field;
extern int isKeyboard;
uint16_t input_text[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
char* targetKeyboard;
void Sys_SetKeys(uint32_t keys, uint32_t state){
	if (!isKeyboard){
		if( keys & SCE_CTRL_SELECT)
			Key_Event(K_ESCAPE, state, Sys_Milliseconds());
		if( keys & SCE_CTRL_START)
			Key_Event(K_ENTER, state, Sys_Milliseconds());
		if( keys & SCE_CTRL_UP)
			Key_Event(K_UPARROW, state, Sys_Milliseconds());
		if( keys & SCE_CTRL_DOWN)
			Key_Event(K_DOWNARROW, state, Sys_Milliseconds());
		if( keys & SCE_CTRL_LEFT)
			Key_Event(K_LEFTARROW, state, Sys_Milliseconds());
		if( keys & SCE_CTRL_RIGHT)
			Key_Event(K_RIGHTARROW, state, Sys_Milliseconds());
		if( keys & SCE_CTRL_TRIANGLE)
			Key_Event(K_AUX4, state, Sys_Milliseconds());
		if( keys & SCE_CTRL_SQUARE)
			Key_Event(K_AUX3, state, Sys_Milliseconds());
		if( keys & SCE_CTRL_CIRCLE)
			Key_Event(K_AUX2, state, Sys_Milliseconds());
		if( keys & SCE_CTRL_CROSS)
			Key_Event(K_AUX1, state, Sys_Milliseconds());
		if( keys & SCE_CTRL_LTRIGGER)
			Key_Event(K_AUX5, state, Sys_Milliseconds());
		if( keys & SCE_CTRL_RTRIGGER)
			Key_Event(K_AUX7, state, Sys_Milliseconds());
	}else{
		SceCommonDialogStatus status = sceImeDialogGetStatus();
		if (status == 2) {
			SceImeDialogResult result;
			memset(&result, 0, sizeof(SceImeDialogResult));
			sceImeDialogGetResult(&result);

			if (result.button == SCE_IME_DIALOG_BUTTON_ENTER)
			{
				utf2ascii(targetKeyboard, input_text);
				if (targetKeyboard == s_maxclients_field.buffer){ // Max players == 8
					if (atoi(targetKeyboard) > 8) sprintf(targetKeyboard, "8");
				}
			}

			sceImeDialogTerm();
			isKeyboard = 0;
		}
	}
}

void Sys_SendKeyEvents (void)
{
	SceCtrlData kDown, kUp;
	sceCtrlPeekBufferPositive(0, &kDown, 1);
	sceCtrlPeekBufferNegative(0, &kUp, 1);
	if(kDown.buttons)
		Sys_SetKeys(kDown.buttons, true);
	if(kUp.buttons)
		Sys_SetKeys(kUp.buttons, false);

	sys_frame_time = Sys_Milliseconds();
}


void Sys_AppActivate (void)
{
}

void Sys_CopyProtect (void)
{
}

char *Sys_GetClipboardData( void )
{
	return NULL;
}

void *Hunk_Begin (int maxsize)
{
	// reserve a huge chunk of memory, but don't commit any yet
	hunkmaxsize = maxsize;
	cursize = 0;
	membase = malloc(hunkmaxsize);

	if (membase == NULL)
		Sys_Error("unable to allocate %d bytes", hunkmaxsize);
	else
		memset (membase, 0, hunkmaxsize);

	return membase;
}

void *Hunk_Alloc (int size)
{
	byte *buf;

	// round to cacheline
	size = (size+31)&~31;

	if (cursize + size > hunkmaxsize)
		Sys_Error("Hunk_Alloc overflow");

	buf = membase + cursize;
	cursize += size;

	return buf;
}

int Hunk_End (void)
{
	byte *n;

	n = realloc(membase, cursize);

	if (n != membase)
		Sys_Error("Hunk_End:  Could not remap virtual block (%d)", errno);

	return cursize;
}

void Hunk_Free (void *base)
{
	if (base)
		free(base);
}

int Sys_Milliseconds (void)
{
	static uint64_t	base;

	uint64_t time = sceKernelGetProcessTimeWide();
	
	if (!base)
	{
		base = time;
	}

	curtime = (int)(time - base);
	
	return (curtime / 1000);
}

void Sys_Mkdir (char *path)
{
	sceIoMkdir(path, 0777);
}

void Sys_MkdirRecursive(char *path) {
        char tmp[256];
        char *p = NULL;
        size_t len;
 
        snprintf(tmp, sizeof(tmp),"%s",path);
        len = strlen(tmp);
        if(tmp[len - 1] == '/')
                tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++)
                if(*p == '/') {
                        *p = 0;
                        Sys_Mkdir(tmp);
                        *p = '/';
                }
        Sys_Mkdir(tmp);
}

static	char	findbase[MAX_OSPATH];
static	char	findpath[MAX_OSPATH];
static	char	findpattern[MAX_OSPATH];
static	SceUID	fdir;

int glob_match(char *pattern, char *text);

static qboolean CompareAttributes(char *path, char *name,
	unsigned musthave, unsigned canthave )
{
	SceIoStat st;
	char fn[MAX_OSPATH];

// . and .. never match
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return false;

	sprintf(fn, "%s/%s", path, name);
	if (sceIoGetstat(fn, &st) == -1)
		return false; // shouldn't happen

	if ( ( st.st_mode & SCE_SO_IFDIR ) && ( canthave & SFF_SUBDIR ) )
		return false;

	if ( ( musthave & SFF_SUBDIR ) && !( st.st_mode & SCE_SO_IFDIR ) )
		return false;

	return true;
}

char *Sys_FindFirst (char *path, unsigned musthave, unsigned canhave)
{
	SceIoDirent d;
	char *p;

	if (fdir)
		Sys_Error ("Sys_BeginFind without close");

//	COM_FilePath (path, findbase);
	strcpy(findbase, path);

	if ((p = strrchr(findbase, '/')) != NULL) {
		*p = 0;
		strcpy(findpattern, p + 1);
	} else
		strcpy(findpattern, "*");

	if (strcmp(findpattern, "*.*") == 0)
		strcpy(findpattern, "*");
	
	fdir = sceIoDopen(findbase);
	if (fdir <= 0)
		return NULL;
	while ((sceIoDread(fdir, &d)) > 0) {
		if (!*findpattern || glob_match(findpattern, d.d_name)) {
//			if (*findpattern)
//				printf("%s matched %s\n", findpattern, d->d_name);
			if (CompareAttributes(findbase, d.d_name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, d.d_name);
				return findpath;
			}
		}
	}
	return NULL;
}

char *Sys_FindNext (unsigned musthave, unsigned canhave)
{
	SceIoDirent d;

	if (fdir <= 0)
		return NULL;
	while ((sceIoDread(fdir, &d)) > 0) {
		if (!*findpattern || glob_match(findpattern, d.d_name)) {
//			if (*findpattern)
//				printf("%s matched %s\n", findpattern, d->d_name);
			if (CompareAttributes(findbase, d.d_name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, d.d_name);
				return findpath;
			}
		}
	}
	return NULL;
}

void Sys_FindClose (void)
{
	if (fdir > 0)
		sceIoDclose(fdir);
		
	fdir = 0;
}

void	Sys_Init (void)
{
}


//=============================================================================
int quake_main (unsigned int argc, void* argv){
	int	time, oldtime, newtime;
	
	Qcommon_Init (argc, argv);

	oldtime = Sys_Milliseconds ();

	while (1)
	{
		do {
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);
		Qcommon_Frame (time);
		oldtime = newtime;
	}

	return 0;
}


int main (int argc, char **argv)
{

	sceAppUtilInit(&(SceAppUtilInitParam){}, &(SceAppUtilBootParam){});
	sceCommonDialogSetConfigParam(&(SceCommonDialogConfigParam){});

	// Setting maximum clocks
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);
	
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	
	vita2d_init();
	vita2d_set_vblank_wait(0);
	fnt = vita2d_load_default_pgf();

	Sys_MkdirRecursive("ux0:/data/quake2/baseq2");
	
	// We need a bigger stack to run Quake 2, so we create a new thread with a proper stack size
	SceUID main_thread = sceKernelCreateThread("Quake II", quake_main, 0x40, 0x800000, 0, 0, NULL);
	if (main_thread >= 0){
		sceKernelStartThread(main_thread, 0, NULL);
		sceKernelWaitThreadEnd(main_thread, NULL, NULL);
	}
	
}


