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

#include <GL/gl.h>
#include <GL/glext.h>

#include "../libretro-common/include/libretro.h"
#include "../libretro-common/include/retro_dirent.h"
#include "../libretro-common/include/features/features_cpu.h"
#include "../libretro-common/include/file/file_path.h"
#include "../libretro-common/include/glsym/glsym.h"

#include "libretro_core_options.h"

#include "../client/client.h"
#include "../client/qmenu.h"
#include "../ref_gl/gl_local.h"

#if defined(HAVE_PSGL)
#define RARCH_GL_FRAMEBUFFER GL_FRAMEBUFFER_OES
#define RARCH_GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE_OES
#define RARCH_GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0_EXT
#elif defined(OSX_PPC)
#define RARCH_GL_FRAMEBUFFER GL_FRAMEBUFFER_EXT
#define RARCH_GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE_EXT
#define RARCH_GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0_EXT
#else
#define RARCH_GL_FRAMEBUFFER GL_FRAMEBUFFER
#define RARCH_GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE
#define RARCH_GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0
#endif

static bool did_flip = false;
boolean gl_set = false;

unsigned	sys_frame_time;
uint64_t rumble_tick;

float *gVertexBuffer;
float *gColorBuffer;
float *gTexCoordBuffer;

void ( APIENTRY * qglBlendFunc )(GLenum sfactor, GLenum dfactor);
void ( APIENTRY * qglTexImage2D )(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglTexParameteri )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglBindFramebuffer )(GLenum target, GLuint framebuffer);
void ( APIENTRY * qglGenerateMipmap )(GLenum target);
void ( APIENTRY * qglTexSubImage2D )(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void ( APIENTRY * qglDepthMask )(GLboolean flag);
void ( APIENTRY * qglPushMatrix )(void);
void ( APIENTRY * qglRotatef )(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglTranslatef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglDepthRange )(GLclampd zNear, GLclampd zFar);
void ( APIENTRY * qglClear )(GLbitfield mask);
void ( APIENTRY * qglEnable )(GLenum cap);
void ( APIENTRY * qglDisable )(GLenum cap);
void ( APIENTRY * qglPopMatrix )(void);
void ( APIENTRY * qglGetFloatv )(GLenum pname, GLfloat *params);
void ( APIENTRY * qglOrtho )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY * qglFrustum )(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void ( APIENTRY * qglLoadMatrixf )(const GLfloat *m);
void ( APIENTRY * qglLoadIdentity )(void);
void ( APIENTRY * qglMatrixMode )(GLenum mode);
void ( APIENTRY * qglBindTexture )(GLenum target, GLuint texture);
void ( APIENTRY * qglReadPixels )(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void ( APIENTRY * qglPolygonMode )(GLenum face, GLenum mode);
void ( APIENTRY * qglVertexPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglColorPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglTexCoordPointer )(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void ( APIENTRY * qglDrawElements )(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void ( APIENTRY * qglClearColor )(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void ( APIENTRY * qglCullFace )(GLenum mode);
void ( APIENTRY * qglViewport )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglDeleteTextures )(GLsizei n, const GLuint *textures);
void ( APIENTRY * qglClearStencil )(GLint s);
void ( APIENTRY * qglColor4f )(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void ( APIENTRY * qglScissor )(GLint x, GLint y, GLsizei width, GLsizei height);
void ( APIENTRY * qglEnableClientState )(GLenum array);
void ( APIENTRY * qglDisableClientState )(GLenum array);
void ( APIENTRY * qglStencilFunc )(GLenum func, GLint ref, GLuint mask);
void ( APIENTRY * qglStencilOp )(GLenum fail, GLenum zfail, GLenum zpass);
void ( APIENTRY * qglScalef )(GLfloat x, GLfloat y, GLfloat z);
void ( APIENTRY * qglDepthFunc )(GLenum func);
void ( APIENTRY * qglTexEnvi )(GLenum target, GLenum pname, GLint param);
void ( APIENTRY * qglGenTextures )(GLsizei n, GLuint *textures);

char g_rom_dir[1024], g_pak_path[1024], g_save_dir[1024];

static struct retro_hw_render_callback hw_render;

#define MAX_INDICES 4096
uint16_t* indices;

float *gVertexBufferPtr;
float *gColorBufferPtr;
float *gTexCoordBufferPtr;

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
retro_environment_t environ_cb;
static retro_input_poll_t poll_cb;
static retro_input_state_t input_cb;
static struct retro_rumble_interface rumble;
static bool libretro_supports_bitmasks = false;

static void audio_callback(void);

#define SAMPLE_RATE   	44100
#define BUFFER_SIZE 	8192
#define MAX_PADS 1
static unsigned quake_devices[1];

// System analog stick range is -0x8000 to 0x8000
#define ANALOG_RANGE 0x8000
// Default deadzone: 15%
static int analog_deadzone = (int)(0.15f * ANALOG_RANGE);

#define GP_MAXBINDS 32

static bool context_needs_reinit = true;

void GL_DrawPolygon(GLenum prim, int num)
{
	qglDrawElements(prim, num, GL_UNSIGNED_SHORT, indices);
}

void vglVertexAttribPointerMapped(int id, void* ptr)
{
	switch (id){
	case 0: // Vertex
		qglVertexPointer(3, GL_FLOAT, 0, ptr);
		break;
	case 1: // TexCoord
		qglTexCoordPointer(2, GL_FLOAT, 0, ptr);
		break;
	case 2: // Color
		qglColorPointer(4, GL_FLOAT, 0, ptr);
		break;
	}
}

static bool initialize_gl()
{
	qglTexImage2D = hw_render.get_proc_address ("glTexImage2D");
	qglTexSubImage2D = hw_render.get_proc_address ("glTexSubImage2D");
	qglTexParameteri = hw_render.get_proc_address ("glTexParameteri");
	qglBindFramebuffer = hw_render.get_proc_address ("glBindFramebuffer");
	qglGenerateMipmap = hw_render.get_proc_address ("glGenerateMipmap");
	qglBlendFunc = hw_render.get_proc_address ("glBlendFunc");
	qglTexSubImage2D = hw_render.get_proc_address ("glTexSubImage2D");
	qglDepthMask = hw_render.get_proc_address ("glDepthMask");
	qglPushMatrix = hw_render.get_proc_address ("glPushMatrix");
	qglRotatef = hw_render.get_proc_address ("glRotatef");
	qglTranslatef = hw_render.get_proc_address ("glTranslatef");
	qglDepthRange = hw_render.get_proc_address ("glDepthRange");
	qglClear = hw_render.get_proc_address ("glClear");
	qglCullFace = hw_render.get_proc_address ("glCullFace");
	qglClearColor = hw_render.get_proc_address ("glClearColor");
	qglEnable = hw_render.get_proc_address ("glEnable");
	qglDisable = hw_render.get_proc_address ("glDisable");
	qglEnableClientState = hw_render.get_proc_address ("glEnableClientState");
	qglDisableClientState = hw_render.get_proc_address ("glDisableClientState");
	qglPopMatrix = hw_render.get_proc_address ("glPopMatrix");
	qglGetFloatv = hw_render.get_proc_address ("glGetFloatv");
	qglOrtho = hw_render.get_proc_address ("glOrtho");
	qglFrustum = hw_render.get_proc_address ("glFrustum");
	qglLoadMatrixf = hw_render.get_proc_address ("glLoadMatrixf");
	qglLoadIdentity = hw_render.get_proc_address ("glLoadIdentity");
	qglMatrixMode = hw_render.get_proc_address ("glMatrixMode");
	qglBindTexture = hw_render.get_proc_address ("glBindTexture");
	qglReadPixels = hw_render.get_proc_address ("glReadPixels");
	qglPolygonMode = hw_render.get_proc_address ("glPolygonMode");
	qglVertexPointer = hw_render.get_proc_address ("glVertexPointer");
	qglTexCoordPointer = hw_render.get_proc_address ("glTexCoordPointer");
	qglColorPointer = hw_render.get_proc_address ("glColorPointer");
	qglDrawElements = hw_render.get_proc_address ("glDrawElements");
	qglViewport = hw_render.get_proc_address ("glViewport");
	qglDeleteTextures = hw_render.get_proc_address ("glDeleteTextures");
	qglClearStencil = hw_render.get_proc_address ("glClearStencil");
	qglColor4f = hw_render.get_proc_address ("glColor4f");
	qglScissor = hw_render.get_proc_address ("glScissor");
	qglStencilFunc = hw_render.get_proc_address ("glStencilFunc");
	qglStencilOp = hw_render.get_proc_address ("glStencilOp");
	qglScalef = hw_render.get_proc_address ("glScalef");
	qglDepthFunc = hw_render.get_proc_address ("glDepthFunc");
	qglTexEnvi = hw_render.get_proc_address ("glTexEnvi");
	qglGenTextures = hw_render.get_proc_address ("glGenTextures");
	
	return true;
}

int GLimp_Init( void *hinstance, void *wndproc )
{
}

void GLimp_AppActivate( qboolean active )
{
}

void GLimp_BeginFrame( float camera_separation )
{
	gVertexBuffer = gVertexBufferPtr;
	gColorBuffer = gColorBufferPtr;
	gTexCoordBuffer = gTexCoordBufferPtr;
	qglEnableClientState(GL_VERTEX_ARRAY);
}

void GLimp_EndFrame (void)
{
	did_flip = true;
}

boolean GLimp_InitGL (void)
{
    int i;
	indices = (uint16_t*)malloc(sizeof(uint16_t*)*MAX_INDICES);
	for (i=0;i<MAX_INDICES;i++){
		indices[i] = i;
	}
	gVertexBufferPtr = (float*)malloc(0x400000);
	gColorBufferPtr = (float*)malloc(0x200000);
	gTexCoordBufferPtr = (float*)malloc(0x200000);
	gl_set = true;
	return true;
}

static void context_destroy() 
{
	context_needs_reinit = true;
}

static void context_reset() { 
	if (!context_needs_reinit)
		return;

	initialize_gl();

	context_needs_reinit = false;
}

typedef struct {
   struct retro_input_descriptor desc[GP_MAXBINDS];
   struct {
      char *key;
      char *com;
   } bind[GP_MAXBINDS];
} gp_layout_t;

gp_layout_t modern = {
   {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Swim Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Strafe Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Strafe Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Swim Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Previous Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Next Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "Jump" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "Fire" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Show Scores" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Menu" },
      { 0 },
   },
   {
      {"JOY_LEFT",  "+moveleft"},     {"JOY_RIGHT", "+moveright"},
      {"JOY_DOWN",  "+back"},         {"JOY_UP",    "+forward"},
      {"JOY_B",     "+movedown"},     {"JOY_A",     "+moveright"},
      {"JOY_X",     "+moveup"},       {"JOY_Y",     "+moveleft"},
      {"JOY_L",     "impulse 12"},    {"JOY_R",     "impulse 10"},
      {"JOY_L2",    "+jump"},         {"JOY_R2",    "+attack"},
      {"JOY_SELECT","+showscores"},   {"JOY_START", "togglemenu"},
      { 0 },
   },
};
gp_layout_t classic = {

   {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Jump" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Cycle Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Freelook" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Fire" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Strafe Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Strafe Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "Look Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "Look Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "Move Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "Swim Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Toggle Run Mode" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Menu" },
      { 0 },
   },
   {
      {"JOY_LEFT",  "+left"},         {"JOY_RIGHT", "+right"},
      {"JOY_DOWN",  "+back"},         {"JOY_UP",    "+forward"},
      {"JOY_B",     "+jump"} ,        {"JOY_A",     "impulse 10"},
      {"JOY_X",     "+klook"},        {"JOY_Y",     "+attack"},
      {"JOY_L",     "+moveleft"},     {"JOY_R",     "+moveright"},
      {"JOY_L2",    "+lookup"},       {"JOY_R2",    "+lookdown"},
      {"JOY_L3",    "+movedown"},     {"JOY_R3",    "+moveup"},
      {"JOY_SELECT","+togglewalk"},   {"JOY_START", "togglemenu"},
      { 0 },
   },
};
gp_layout_t classic_alt = {

   {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "Look Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Look Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,     "Look Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,     "Look Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,     "Jump" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,     "Fire" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,    "Run" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,    "Next Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,    "Move Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,    "Previous Weapon" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,"Toggle Run Mode" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Menu" },
      { 0 },
   },
   {
      {"JOY_LEFT",  "+moveleft"},     {"JOY_RIGHT", "+moveright"},
      {"JOY_DOWN",  "+back"},         {"JOY_UP",    "+forward"},
      {"JOY_B",     "+lookdown"},     {"JOY_A",     "+right"},
      {"JOY_X",     "+lookup"},       {"JOY_Y",     "+left"},
      {"JOY_L",     "+jump"},         {"JOY_R",     "+attack"},
      {"JOY_L2",    "+speed"},          {"JOY_R2",    "impulse 10"},
      {"JOY_L3",    "+movedown"},     {"JOY_R3",    "impulse 12"},
      {"JOY_SELECT","+togglewalk"},   {"JOY_START", "togglemenu"},
      { 0 },
   },
};

gp_layout_t *gp_layoutp = NULL;

/* sys.c */
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "errno.h"

#define RETRO_DEVICE_MODERN  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 2)
#define RETRO_DEVICE_JOYPAD_ALT  RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 1)

int	curtime;
char cmd_line[256];
	
int		hunkcount;

static byte	*membase;
static int		hunkmaxsize;
static int		cursize;

int isKeyboard;

bool shutdown_core = false;

netadr_t	net_local_adr;

extern uint64_t rumble_tick;
int scr_width = 960, scr_height = 544;

void *GetGameAPI (void *import);

qboolean	NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
		return true;
	return false;
}

void NET_Init (void)
{
	
}

void NET_Sleep(int msec)
{
}

#define	LOOPBACK	0x7f000001

#define	MAX_LOOPBACK	4

typedef struct
{
	byte	data[MAX_MSGLEN];
	int		datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t	msgs[MAX_LOOPBACK];
	int			get, send;
} loopback_t;

loopback_t	loopbacks[2];

qboolean	NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return false;

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	memcpy (net_message->data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message->cursize = loop->msgs[i].datalen;
	*net_from = net_local_adr;
	return true;

}


void NET_SendLoopPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock^1];

	i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}

qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b)
{
	if (a.type != b.type)
		return false;

	if (a.type == NA_LOOPBACK)
		return true;

	if (a.type == NA_IP)
	{
		if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3])
			return true;
		return false;
	}

	if (a.type == NA_IPX)
	{
		if ((memcmp(a.ipx, b.ipx, 10) == 0))
			return true;
		return false;
	}
	return false;
}

char	*NET_AdrToString (netadr_t a)
{
	static	char	s[64];
	
	//Com_sprintf (s, sizeof(s), "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));

	return s;
}

qboolean	NET_StringToAdr (char *s, netadr_t *a)
{
	memset (a, 0, sizeof(*a));
	a->type = NA_LOOPBACK;
	return true;
}

void NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to)
{
	if ( to.type == NA_LOOPBACK )
	{
		NET_SendLoopPacket (sock, length, data, to);
		return;
	}
}

void	NET_Config (qboolean multiplayer)
{
}

qboolean	NET_IsLocalAddress (netadr_t adr)
{
	return NET_CompareAdr (adr, net_local_adr);
}

qboolean	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	if (NET_GetLoopPacket (sock, net_from, net_message))
		return true;
	
	return false;
}

void GLimp_Shutdown( void )
{

}

void CDAudio_Play(int track, qboolean looping)
{
}


