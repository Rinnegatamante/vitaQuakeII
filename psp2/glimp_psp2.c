/*
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
/*
** GLW_IMP.C
**
** This file contains ALL PSVITA specific stuff having to do with the
** OpenGL refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_Shutdown
**
*/
#include <vitasdk.h>
#include "vitaGL.h"
#include "../ref_gl/gl_local.h"

qboolean GLimp_InitGL (void);

extern uint32_t postfx_shader;
static GLuint main_fb = 0xDEADBEEF, main_fb_tex;
extern GLuint cur_shader[2];
extern int postfx_idx;

extern int isKeyboard;
extern cvar_t *vid_fullscreen;
extern cvar_t *vid_ref;
extern uint8_t is_uma0;
qboolean gl_set = false;
int msaa = 0, postfx = 0, scr_width = 960, scr_height = 544;
float *gVertexBufferPtr;
float *gColorBufferPtr;
float *gTexCoordBufferPtr;

/*
** GLimp_SetMode
*/
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

/*
** GLimp_Shutdown
**
** This routine does all OS specific shutdown procedures for the OpenGL
** subsystem.  Under OpenGL this means NULLing out the current DC and
** HGLRC, deleting the rendering context, and releasing the DC acquired
** for the window.  The state structure is also nulled out.
**
*/
void GLimp_Shutdown( void )
{

}


/*
** GLimp_Init
**
** This routine is responsible for initializing the OS specific portions
** of OpenGL.  Under Win32 this means dealing with the pixelformats and
** doing the wgl interface stuff.
*/
int GLimp_Init( void *hinstance, void *wndproc )
{
	char res_str[64];
	FILE *f = NULL;
	if (is_uma0) f = fopen("uma0:data/quake2/antialiasing.cfg", "rb");
	else f = fopen("ux0:data/quake2/antialiasing.cfg", "rb");
	if (f != NULL){
		fread(res_str, 1, 64, f);
		fclose(f);
		sscanf(res_str, "%d", &msaa);
	}
	
	// Initializing vitaGL
	vglSetVDMBufferSize(1024 * 1024);
	switch (msaa) {
	case 1:
		vglInitExtended(0, scr_width, scr_height, 0x1000000, SCE_GXM_MULTISAMPLE_2X);
		break;
	case 2:
		vglInitExtended(0, scr_width, scr_height, 0x1000000, SCE_GXM_MULTISAMPLE_4X);
		break;
	default:
		vglInitExtended(0, scr_width, scr_height, 0x1000000, SCE_GXM_MULTISAMPLE_NONE);
		break;
	}
	vglUseVram(GL_TRUE);
	
	if (is_uma0) f = fopen("uma0:data/quake2/postfx.cfg", "rb");
	else f = fopen("ux0:data/quake2/postfx.cfg", "rb");
	if (f != NULL){
		fread(res_str, 1, 64, f);
		fclose(f);
		sscanf(res_str, "%d", &postfx);
	}

	gl_config.allow_cds = true;
	return true;
}

#define MAX_INDICES 4096
uint16_t* indices;

GLuint fs[9];
GLuint vs[4];
GLuint programs[9];

void GL_LoadShader(const char* filename, GLuint idx, GLboolean fragment){
	FILE* f = fopen(filename, "rb");
	fseek(f, 0, SEEK_END);
	long int size = ftell(f);
	fseek(f, 0, SEEK_SET);
	void* res = malloc(size);
	fread(res, 1, size, f);
	fclose(f);
	if (fragment) glShaderBinary(1, &fs[idx], 0, res, size);
	else glShaderBinary(1, &vs[idx], 0, res, size);
	free(res);
}

int state_mask = 0;
GLint monocolor;
GLint modulcolor[2];

void GL_SetProgram(){
	switch (state_mask){
		case 0x00: // Everything off
		case 0x04: // Modulate
		case 0x08: // Alpha Test
		case 0x0C: // Alpha Test + Modulate
			glUseProgram(programs[NO_COLOR]);
			break;
		case 0x01: // Texcoord
		case 0x03: // Replace + Texcoord + Color
			glUseProgram(programs[TEX2D_REPL]);
			break;
		case 0x02: // Color
		case 0x06: // Color + Modulate
			glUseProgram(programs[RGBA_COLOR]);
			break;
		case 0x05: // Modulate + Texcoord
			glUseProgram(programs[TEX2D_MODUL]);
			break;
		case 0x07: // Modulate + Texcoord + Color
			glUseProgram(programs[TEX2D_MODUL_CLR]);
			break;
		case 0x09: // Alpha Test + Texcoord
		case 0x0B: // Alpha Test + Color + Texcoord
			glUseProgram(programs[TEX2D_REPL_A]);
			break;
		case 0x0A: // Alpha Test + Color
		case 0x0E: // Alpha Test + Modulate + Color
			glUseProgram(programs[RGBA_CLR_A]);
			break;
		case 0x0D: // Alpha Test + Modulate + Texcoord
			glUseProgram(programs[TEX2D_MODUL_A]);
			break;
		case 0x0F: // Alpha Test + Modulate + Texcood + Color
			glUseProgram(programs[FULL_A]);
			break;
		default:
			break;
	}
}

