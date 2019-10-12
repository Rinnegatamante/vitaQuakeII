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

/* draw.c */

typedef struct
{
	unsigned		width, height;			/* coordinates from main game */
} viddef_t;

#include "gl_local.h"

image_t		*draw_chars;

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal (void)
{
	/* load console characters (don't bilerp characters) */
	draw_chars = GL_FindImage ("pics/conchars.pcx", it_pic, false);
	GL_Bind( draw_chars->texnum );
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}


static void DrawQuad(float x, float y, float w, float h, float u, float v, float uw, float vh)
{
   float texcoord[2*4] = {u, v, u + uw, v, u + uw, v + vh, u, v + vh};
   float vertex[3*4] = {x,y,0.5f,x+w,y,0.5f, x+w, y+h,0.5f, x, y+h,0.5f};
   glVertexAttribPointerMapped(0, vertex);
   glVertexAttribPointerMapped(1, texcoord);
   GL_DrawPolygon(GL_TRIANGLE_FAN, 4);
}

static void DrawPic(float x, float y, float w, float h, float u, float v, float u2, float v2)
{
   float texcoord[2*4] = {u, v, u2, v, u2, v2, u, v2};
   float vertex[3*4] = {x,y,0.5f,x+w,y,0.5f, x+w, y+h,0.5f, x, y+h,0.5f};
   glVertexAttribPointerMapped(0, vertex);
   glVertexAttribPointerMapped(1, texcoord);
   GL_DrawPolygon(GL_TRIANGLE_FAN, 4);
}

static void DrawQuad_NoTex(float x, float y, float w, float h, float r, float g, float b, float a)
{
   float vertex[3*4] = {x,y,0.5f,x+w,y,0.5f, x+w, y+h,0.5f, x, y+h,0.5f};
   qglDisableClientState(GL_TEXTURE_COORD_ARRAY);
   qglColor4f(r, g, b, a);
   glVertexAttribPointerMapped(0, vertex);
   GL_DrawPolygon(GL_TRIANGLE_FAN, 4);
   qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char (int x, int y, int num, float factor)
{
	int				row, col;
	float			frow, fcol, size;

	num &= 255;
	
	if ( (num&127) == 32 )
		return;		/* space */

	if (y <= -8)
		return;			/* totally off screen */

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	GL_Bind (draw_chars->texnum);
	
	DrawQuad(x, y, 8 * factor, 8 * factor, fcol, frow, size, size);

}

/*
=============
Draw_FindPic
=============
*/
image_t	*Draw_FindPic (char *name)
{
	image_t *gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "pics/%s.pcx", name);
		gl = GL_FindImage (fullname, it_pic, false);
	}
	else
		gl = GL_FindImage (name+1, it_pic, false);

	return gl;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize (int *w, int *h, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		*w = *h = -1;
		return;
	}
	*w = gl->width;
	*h = gl->height;
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic (int x, int y, int w, int h, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		ri.Con_Printf (PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) && !gl->has_alpha)
		qglDisable(GL_ALPHA_TEST);

	GL_Bind (gl->texnum);
	DrawPic(x, y, w, h, gl->sl, gl->tl, gl->sh, gl->th);

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) && !gl->has_alpha)
		qglEnable(GL_ALPHA_TEST);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, char *pic, float factor)
{
	image_t *gl;

	gl = Draw_FindPic (pic);
	if (!gl)
	{
		ri.Con_Printf (PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) && !gl->has_alpha)
		qglDisable(GL_ALPHA_TEST);

	GL_Bind (gl->texnum);
	DrawPic(x, y, gl->width * factor, gl->height * factor, gl->sl, gl->tl, gl->sh, gl->th);

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) )  && !gl->has_alpha)
		qglEnable(GL_ALPHA_TEST);
}

/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h, char *pic)
{
	image_t	*image;

	image = Draw_FindPic (pic);
	if (!image)
	{
		ri.Con_Printf (PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) )  && !image->has_alpha)
		qglDisable(GL_ALPHA_TEST);

	
	GL_Bind (image->texnum);
	DrawQuad(x, y, w, h, x/64.0, y/64.0, w/64.0, h/64.0);

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) )  && !image->has_alpha)
		qglEnable(GL_ALPHA_TEST);
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	union
	{
		unsigned	c;
		byte		v[4];
	} color;

	if ( (unsigned)c > 255)
		ri.Sys_Error (ERR_FATAL, "Draw_Fill: bad color");

	color.c = d_8to24table[c];
	
	DrawQuad_NoTex(x, y, w, h, (color.v[0]) / 255.0f, (color.v[1]) / 255.0f, (color.v[2]) / 255.0f, 1.0f);
	
	qglColor4f(1,1,1,1);

}

/*============================================================================= */

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen (void)
{
	qglEnable (GL_BLEND);
	DrawQuad_NoTex(0, 0, vid.width, vid.height, 0, 0, 0, 0.8f);
	qglColor4f (1,1,1,1);
	qglDisable (GL_BLEND);
}


/*==================================================================== */


/*
=============
Draw_StretchRaw
=============
*/
extern unsigned	r_rawpalette[256];

void Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data)
{
	unsigned	image32[320*240];
	int			i, j, trows;
	byte		*source;
	int			frac, fracstep;
	float		hscale;
	int			row;
	float		t;
	
	GL_Bind (0);

	if (rows<=256)
	{
		hscale = 1;
		trows = rows;
	}
	else
	{
		hscale = rows/256.0;
		trows = 256;
	}
	t = rows*hscale / 256;

	unsigned *dest;

	for (i=0 ; i<trows ; i++)
	{
		row = (int)(i*hscale);
		if (row > rows)
			break;
		source = data + cols*row;
		dest = &image32[i*256];
		fracstep = cols*0x10000/256;
		frac = fracstep >> 1;
		for (j=0 ; j<256 ; j++)
		{
			dest[j] = r_rawpalette[source[frac>>16]];
			frac += fracstep;
		}
	}

	qglTexImage2D(GL_TEXTURE_2D, 0, gl_tex_solid_format, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32);
	
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#if 0
   if ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) 
#endif
		qglDisable(GL_ALPHA_TEST);

	DrawPic(x, y, w, h, 0, 0, 1, t);

#if 0
	if ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) 
#endif
		qglEnable(GL_ALPHA_TEST);
}