void CDAudio_Stop(void)
{
}


void CDAudio_Resume(void)
{
}


void CDAudio_Update(void)
{
}


int CDAudio_Init(void)
{
	return 0;
}


void CDAudio_Shutdown(void)
{
}

static void extract_directory(char *buf, const char *path, size_t size)
{
   char *base = NULL;

   strncpy(buf, path, size - 1);
   buf[size - 1] = '\0';

   base = strrchr(buf, '/');
   if (!base)
      base = strrchr(buf, '\\');

   if (base)
      *base = '\0';
   else
    {
       buf[0] = '.';
       buf[1] = '\0';
    }
}

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
	#ifdef DEBUG
	__gnuc_va_list arg;
	int done;
	va_start(arg, format);
	char msg[512];
	done = vsprintf(msg, format, arg);
	va_end(arg);
	int i;
	printf("LOG2FILE: %s\n", msg);
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
}

void Sys_Quit (void)
{
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
	Cbuf_AddText ("lookstrafe \"1.000000\"\n");
	Cbuf_AddText ("lookspring \"0.000000\"\n");
	Cbuf_AddText ("vid_gamma \"0.700000\"\n");
	
}

extern menufield_s s_maxclients_field;
char *targetKeyboard;
void Sys_SetKeys(uint32_t keys, uint32_t state){
	Key_Event(keys, state, Sys_Milliseconds());
}