void GL_EnableState(GLenum state){	
	switch (state){
		case GL_TEXTURE_COORD_ARRAY:
			state_mask |= 0x01;
			break;
		case GL_COLOR_ARRAY:
			state_mask |= 0x02;
			break;
		case GL_MODULATE:
			state_mask |= 0x04;
			break;
		case GL_REPLACE:
			state_mask &= ~0x04;
			break;
		case GL_ALPHA_TEST:
			state_mask |= 0x08;
			break;
	}
	GL_SetProgram();
}

void GL_DisableState(GLenum state){	
	switch (state){
		case GL_TEXTURE_COORD_ARRAY:
			state_mask &= ~0x01;
			break;
		case GL_COLOR_ARRAY:
			state_mask &= ~0x02;
			break;
		case GL_ALPHA_TEST:
			state_mask &= ~0x08;
			break;
		default:
			break;
	}
	GL_SetProgram();
}

float cur_clr[4];

void GL_DrawPolygon(GLenum prim, int num){
	if (state_mask == 0x05) glUniform4fv(modulcolor[0], 1, cur_clr);
	else if (state_mask == 0x0D) glUniform4fv(modulcolor[1], 1, cur_clr);
	vglDrawObjects(prim, num, GL_TRUE);
}

void GL_Color(float r, float g, float b, float a){
	cur_clr[0] = r;
	cur_clr[1] = g;
	cur_clr[2] = b;
	cur_clr[3] = a;
}

