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
#include "../client/qmenu.h"
extern viddef_t vid;

#define REF_OPENGL  0

cvar_t *vid_ref;
cvar_t *vid_fullscreen;
cvar_t *vid_gamma;
cvar_t *scr_viewsize;

extern int msaa;
extern uint8_t is_uma0;

static cvar_t *sw_mode;
static cvar_t *sw_stipplealpha;
static cvar_t *gl_picmip;
static cvar_t *gl_mode;
static cvar_t *gl_driver;
extern cvar_t *gl_shadows;
extern int scr_width;
extern int scr_height;

extern void M_ForceMenuOff( void );

static menuframework_s  s_opengl_menu;
static menuframework_s *s_current_menu;

static menulist_s       s_mode_list;
static menulist_s       s_ref_list;
static menulist_s       s_msaa;
static menuslider_s     s_tq_slider;
static menuslider_s     s_screensize_slider;
static menuslider_s     s_brightness_slider;
static menulist_s       s_fs_box;
static menulist_s       s_finish_box;
static menuaction_s     s_cancel_action;
static menuaction_s     s_defaults_action;
static menulist_s       s_shadows_slider;

viddef_t    viddef;             // global video state

refexport_t re;

refexport_t GetRefAPI (refimport_t rimp);


/*
==========================================================================

DIRECT LINK GLUE

==========================================================================
*/

#define MAXPRINTMSG 4096
void VID_Printf (int print_level, char *fmt, ...)
{
        va_list     argptr;
        char        msg[MAXPRINTMSG];

        va_start (argptr,fmt);
        vsprintf (msg,fmt,argptr);
        va_end (argptr);

        if (print_level == PRINT_ALL)
                Com_Printf ("%s", msg);
        else
                Com_DPrintf ("%s", msg);
}

void VID_Error (int err_level, char *fmt, ...)
{
        va_list     argptr;
        char        msg[MAXPRINTMSG];

        va_start (argptr,fmt);
        vsprintf (msg,fmt,argptr);
        va_end (argptr);

        Com_Error (err_level, "%s", msg);
}

void VID_NewWindow (int width, int height)
{
        viddef.width = width;
        viddef.height = height;
}

/*
** VID_GetModeInfo
*/
typedef struct vidmode_s
{
    const char *description;
    int         width, height;
    int         mode;
} vidmode_t;

vidmode_t vid_modes[] =
{
    { "Mode 0: 480x272",   480, 272,   0 },
	{ "Mode 1: 640x368",   640, 368,   1 },
	{ "Mode 2: 720x408",   720, 408,   2 },
	{ "Mode 3: 960x544",   960, 544,   3 }
};
#define VID_NUM_MODES ( sizeof( vid_modes ) / sizeof( vid_modes[0] ) )

qboolean VID_GetModeInfo( int *width, int *height, int mode )
{
    if ( mode < 0 || mode >= VID_NUM_MODES )
        return false;

    *width  = vid_modes[mode].width;
    *height = vid_modes[mode].height;
    printf("VID_GetModeInfo %dx%d mode %d\n",width,height,mode);

    return true;
}

static void NullCallback( void *unused )
{
}

static void ResCallback( void *unused )
{
	char res_str[64];
	FILE *f = NULL;
	if (is_uma0) f = fopen("uma0:data/quake2/resolution.cfg", "wb");
	else f = fopen("ux0:data/quake2/resolution.cfg", "wb");
	sprintf(res_str, "%dx%d", vid_modes[s_mode_list.curvalue].width, vid_modes[s_mode_list.curvalue].height);
	fwrite(res_str, 1, strlen(res_str), f);
	fclose(f);
}

static void MsaaCallback( void *unused )
{
	char res_str[64];
	FILE *f = NULL;
	if (is_uma0) f = fopen("uma0:data/quake2/antialiasing.cfg", "wb");
	else f = fopen("ux0:data/quake2/antialiasing.cfg", "wb");
	sprintf(res_str, "%d", s_msaa.curvalue);
	fwrite(res_str, 1, strlen(res_str), f);
	fclose(f);
}

static void ScreenSizeCallback( void *s )
{
    menuslider_s *slider = ( menuslider_s * ) s;

    Cvar_SetValue( "viewsize", slider->curvalue * 10 );
}

static void ShadowsCallback( void *unused )
{
	Cvar_SetValue( "gl_shadows", s_shadows_slider.curvalue );
}

static void BrightnessCallback( void *s )
{
    menuslider_s *slider = ( menuslider_s * ) s;

    //if ( strcmp( vid_ref->string, "soft" ) == 0 )
    {
        float gamma = ( 0.8 - ( slider->curvalue/10.0 - 0.5 ) ) + 0.5;

        Cvar_SetValue( "vid_gamma", gamma );
    }
}

static void ResetDefaults( void *unused )
{
	VID_MenuInit();
}

