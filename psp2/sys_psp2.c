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

#include "../client/client.h"
#include "../client/qmenu.h"
#include "errno.h"

#include <vitasdk.h>
#include <vitaGL.h>

int _newlib_heap_size_user = 192 * 1024 * 1024;
int sceUserMainThreadStackSize = 8 * 1024 * 1024;
int	curtime;
unsigned	sys_frame_time;
char cmd_line[256];
	
int		hunkcount;

static byte	*membase;
static int		hunkmaxsize;
static int		cursize;

int isKeyboard;

extern uint64_t rumble_tick;
extern int scr_width;
extern int scr_height;

uint8_t is_uma0 = 0;

void *GetGameAPI (void *import);

int y = 20;
void LOG_FILE(const char *format, ...){
	#ifndef RELEASE
	__gnuc_va_list arg;
	int done;
	va_start(arg, format);
	char msg[512];
	done = vsprintf(msg, format, arg);
	va_end(arg);
	int i;
	sprintf(msg, "%s\n", msg);
	FILE* log = fopen("ux0:/data/quake2/quake.log", "a+");
	if (log != NULL) {
		fwrite(msg, 1, strlen(msg), log);
		fclose(log);
	}
	#endif
}

void Sys_Error (char *error, ...)
{
	char str[512] = { 0 };
	va_list		argptr;

	va_start (argptr,error);
	vsnprintf (str,512, error, argptr);
	va_end (argptr);
	LOG_FILE(str);

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
	LOG_FILE("%s",string);
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
	Cbuf_AddText("unbindall\n");
	
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

	Cbuf_AddText ("bind SELECT \"help\"\n");

	Cbuf_AddText ("lookstrafe \"1.000000\"\n");
	Cbuf_AddText ("lookspring \"0.000000\"\n");
	Cbuf_AddText ("vid_gamma \"0.700000\"\n");
	
}

void simulateKeyPress(char* text){
	
	//We first delete the current text
	int i;
	for (i=0;i<100;i++){
		Key_Event(K_BACKSPACE, true, Sys_Milliseconds() + i * 2);
		Key_Event(K_BACKSPACE, false, Sys_Milliseconds() + i * 2 + 1);
	}
	
	i = 0;
	while (*text){
		Key_Event(*text, true, Sys_Milliseconds() + (i++));
		Key_Event(*text, false, Sys_Milliseconds() + (i++));
		text++;
	}
	
	Key_Event(K_ENTER, true, Sys_Milliseconds() + (i++));
	Key_Event(K_ENTER, false, Sys_Milliseconds() + (i++));
}