qboolean reset_shaders = false;
qboolean shaders_set = false;
void GL_ResetShaders(){
	glFinish();
	int i;
	if (shaders_set){
		for (i=0;i<9;i++){
			glDeleteProgram(programs[i]);
		}
		for (i=0;i<9;i++){
			glDeleteShader(fs[i]);
		}
		for (i=0;i<4;i++){
			glDeleteShader(vs[i]);
		}
	}else shaders_set = true;

	// Loading shaders
	for (i=0;i<9;i++){
		fs[i] = glCreateShader(GL_FRAGMENT_SHADER);
	}
	for (i=0;i<4;i++){
		vs[i] = glCreateShader(GL_VERTEX_SHADER);
	}

	GL_LoadShader("app0:shaders/modulate_f.gxp", MODULATE, GL_TRUE);
	GL_LoadShader("app0:shaders/modulate_rgba_f.gxp", MODULATE_WITH_COLOR, GL_TRUE);
	GL_LoadShader("app0:shaders/replace_f.gxp", REPLACE, GL_TRUE);
	GL_LoadShader("app0:shaders/modulate_alpha_f.gxp", MODULATE_A, GL_TRUE);
	GL_LoadShader("app0:shaders/modulate_rgba_alpha_f.gxp", MODULATE_COLOR_A, GL_TRUE);
	GL_LoadShader("app0:shaders/replace_alpha_f.gxp", REPLACE_A, GL_TRUE);
	GL_LoadShader("app0:shaders/texture2d_v.gxp", TEXTURE2D, GL_FALSE);
	GL_LoadShader("app0:shaders/texture2d_rgba_v.gxp", TEXTURE2D_WITH_COLOR, GL_FALSE);

	GL_LoadShader("app0:shaders/rgba_f.gxp", RGBA_COLOR, GL_TRUE);
	GL_LoadShader("app0:shaders/vertex_f.gxp", MONO_COLOR, GL_TRUE);
	GL_LoadShader("app0:shaders/rgba_alpha_f.gxp", RGBA_A, GL_TRUE);
	GL_LoadShader("app0:shaders/rgba_v.gxp", COLOR, GL_FALSE);
	GL_LoadShader("app0:shaders/vertex_v.gxp", VERTEX_ONLY, GL_FALSE);

	// Setting up programs
	for (i=0;i<9;i++){
		programs[i] = glCreateProgram();
		switch (i){
			case TEX2D_REPL:
				glAttachShader(programs[i], fs[REPLACE]);
				glAttachShader(programs[i], vs[TEXTURE2D]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				glLinkProgram(programs[i]);
				break;
			case TEX2D_MODUL:
				glAttachShader(programs[i], fs[MODULATE]);
				glAttachShader(programs[i], vs[TEXTURE2D]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				glLinkProgram(programs[i]);
				modulcolor[0] = glGetUniformLocation(programs[i], "vColor");
				break;
			case TEX2D_MODUL_CLR:
				glAttachShader(programs[i], fs[MODULATE_WITH_COLOR]);
				glAttachShader(programs[i], vs[TEXTURE2D_WITH_COLOR]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				vglBindAttribLocation(programs[i], 2, "color", 4, GL_FLOAT);
				glLinkProgram(programs[i]);
				break;
			case RGBA_COLOR:
				glAttachShader(programs[i], fs[RGBA_COLOR]);
				glAttachShader(programs[i], vs[COLOR]);
				vglBindAttribLocation(programs[i], 0, "aPosition", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "aColor", 4, GL_FLOAT);
				glLinkProgram(programs[i]);
				break;
			case NO_COLOR:
				glAttachShader(programs[i], fs[MONO_COLOR]);
				glAttachShader(programs[i], vs[VERTEX_ONLY]);
				vglBindAttribLocation(programs[i], 0, "aPosition", 3, GL_FLOAT);
				glLinkProgram(programs[i]);
				monocolor = glGetUniformLocation(programs[i], "color");
				break;
			case TEX2D_REPL_A:
				glAttachShader(programs[i], fs[REPLACE_A]);
				glAttachShader(programs[i], vs[TEXTURE2D]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				glLinkProgram(programs[i]);
				break;
			case TEX2D_MODUL_A:
				glAttachShader(programs[i], fs[MODULATE_A]);
				glAttachShader(programs[i], vs[TEXTURE2D]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				glLinkProgram(programs[i]);
				modulcolor[1] = glGetUniformLocation(programs[i], "vColor");
				break;
			case FULL_A:
				glAttachShader(programs[i], fs[MODULATE_COLOR_A]);
				glAttachShader(programs[i], vs[TEXTURE2D_WITH_COLOR]);
				vglBindAttribLocation(programs[i], 0, "position", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "texcoord", 2, GL_FLOAT);
				vglBindAttribLocation(programs[i], 2, "color", 4, GL_FLOAT);
				glLinkProgram(programs[i]);
				break;
			case RGBA_CLR_A:
				glAttachShader(programs[i], fs[RGBA_A]);
				glAttachShader(programs[i], vs[COLOR]);
				vglBindAttribLocation(programs[i], 0, "aPosition", 3, GL_FLOAT);
				vglBindAttribLocation(programs[i], 1, "aColor", 4, GL_FLOAT);
				glLinkProgram(programs[i]);
				break;
		}
	}
}

float *fx_vertices;
float *fx_texcoords;

qboolean GLimp_InitGL (void)
{
	GL_ResetShaders();
    int i;
	indices = (uint16_t*)malloc(sizeof(uint16_t)*MAX_INDICES);
	for (i=0;i<MAX_INDICES;i++){
		indices[i] = i;
	}
	gVertexBufferPtr = (float*)malloc(0x400000);
	gColorBufferPtr = (float*)malloc(0x200000);
	gTexCoordBufferPtr = (float*)malloc(0x200000);
	
	fx_vertices = (float*)malloc(12 * sizeof(float));
	fx_texcoords = (float*)malloc(8 * sizeof(float));
	fx_vertices[0] =   0.0f;
	fx_vertices[1] =   0.0f;
	fx_vertices[2] =   0.0f;
	fx_vertices[3] = 960.0f;
	fx_vertices[4] =   0.0f;
	fx_vertices[5] =   0.0f;
	fx_vertices[6] = 960.0f;
	fx_vertices[7] = 544.0f;
	fx_vertices[8] =   0.0f;
	fx_vertices[9] =   0.0f;
	fx_vertices[10]= 544.0f;
	fx_vertices[11]=   0.0f;
	fx_texcoords[0] = 0.0f;
	fx_texcoords[1] = 0.0f;
	fx_texcoords[2] = 1.0f;
	fx_texcoords[3] = 0.0f;
	fx_texcoords[4] = 1.0f;
	fx_texcoords[5] = 1.0f;
	fx_texcoords[6] = 0.0f;
	fx_texcoords[7] = 1.0f;
	
	vglIndexPointerMapped(indices);
	gVertexBuffer = gVertexBufferPtr;
	gColorBuffer = gColorBufferPtr;
	gTexCoordBuffer = gTexCoordBufferPtr;
	
	gl_set = true;
	return true;
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame( float camera_separation )
{
	if (postfx_shader != 0) {
		if (main_fb == 0xDEADBEEF) {
			glGenTextures(1, &main_fb_tex);
			glBindTexture(GL_TEXTURE_2D, main_fb_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scr_width, scr_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			glGenFramebuffers(1, &main_fb);
			glBindFramebuffer(GL_FRAMEBUFFER, main_fb);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, main_fb_tex, 0);
		} else glBindFramebuffer(GL_FRAMEBUFFER, main_fb);
	} else glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*
** GLimp_EndFrame
**
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
extern float *fx_vertices;
extern float *fx_texcoords;
void GLimp_EndFrame (void)
{
	if (postfx_shader != 0) {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, main_fb_tex);
		glUseProgram(cur_shader[postfx_idx]);
		vglVertexAttribPointerMapped(0, fx_vertices);
		vglVertexAttribPointerMapped(1, fx_texcoords);
		vglDrawObjects(GL_TRIANGLE_FAN, 4, true);
	}
	vglSwapBuffers(isKeyboard);
	vglIndexPointerMapped(indices);
	gVertexBuffer = gVertexBufferPtr;
	gColorBuffer = gColorBufferPtr;
	gTexCoordBuffer = gTexCoordBufferPtr;
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate( qboolean active )
{

}