static void keyboard_cb(bool down, unsigned keycode, uint32_t character, uint16_t mod)
{
	// character-only events are discarded
	if (keycode != RETROK_UNKNOWN) {
		if (down)
			Sys_SetKeys((uint32_t) keycode, 1);
		else
			Sys_SetKeys((uint32_t) keycode, 0);
	}
}

void Sys_SendKeyEvents (void)
{
	int port;

	if (!poll_cb)
		return;

	poll_cb();

	if (!input_cb)
		return;

	for (port = 0; port < MAX_PADS; port++)
	{
		if (!input_cb)
			break;

		switch (quake_devices[port])
		{
		case RETRO_DEVICE_JOYPAD:
		case RETRO_DEVICE_JOYPAD_ALT:
		case RETRO_DEVICE_MODERN:
		{
			unsigned i;
			int16_t ret    = 0;
			if (libretro_supports_bitmasks)
				ret = input_cb(port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
			else
			{
				for (i=RETRO_DEVICE_ID_JOYPAD_B; i <= RETRO_DEVICE_ID_JOYPAD_R3; ++i)
				{
					if (input_cb(port, RETRO_DEVICE_JOYPAD, 0, i))
						ret |= (1 << i);
				}
			}

			for (i=RETRO_DEVICE_ID_JOYPAD_B; 
				i <= RETRO_DEVICE_ID_JOYPAD_R3; ++i)
			{
				if (ret & (1 << i))
					Sys_SetKeys(K_JOY1 + i, 1);
				else
					Sys_SetKeys(K_JOY1 + i, 0);
			}
		}
		break;
		case RETRO_DEVICE_KEYBOARD:
			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT))
				Sys_SetKeys(K_MOUSE1, 1);
			else
				Sys_SetKeys(K_MOUSE1, 0);

			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT))
				Sys_SetKeys(K_MOUSE2, 1);
			else
				Sys_SetKeys(K_MOUSE2, 0);

			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_MIDDLE))
				Sys_SetKeys(K_MOUSE3, 1);
			else
				Sys_SetKeys(K_MOUSE3, 0);

			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELUP))
				Sys_SetKeys(K_MOUSE4, 1);
			else
				Sys_SetKeys(K_MOUSE4, 0);

			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_WHEELDOWN))
				Sys_SetKeys(K_MOUSE5, 1);
			else
				Sys_SetKeys(K_MOUSE5, 0);

			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP))
				Sys_SetKeys(K_MOUSE6, 1);
			else
				Sys_SetKeys(K_MOUSE6, 0);

			if (input_cb(port, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN))
				Sys_SetKeys(K_MOUSE7, 1);
			else
				Sys_SetKeys(K_MOUSE7, 0);

			if (quake_devices[0] == RETRO_DEVICE_KEYBOARD) {
				if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_UP))
					Sys_SetKeys(K_UPARROW, 1);
				else
					Sys_SetKeys(K_UPARROW, 0);

				if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_DOWN))
					Sys_SetKeys(K_DOWNARROW, 1);
				else
					Sys_SetKeys(K_DOWNARROW, 0);

				if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_LEFT))
					Sys_SetKeys(K_LEFTARROW, 1);
				else
					Sys_SetKeys(K_LEFTARROW, 0);

				if (input_cb(port, RETRO_DEVICE_KEYBOARD, 0, RETROK_RIGHT))
					Sys_SetKeys(K_RIGHTARROW, 1);
				else
					Sys_SetKeys(K_RIGHTARROW, 0);
			}
			break;
		case RETRO_DEVICE_NONE:
			break;
		}
	}
	
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

	uint64_t time = cpu_features_get_time_usec() / 1000;
	
	if (!base)
	{
		base = time;
	}

	curtime = (int)(time - base);
	
	return curtime;
}