static void ApplyChanges( void *unused )
{
    float gamma;

    /*
    ** invert sense so greater = brighter, and scale to a range of 0.5 to 1.3
    */
    gamma = ( 0.8 - ( s_brightness_slider.curvalue/10.0 - 0.5 ) ) + 0.5;

    Cvar_SetValue( "vid_gamma", gamma );
    //Cvar_SetValue( "gl_mode", s_mode_list.curvalue );
	Cvar_SetValue( "gl_picmip", 3 - s_tq_slider.curvalue );
	
    Cvar_Set( "vid_ref", "gl" );
	Cvar_Set( "gl_driver", "opengl32" );

    M_ForceMenuOff();
}

static void CancelChanges( void *unused )
{
    extern void M_PopMenu( void );

    M_PopMenu();
}

void    VID_Init (void)
{
    refimport_t ri;
	
    viddef.width = scr_width;
    viddef.height = scr_height;

    ri.Cmd_AddCommand = Cmd_AddCommand;
    ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
    ri.Cmd_Argc = Cmd_Argc;
    ri.Cmd_Argv = Cmd_Argv;
    ri.Cmd_ExecuteText = Cbuf_ExecuteText;
    ri.Con_Printf = VID_Printf;
    ri.Sys_Error = VID_Error;
    ri.FS_LoadFile = FS_LoadFile;
    ri.FS_FreeFile = FS_FreeFile;
    ri.FS_Gamedir = FS_Gamedir;
    ri.Vid_NewWindow = VID_NewWindow;
    ri.Cvar_Get = Cvar_Get;
    ri.Cvar_Set = Cvar_Set;
    ri.Cvar_SetValue = Cvar_SetValue;
    ri.Vid_GetModeInfo = VID_GetModeInfo;
	ri.Vid_MenuInit = VID_MenuInit;
	
    //JASON this is called from the video DLL
    re = GetRefAPI(ri);

    if (re.api_version != API_VERSION)
        Com_Error (ERR_FATAL, "Re has incompatible api_version");
    
        // call the init function
    if (re.Init (NULL, NULL) == -1)
        Com_Error (ERR_FATAL, "Couldn't start refresh");

    vid_ref = Cvar_Get ("vid_ref", "soft", CVAR_ARCHIVE);
    vid_fullscreen = Cvar_Get ("vid_fullscreen", "0", CVAR_ARCHIVE);
    vid_gamma = Cvar_Get( "vid_gamma", "1", CVAR_ARCHIVE );

}

void    VID_Shutdown (void)
{
    if (re.Shutdown)
        re.Shutdown ();
}

void    VID_CheckChanges (void)
{
}