extern menufield_s s_maxclients_field;
uint16_t input_text[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
char *targetKeyboard;
void Sys_SetKeys(uint32_t keys, uint32_t state){
	if (!isKeyboard){
		if( keys & SCE_CTRL_START)
			Key_Event(K_ESCAPE, state, Sys_Milliseconds());
		if( keys & SCE_CTRL_SELECT)
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
				if (targetKeyboard == s_maxclients_field.buffer){ // Max players == 100
					if (atoi(targetKeyboard) > 100) sprintf(targetKeyboard, "100");
				} else if (targetKeyboard == cmd_line) {
					utf2ascii(cmd_line, input_text);
					simulateKeyPress(cmd_line);
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

	uint64_t time = sceKernelGetProcessTimeWide() / 1000;
	
	if (!base)
	{
		base = time;
	}

	curtime = (int)(time - base);
	
	return curtime;
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
static	char	findpath[512];
static	char	findpattern[MAX_OSPATH];
static	SceUID	fdir = -1;

int glob_match(char *pattern, char *text);

char *Sys_FindFirst (char *path, unsigned musthave, unsigned canhave)
{
	SceIoDirent d;
	char *p;

	if (fdir >= 0)
		Sys_FindClose();

	COM_FilePath (path, findbase);
	strcpy(findbase, path);

	if ((p = strrchr(findbase, '/')) != NULL) {
		*p = 0;
		strcpy(findpattern, p + 1);
	} else
		strcpy(findpattern, "*");

	if (strcmp(findpattern, "*.*") == 0)
		strcpy(findpattern, "*");
	
	fdir = sceIoDopen(findbase);
	if (fdir < 0)
		return NULL;
	while ((sceIoDread(fdir, &d)) > 0) {
		if (!*findpattern || glob_match(findpattern, d.d_name)) {
			sprintf (findpath, "%s/%s", findbase, d.d_name);
			return findpath;
		}
	}
	return NULL;
}

char *Sys_FindNext (unsigned musthave, unsigned canhave)
{
	SceIoDirent d;

	if (fdir < 0)
		return NULL;
	while ((sceIoDread(fdir, &d)) > 0) {
		if (!*findpattern || glob_match(findpattern, d.d_name)) {
			//if (*findpattern)
			//	printf("%s matched %s\n", findpattern, d.d_name);
			//if (CompareAttributes(findbase, d.d_name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, d.d_name);
				return findpath;
			//}
		}
	}
	return NULL;
}

void Sys_FindClose (void)
{
	if (fdir >= 0)
		sceIoDclose(fdir);
		
	fdir = -1;
}

void	Sys_Init (void)
{
}

extern void IN_StopRumble();
extern uint16_t input_text[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
static uint16_t title[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
static uint16_t initial_text[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
extern char title_keyboard[256];

extern void ascii2utf(uint16_t* dst, char* src);
extern void utf2ascii(char* dst, uint16_t* src);

//=============================================================================
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
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);

	FILE *f = fopen("uma0:/data/quake2/baseq2/pak0.pak", "rb");
	if (f) {
		fclose(f);
		is_uma0 = 1;
	}
	
	char res_str[64];
	if (is_uma0) f = fopen("uma0:data/quake2/resolution.cfg", "rb");
	else f = fopen("ux0:data/quake2/resolution.cfg", "rb");
	if (f != NULL){
		fread(res_str, 1, 64, f);
		fclose(f);
		sscanf(res_str, "%dx%d", &scr_width, &scr_height);
	}
	
	int	time, oldtime, newtime;
	
	// Official mission packs support
	#ifdef ROGUE
	char* int_argv[4] = {"", "+set", "game", "rogue"};
	Qcommon_Init(4, int_argv);
	#elif defined(XATRIX)
	char* int_argv[4] = {"", "+set", "game", "xatrix"};
	Qcommon_Init(4, int_argv);
	#elif defined(ZAERO)
	char* int_argv[4] = {"", "+set", "game", "zaero"};
	Qcommon_Init(4, int_argv);
	#else
	SceAppUtilAppEventParam eventParam;
	memset(&eventParam, 0, sizeof(SceAppUtilAppEventParam));
	sceAppUtilReceiveAppEvent(&eventParam);
	if (eventParam.type == 0x05){
		char buffer[2048], fname[256];
		memset(buffer, 0, 2048);
		sceAppUtilAppEventParseLiveArea(&eventParam, buffer);
		sprintf(fname, "app0:%s.bin", buffer);
		sceAppMgrLoadExec(fname, NULL, NULL);	
	}else Qcommon_Init (argc, argv);
	#endif
	oldtime = Sys_Milliseconds ();
	
	// Disabling all FPU exceptions traps on main thread
	sceKernelChangeThreadVfpException(0x0800009FU, 0x0);

	while (1)
	{
		
		// Rumble effect managing (PSTV only)
		if (rumble_tick != 0) {
			if (sceKernelGetProcessTimeWide() - rumble_tick > 500000) IN_StopRumble(); // 0.5 sec
		}
		
		// OSK management for Console
		if (cls.key_dest == key_console) {
			SceCtrlData tmp_pad;
			uint32_t oldpad = 0;
			sceCtrlPeekBufferPositive(0, &tmp_pad, 1);
			if (!isKeyboard) {
				if ((tmp_pad.buttons & SCE_CTRL_SELECT) && (!(oldpad & SCE_CTRL_SELECT))) {
					isKeyboard = 1;
					memset(cmd_line, 0, 256);
					targetKeyboard = cmd_line;
					memset(input_text, 0, (SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1) << 1);
					memset(initial_text, 0, (SCE_IME_DIALOG_MAX_TEXT_LENGTH) << 1);
					sprintf(title_keyboard, "Insert Quake command");
					ascii2utf(title, title_keyboard);
					SceImeDialogParam param;
					sceImeDialogParamInit(&param);
					param.supportedLanguages = 0x0001FFFF;
					param.languagesForced = SCE_TRUE;
					param.type = SCE_IME_TYPE_BASIC_LATIN;
					param.title = title;
					param.maxTextLength = SCE_IME_DIALOG_MAX_TEXT_LENGTH;
					param.initialText = initial_text;
					param.inputTextBuffer = input_text;
					sceImeDialogInit(&param);
				}
			}
			oldpad = tmp_pad.buttons;
		}
		
		do {
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);
		Qcommon_Frame (time);
		oldtime = newtime;
	}
	vglEnd();
	return 0;
}