void Sys_Mkdir (char *path)
{
	path_mkdir(path);
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
static	char	findpath[MAX_OSPATH * 2];
static	char	findpattern[MAX_OSPATH];
static	RDIR	*fdir = NULL;

int glob_match(char *pattern, char *text);

/* Like glob_match, but match PATTERN against any final segment of TEXT.  */
static int glob_match_after_star(char *pattern, char *text)
{
	register char *p = pattern, *t = text;
	register char c, c1;

	while ((c = *p++) == '?' || c == '*')
		if (c == '?' && *t++ == '\0')
			return 0;

	if (c == '\0')
		return 1;

	if (c == '\\')
		c1 = *p;
	else
		c1 = c;

	while (1) {
		if ((c == '[' || *t == c1) && glob_match(p - 1, t))
			return 1;
		if (*t++ == '\0')
			return 0;
	}
}

/* Return nonzero if PATTERN has any special globbing chars in it.  */
static int glob_pattern_p(char *pattern)
{
	register char *p = pattern;
	register char c;
	int open = 0;

	while ((c = *p++) != '\0')
		switch (c) {
		case '?':
		case '*':
			return 1;

		case '[':		/* Only accept an open brace if there is a close */
			open++;		/* brace to match it.  Bracket expressions must be */
			continue;	/* complete, according to Posix.2 */
		case ']':
			if (open)
				return 1;
			continue;

		case '\\':
			if (*p++ == '\0')
				return 0;
		}

	return 0;
}

/* Match the pattern PATTERN against the string TEXT;
   return 1 if it matches, 0 otherwise.
   A match means the entire string TEXT is used up in matching.
   In the pattern string, `*' matches any sequence of characters,
   `?' matches any character, [SET] matches any character in the specified set,
   [!SET] matches any character not in the specified set.
   A set is composed of characters or ranges; a range looks like
   character hyphen character (as in 0-9 or A-Z).
   [0-9a-zA-Z_] is the set of characters allowed in C identifiers.
   Any other character in the pattern must be matched exactly.
   To suppress the special syntactic significance of any of `[]*?!-\',
   and match the character exactly, precede it with a `\'.
*/

int glob_match(char *pattern, char *text)
{
	register char *p = pattern, *t = text;
	register char c;

	while ((c = *p++) != '\0')
		switch (c) {
		case '?':
			if (*t == '\0')
				return 0;
			else
				++t;
			break;

		case '\\':
			if (*p++ != *t++)
				return 0;
			break;

		case '*':
			return glob_match_after_star(p, t);

		case '[':
			{
				register char c1 = *t++;
				int invert;

				if (!c1)
					return (0);

				invert = ((*p == '!') || (*p == '^'));
				if (invert)
					p++;

				c = *p++;
				while (1) {
					register char cstart = c, cend = c;

					if (c == '\\') {
						cstart = *p++;
						cend = cstart;
					}
					if (c == '\0')
						return 0;

					c = *p++;
					if (c == '-' && *p != ']') {
						cend = *p++;
						if (cend == '\\')
							cend = *p++;
						if (cend == '\0')
							return 0;
						c = *p++;
					}
					if (c1 >= cstart && c1 <= cend)
						goto match;
					if (c == ']')
						break;
				}
				if (!invert)
					return 0;
				break;

			  match:
				/* Skip the rest of the [...] construct that already matched.  */
				while (c != ']') {
					if (c == '\0')
						return 0;
					c = *p++;
					if (c == '\0')
						return 0;
					else if (c == '\\')
						++p;
				}
				if (invert)
					return 0;
				break;
			}

		default:
			if (c != *t++)
				return 0;
		}

	return *t == '\0';
}

static qboolean CompareAttributes(char *path, char *name,
	unsigned musthave, unsigned canthave )
{

// . and .. never match
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return false;

	return true;
}

char *Sys_FindFirst (char *path, unsigned musthave, unsigned canhave)
{
	char *p;

	if (fdir >= 0)
		Sys_Error ("Sys_BeginFind without close");

	COM_FilePath (path, findbase);
	strcpy(findbase, path);

	if ((p = strrchr(findbase, '/')) != NULL) {
		*p = 0;
		strcpy(findpattern, p + 1);
	} else
		strcpy(findpattern, "*");

	if (strcmp(findpattern, "*.*") == 0)
		strcpy(findpattern, "*");
	
	fdir = retro_opendir(findbase);
	if (fdir == NULL)
		return NULL;
	while ((retro_readdir(fdir)) > 0) {
		if (!*findpattern || glob_match(findpattern, retro_dirent_get_name(fdir))) {
			//if (*findpattern)
			//	printf("%s matched %s\n", findpattern, d.d_name);
			//if (CompareAttributes(findbase, d.d_name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, retro_dirent_get_name(fdir));
				return findpath;
			//}
		}
	}
	return NULL;
}

char *Sys_FindNext (unsigned musthave, unsigned canhave)
{
	if (fdir < 0)
		return NULL;
	while ((retro_readdir(fdir)) > 0) {
		if (!*findpattern || glob_match(findpattern, retro_dirent_get_name(fdir))) {
			//if (*findpattern)
			//	printf("%s matched %s\n", findpattern, d.d_name);
			//if (CompareAttributes(findbase, d.d_name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, retro_dirent_get_name(fdir));
				return findpath;
			//}
		}
	}
	return NULL;
}

void Sys_FindClose (void)
{
	if (fdir >= 0)
		retro_closedir(fdir);
		
	fdir = NULL;
}

void	Sys_Init (void)
{
}

extern void IN_StopRumble();

//=============================================================================
bool initial_resolution_set = false;
static void update_variables(bool startup)
{
	struct retro_variable var;

	var.key = "vitaquakeii_resolution";
	var.value = NULL;

	if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && !initial_resolution_set)
	{
		char *pch;
		char str[100];
		snprintf(str, sizeof(str), "%s", var.value);

		pch = strtok(str, "x");
		if (pch)
			scr_width = strtoul(pch, NULL, 0);
		pch = strtok(NULL, "x");
		if (pch)
			scr_height = strtoul(pch, NULL, 0);

		if (log_cb)
			log_cb(RETRO_LOG_INFO, "Got size: %u x %u.\n", scr_width, scr_height);

		initial_resolution_set = true;
   }

}

