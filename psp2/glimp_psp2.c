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
** GLimp_SwitchFullscreen
**
*/
#include <vitasdk.h>
#include "vitaGL.h"
#include "../ref_gl/gl_local.h"

static qboolean GLimp_SwitchFullscreen( int width, int height );
qboolean GLimp_InitGL (void);

extern cvar_t *vid_fullscreen;
extern cvar_t *vid_ref;

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
	gl_config.allow_cds = true;
	return true;
}

#define MAX_INDICES 4096
uint16_t* indices;

qboolean GLimp_InitGL (void)
{
    int i;
	indices = (uint16_t*)malloc(sizeof(uint16_t*)*MAX_INDICES);
	for (i=0;i<MAX_INDICES;i++){
		indices[i] = i;
	}
	gVertexBuffer = (float*)malloc(sizeof(float)*VERTEXARRAYSIZE);
	gColorBuffer = (float*)malloc(sizeof(float)*VERTEXARRAYSIZE);
	gTexCoordBuffer = (float*)malloc(sizeof(float)*VERTEXARRAYSIZE);
	return true;
}

/*
** GLimp_BeginFrame
*/
void GLimp_BeginFrame( float camera_separation )
{
	vglStartRendering();
	vglIndexPointer(GL_SHORT, 0, MAX_INDICES, indices);
}

/*
** GLimp_EndFrame
** 
** Responsible for doing a swapbuffers and possibly for other stuff
** as yet to be determined.  Probably better not to make this a GLimp
** function and instead do a call to GLimp_SwapBuffers.
*/
void GLimp_EndFrame (void)
{
	vglStopRendering();
}

/*
** GLimp_AppActivate
*/
void GLimp_AppActivate( qboolean active )
{
	
}