void    VID_MenuInit (void)
{

	static const char *resolutions[] = 
	{
		"480x272",
		"640x368",
		"720x408",
		"960x544",
		0
	};
	
	static const char *msaa_modes[] =
	{
		"disabled",
		"MSAA 2x",
		"MSAA 4x",
		0
	};
	
	static const char *refs[] =
	{
		"vitaGL",
		0
	};
	
    static const char *yesno_names[] =
    {
        "no",
        "yes",
        0
    };
    int i;

	if ( !gl_driver )
		gl_driver = Cvar_Get( "gl_driver", "opengl32", 0 );
	if ( !gl_picmip )
		gl_picmip = Cvar_Get( "gl_picmip", "0", 0 );
	if ( !gl_mode )
		gl_mode = Cvar_Get( "gl_mode", "3", 0 );
    if ( !scr_viewsize )
        scr_viewsize = Cvar_Get ("viewsize", "100", CVAR_ARCHIVE);
	
    s_screensize_slider.curvalue = scr_viewsize->value/10;

    s_shadows_slider.curvalue = gl_shadows->value;
	
	switch (viddef.width) {
		case 960:
			s_mode_list.curvalue = 3;
			break;
		case 720:
			s_mode_list.curvalue = 2;
			break;
		case 640:
			s_mode_list.curvalue = 1;
			break;
		default:
			s_mode_list.curvalue = 0;
			break;
	}
	Cvar_SetValue( "gl_mode", s_mode_list.curvalue );
	
    s_ref_list.curvalue = REF_OPENGL;
	s_msaa.curvalue = msaa;
    s_opengl_menu.x = viddef.width * 0.50;
	s_opengl_menu.nitems = 0;

	s_ref_list.generic.type = MTYPE_SPINCONTROL;
	s_ref_list.generic.name = "driver";
	s_ref_list.generic.x = 0;
	s_ref_list.generic.y = 0;
	s_ref_list.generic.callback = NullCallback;
	s_ref_list.itemnames = refs;

    s_mode_list.generic.type = MTYPE_SPINCONTROL;
    s_mode_list.generic.x        = 0;
    s_mode_list.generic.y        = 10;
    s_mode_list.generic.name = "video mode";
	s_mode_list.generic.statusbar = "you need to restart vitaQuakeII to apply changes";
	s_mode_list.generic.callback = ResCallback;
    s_mode_list.itemnames = resolutions;
	
	s_screensize_slider.generic.type	= MTYPE_SLIDER;
	s_screensize_slider.generic.x		= 0;
	s_screensize_slider.generic.y		= 20;
	s_screensize_slider.generic.name	= "screen size";
	s_screensize_slider.minvalue = 3;
	s_screensize_slider.maxvalue = 12;
	s_screensize_slider.generic.callback = ScreenSizeCallback;

    s_brightness_slider.generic.type = MTYPE_SLIDER;
    s_brightness_slider.generic.x    = 0;
    s_brightness_slider.generic.y    = 30;
    s_brightness_slider.generic.name = "brightness";
    s_brightness_slider.generic.callback = BrightnessCallback;
    s_brightness_slider.minvalue = 5;
    s_brightness_slider.maxvalue = 13;
    s_brightness_slider.curvalue = ( 1.3 - vid_gamma->value + 0.5 ) * 10;

	s_msaa.generic.type = MTYPE_SPINCONTROL;
	s_msaa.generic.name = "anti-aliasing";
	s_msaa.generic.x = 0;
	s_msaa.generic.y = 40;
	s_msaa.generic.callback = MsaaCallback;
	s_msaa.generic.statusbar = "you need to restart vitaQuakeII to apply changes";
	s_msaa.itemnames = msaa_modes;

    s_cancel_action.generic.type = MTYPE_ACTION;
    s_cancel_action.generic.name = "cancel";
    s_cancel_action.generic.x    = 0;
    s_cancel_action.generic.y    = 100;
    s_cancel_action.generic.callback = CancelChanges;

    s_tq_slider.generic.type	= MTYPE_SLIDER;
	s_tq_slider.generic.x		= 0;
	s_tq_slider.generic.y		= 60;
	s_tq_slider.generic.name	= "texture quality";
	s_tq_slider.minvalue = 0;
	s_tq_slider.maxvalue = 3;
	s_tq_slider.curvalue = 3-gl_picmip->value;
	
	s_defaults_action.generic.type = MTYPE_ACTION;
	s_defaults_action.generic.name = "reset to default";
	s_defaults_action.generic.x    = 0;
	s_defaults_action.generic.y    = 90;
	s_defaults_action.generic.callback = ResetDefaults;

    s_shadows_slider.generic.type = MTYPE_SPINCONTROL;
    s_shadows_slider.generic.x        = 0;
    s_shadows_slider.generic.y        = 70;
    s_shadows_slider.generic.name = "render shadows";
    s_shadows_slider.generic.callback = ShadowsCallback;
	s_shadows_slider.itemnames = yesno_names;

    Menu_AddItem( &s_opengl_menu, ( void * ) &s_ref_list );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_mode_list );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_screensize_slider );
    Menu_AddItem( &s_opengl_menu, ( void * ) &s_brightness_slider );
	Menu_AddItem( &s_opengl_menu, ( void * ) &s_msaa );
    Menu_AddItem( &s_opengl_menu, ( void * ) &s_tq_slider );
    Menu_AddItem( &s_opengl_menu, ( void * ) &s_shadows_slider );

	Menu_AddItem( &s_opengl_menu, ( void * ) &s_defaults_action );
    Menu_AddItem( &s_opengl_menu, ( void * ) &s_cancel_action );
	

    Menu_Center( &s_opengl_menu );

    s_opengl_menu.x -= 8;
}

void    VID_MenuDraw (void)
{
    int w, h;
	
	float scale = SCR_GetMenuScale();

    s_current_menu = &s_opengl_menu;

    /*
    ** draw the banner
    */
    re.DrawGetPicSize( &w, &h, "m_banner_video" );
    re.DrawPic( viddef.width / 2 - (w * scale) / 2, viddef.height /2 - 110 * scale, "m_banner_video", scale );

    /*
    ** move cursor to a reasonable starting position
    */
    Menu_AdjustCursor( s_current_menu, 1 );

    /*
    ** draw the menu
    */
    Menu_Draw( s_current_menu );
}

const char *VID_MenuKey( int k)
{
    menuframework_s *m = s_current_menu;
    static const char *sound = "misc/menu1.wav";

    switch ( k )
    {
    case K_AUX4:
        ApplyChanges( NULL );
        return NULL;
    case K_KP_UPARROW:
    case K_UPARROW:
        m->cursor--;
        Menu_AdjustCursor( m, -1 );
        break;
    case K_KP_DOWNARROW:
    case K_DOWNARROW:
        m->cursor++;
        Menu_AdjustCursor( m, 1 );
        break;
    case K_KP_LEFTARROW:
    case K_LEFTARROW:
        Menu_SlideItem( m, -1 );
        break;
    case K_KP_RIGHTARROW:
    case K_RIGHTARROW:
        Menu_SlideItem( m, 1 );
        break;
    case K_KP_ENTER:
    case K_AUX1:
        if ( !Menu_SelectItem( m ) )
            ApplyChanges( NULL );
        break;
    }

    return sound;
//  return NULL;
}