void retro_init(void)
{
   struct retro_log_callback log;

   if(environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;
}

void retro_deinit(void)
{
   libretro_supports_bitmasks = false;
}

static void extract_basename(char *buf, const char *path, size_t size)
{
   char *ext        = NULL;
   const char *base = strrchr(path, '/');
   if (!base)
      base = strrchr(path, '\\');
   if (!base)
      base = path;

   if (*base == '\\' || *base == '/')
      base++;

   strncpy(buf, base, size - 1);
   buf[size - 1] = '\0';

   ext = strrchr(buf, '.');
   if (ext)
      *ext = '\0';
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void gp_layout_set_bind(gp_layout_t gp_layout)
{
   char buf[100];
   unsigned i;
   for (i=0; gp_layout.bind[i].key; ++i)
   {
      snprintf(buf, sizeof(buf), "bind %s \"%s\"\n", gp_layout.bind[i].key,
                                                   gp_layout.bind[i].com);
      Cbuf_AddText(buf);
   }
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   if (port == 0)
   {
      switch (device)
      {
         case RETRO_DEVICE_JOYPAD:
            quake_devices[port] = RETRO_DEVICE_JOYPAD;
            environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, classic.desc);
            gp_layout_set_bind(classic);
            break;
         case RETRO_DEVICE_JOYPAD_ALT:
            quake_devices[port] = RETRO_DEVICE_JOYPAD;
            environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, classic_alt.desc);
            gp_layout_set_bind(classic_alt);
            break;
         case RETRO_DEVICE_MODERN:
            quake_devices[port] = RETRO_DEVICE_MODERN;
            environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, modern.desc);
            gp_layout_set_bind(modern);
            break;
         case RETRO_DEVICE_KEYBOARD:
            quake_devices[port] = RETRO_DEVICE_KEYBOARD;
            break;
         case RETRO_DEVICE_NONE:
         default:
            quake_devices[port] = RETRO_DEVICE_NONE;
            if (log_cb)
               log_cb(RETRO_LOG_ERROR, "[libretro]: Invalid device.\n");
      }
   }
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = "vitaQuakeII";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version  = "v2.1" ;
   info->need_fullpath    = true;
   info->valid_extensions = "pak";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   info->timing.fps            = 60;
   info->timing.sample_rate    = SAMPLE_RATE;

   info->geometry.base_width   = scr_width;
   info->geometry.base_height  = scr_height;
   info->geometry.max_width    = 960;
   info->geometry.max_height   = 544;
   info->geometry.aspect_ratio = (scr_width * 1.0f) / (scr_height * 1.0f);
}

void retro_set_environment(retro_environment_t cb)
{
   static const struct retro_controller_description port_1[] = {
      { "Gamepad Classic", RETRO_DEVICE_JOYPAD },
      { "Gamepad Classic Alt", RETRO_DEVICE_JOYPAD_ALT },
      { "Gamepad Modern", RETRO_DEVICE_MODERN },
      { "Keyboard + Mouse", RETRO_DEVICE_KEYBOARD },
   };

   static const struct retro_controller_info ports[] = {
      { port_1, 3 },
      { 0 },
   };

   environ_cb = cb;

   libretro_set_core_options(environ_cb);
   cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}

void retro_reset(void)
{
}

void retro_set_rumble_strong(void)
{
   uint16_t strength_strong = 0xffff;
   if (!rumble.set_rumble_state)
      return;

   rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, strength_strong);
}

void retro_unset_rumble_strong(void)
{
   if (!rumble.set_rumble_state)
      return;

   rumble.set_rumble_state(0, RETRO_RUMBLE_STRONG, 0);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
   audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

int	ttime, oldtime, newtime;

bool retro_load_game(const struct retro_game_info *info)
{
	enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
	if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
	{
		if (log_cb)
			log_cb(RETRO_LOG_INFO, "XRGB8888 is not supported.\n");
		return false;
	}

	hw_render.context_type    = RETRO_HW_CONTEXT_OPENGL;
	hw_render.context_reset   = context_reset;
	hw_render.context_destroy = context_destroy;
	hw_render.bottom_left_origin = true;
	hw_render.depth = true;
	hw_render.stencil = true;

	if (!environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &hw_render))
	{
		if (log_cb)
			log_cb(RETRO_LOG_ERROR, "vitaQuakeII: libretro frontend doesn't have OpenGL support.\n");
		return false;
	}
	
	int i;
	char *path_lower;
#if defined(_WIN32)
	char slash = '\\';
#else
	char slash = '/';
#endif
	bool use_external_savedir = false;
	const char *base_save_dir = NULL;
	struct retro_keyboard_callback cb = { keyboard_cb };

	if (!info)
		return false;
	
	path_lower = strdup(info->path);
	
	for (i=0; path_lower[i]; ++i)
		path_lower[i] = tolower(path_lower[i]);
	
	environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &cb);
	
	update_variables(true);

	extract_directory(g_rom_dir, info->path, sizeof(g_rom_dir));
	
	snprintf(g_pak_path, sizeof(g_pak_path), "%s", info->path);
	
	if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &base_save_dir) && base_save_dir)
	{
		if (strlen(base_save_dir) > 0)
		{
			// Get game 'name' (i.e. subdirectory)
			char game_name[1024];
			extract_basename(game_name, g_rom_dir, sizeof(game_name));
			
			// > Build final save path
			snprintf(g_save_dir, sizeof(g_save_dir), "%s%c%s", base_save_dir, slash, game_name);
			use_external_savedir = true;
			
			// > Create save directory, if required
			if (!path_is_directory(g_save_dir))
			{
				use_external_savedir = path_mkdir(g_save_dir);
			}
		}
	}
	
	// > Error check
	if (!use_external_savedir)
	{
		// > Use ROM directory fallback...
		snprintf(g_save_dir, sizeof(g_save_dir), "%s", g_rom_dir);
	}
	else
	{
		// > Final check: is the save directory the same as the 'rom' directory?
		//   (i.e. ensure logical behaviour if user has set a bizarre save path...)
		use_external_savedir = (strcmp(g_save_dir, g_rom_dir) != 0);
	}
	
	if (environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble))
		log_cb(RETRO_LOG_INFO, "Rumble environment supported.\n");
	else
		log_cb(RETRO_LOG_INFO, "Rumble environment not supported.\n");
	
	if (strstr(path_lower, "baseq2"))
	{
		extract_directory(g_rom_dir, g_rom_dir, sizeof(g_rom_dir));
	}
	
	oldtime = Sys_Milliseconds ();

	return true;
	
}

static void audio_process(void)
{
}

bool first_boot = true;

void retro_run(void)
{
	qglBindFramebuffer(RARCH_GL_FRAMEBUFFER, hw_render.get_current_framebuffer());
	qglEnable(GL_TEXTURE_2D);
	if (first_boot) {
		const char *argv[32];
		const char *empty_string = "";
	
		argv[0] = empty_string;
		Qcommon_Init(1, argv);
		first_boot = false;
	}
	
	if (rumble_tick != 0) {
		if (cpu_features_get_time_usec() - rumble_tick > 500000) IN_StopRumble(); // 0.5 sec
	}
	
	do {
		newtime = Sys_Milliseconds ();
		ttime = newtime - oldtime;
	} while (ttime < 1);
	
	Qcommon_Frame (ttime);
	oldtime = newtime;

	if (shutdown_core)
		return;

	video_cb(RETRO_HW_FRAME_BUFFER_VALID, scr_width, scr_height, 0);
  
	audio_process();
	audio_callback();
}

void retro_unload_game(void)
{
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
   (void)type;
   (void)info;
   (void)num;
   return false;
}

size_t retro_serialize_size(void)
{
   return 0;
}

bool retro_serialize(void *data_, size_t size)
{
   return false;
}

bool retro_unserialize(const void *data_, size_t size)
{
   return false;
}

void *retro_get_memory_data(unsigned id)
{
   (void)id;
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   (void)id;
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
	printf("retro_cheat_set called\n");
   (void)index;
   (void)enabled;
   (void)code;
}


/* snddma.c */
#include "../client/snd_loc.h"

static volatile int sound_initialized = 0;
static byte *audio_buffer;
static int stop_audio = false;

static unsigned audio_buffer_ptr;

static void audio_callback(void)
{
	unsigned read_first, read_second;
	float samples_per_frame = (2 * SAMPLE_RATE) / 60;
	unsigned read_end = audio_buffer_ptr + samples_per_frame;

	if (read_end > BUFFER_SIZE / 2)
		read_end = BUFFER_SIZE / 2;

	read_first  = read_end - audio_buffer_ptr;
	read_second = samples_per_frame - read_first;

	audio_batch_cb(audio_buffer + audio_buffer_ptr, read_first / (dma.samplebits / 8));
	audio_buffer_ptr += read_first;
	if (read_second >= 1) {
		audio_batch_cb(audio_buffer, read_second / (dma.samplebits / 8));
		audio_buffer_ptr = read_second;
	}
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
	dma.samples = BUFFER_SIZE / 2;
	dma.samplepos = 0;
	dma.submission_chunk = 1;
	dma.buffer = audio_buffer;

	sound_initialized = 1;

	return true;
}

int SNDDMA_GetDMAPos(void)
{
	if(!sound_initialized)
		return 0;
	
	dma.samplepos = audio_buffer_ptr;
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

/* vid.c */

extern viddef_t vid;

#define REF_OPENGL  0

cvar_t *vid_ref;
cvar_t *vid_fullscreen;
cvar_t *vid_gamma;
cvar_t *scr_viewsize;

static cvar_t *sw_mode;
static cvar_t *sw_stipplealpha;
cvar_t *gl_picmip;
cvar_t *gl_mode;
cvar_t *gl_driver;
extern cvar_t *gl_shadows;

extern void M_ForceMenuOff( void );

static menuframework_s  s_opengl_menu;
static menuframework_s *s_current_menu;

static menulist_s       s_mode_list;
static menulist_s       s_ref_list;
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
#ifdef DEBUG
	printf(msg);
#endif
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
#ifdef DEBUG
	printf(msg);
#endif
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
    printf("VID_GetModeInfo %dx%d mode %d\n",*width,*height,mode);

    return true;
}

static void NullCallback( void *unused )
{
}

static void ResCallback( void *unused )
{
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

    if ( strcmp( vid_ref->string, "soft" ) == 0 )
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
	
	static const char *refs[] =
	{
		"openGL",
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

    s_current_menu = &s_opengl_menu;

    /*
    ** draw the banner
    */
    re.DrawGetPicSize( &w, &h, "m_banner_video" );
    re.DrawPic( viddef.width / 2 - w / 2, viddef.height /2 - 110, "m_banner_video" );

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

int GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	int width, height;

	ri.Con_Printf( PRINT_ALL, "Initializing OpenGL display\n");
	ri.Con_Printf (PRINT_ALL, "...setting mode %d:", mode );

	if ( !ri.Vid_GetModeInfo( &width, &height, mode ) )
	{
		ri.Con_Printf( PRINT_ALL, " invalid mode\n" );
		return rserr_invalid_mode;
	}

	ri.Con_Printf( PRINT_ALL, " %d %d\n", width, height );

	// destroy the existing window
	GLimp_Shutdown ();

	*pwidth = width;
	*pheight = height;
	ri.Vid_NewWindow (width, height);

	if (!gl_set) GLimp_InitGL();

	return rserr_ok;
}

/* input.c */

cvar_t *in_joystick;
cvar_t *leftanalog_sensitivity;
cvar_t *rightanalog_sensitivity;
cvar_t *vert_motioncam_sensitivity;
cvar_t *hor_motioncam_sensitivity;
cvar_t *use_gyro;
extern cvar_t *pstv_rumble;
extern cvar_t *gl_xflip;

void IN_Init (void)
{
	in_joystick	= Cvar_Get ("in_joystick", "1",	CVAR_ARCHIVE);
	leftanalog_sensitivity = Cvar_Get ("leftanalog_sensitivity", "2.0", CVAR_ARCHIVE);
	rightanalog_sensitivity = Cvar_Get ("rightanalog_sensitivity", "2.0", CVAR_ARCHIVE);
	vert_motioncam_sensitivity = Cvar_Get ("vert_motioncam_sensitivity", "2.0", CVAR_ARCHIVE);
	hor_motioncam_sensitivity = Cvar_Get ("hor_motioncam_sensitivity", "2.0", CVAR_ARCHIVE);
	use_gyro = Cvar_Get ("use_gyro", "0", CVAR_ARCHIVE);
	pstv_rumble	= Cvar_Get ("pstv_rumble", "1",	CVAR_ARCHIVE);

	int i;
	for (i = 0; i < MAX_PADS; i++)
		quake_devices[i] = RETRO_DEVICE_JOYPAD;
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
}

void IN_Frame (void)
{
}

void IN_StartRumble (void)
{
	if (!pstv_rumble->value) return;

}

void IN_StopRumble (void)
{

}

void IN_Move (usercmd_t *cmd)
{
   static int cur_mx;
   static int cur_my;
   int mx, my, lsx, lsy, rsx, rsy;

   if (quake_devices[0] == RETRO_DEVICE_KEYBOARD) {
      mx = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
      my = input_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

      if (mx != cur_mx || my != cur_my)
      {

         mx *= sensitivity->value;
         my *= sensitivity->value;

         cl.viewangles[YAW] -= m_yaw->value * mx;

         cl.viewangles[PITCH] += m_pitch->value * my;

         if (cl.viewangles[PITCH] > 80)
            cl.viewangles[PITCH] = 80;
         if (cl.viewangles[PITCH] < -70)
            cl.viewangles[PITCH] = -70;
         cur_mx = mx;
         cur_my = my;
      }
   } else if (quake_devices[0] != RETRO_DEVICE_NONE && quake_devices[0] != RETRO_DEVICE_KEYBOARD) {
      // Left stick move
      lsx = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
               RETRO_DEVICE_ID_ANALOG_X);
      lsy = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,
               RETRO_DEVICE_ID_ANALOG_Y);

      if (lsx > analog_deadzone || lsx < -analog_deadzone) {
         if (lsx > analog_deadzone)
            lsx = lsx - analog_deadzone;
         if (lsx < -analog_deadzone)
            lsx = lsx + analog_deadzone;
         cmd->sidemove += cl_sidespeed->value * lsx / (ANALOG_RANGE - analog_deadzone);
      }

      if (lsy > analog_deadzone || lsy < -analog_deadzone) {
         if (lsy > analog_deadzone)
            lsy = lsy - analog_deadzone;
         if (lsy < -analog_deadzone)
            lsy = lsy + analog_deadzone;
         cmd->forwardmove -= cl_forwardspeed->value * lsy / (ANALOG_RANGE - analog_deadzone);
      }

      // Right stick Look
      rsx = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
               RETRO_DEVICE_ID_ANALOG_X);
      rsy = input_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT,
               RETRO_DEVICE_ID_ANALOG_Y);

      if (rsx > analog_deadzone || rsx < -analog_deadzone) {
         if (rsx > analog_deadzone)
            rsx = rsx - analog_deadzone;
         if (rsx < -analog_deadzone)
            rsx = rsx + analog_deadzone;
         // For now we are sharing the sensitivity with the mouse setting
         cl.viewangles[YAW] -= sensitivity->value * rsx / (ANALOG_RANGE - analog_deadzone);
      }

      if (rsy > analog_deadzone || rsy < -analog_deadzone) {
         if (rsy > analog_deadzone)
            rsy = rsy - analog_deadzone;
         if (rsy < -analog_deadzone)
            rsy = rsy + analog_deadzone;
         cl.viewangles[PITCH] -= sensitivity->value * rsy / (ANALOG_RANGE - analog_deadzone);
      }

      if (cl.viewangles[PITCH] > 80)
         cl.viewangles[PITCH] = 80;
      if (cl.viewangles[PITCH] < -70)
         cl.viewangles[PITCH] = -70;
   }
}

void IN_Activate (qboolean active)
{
}

void IN_ActivateMouse (void)
{
}

void IN_DeactivateMouse (void)
{
}

void IN_MouseEvent (int mstate)
{
}

