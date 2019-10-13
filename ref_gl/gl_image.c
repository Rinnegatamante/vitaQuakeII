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

typedef struct
{
	unsigned		width, height;			/* coordinates from main game */
} viddef_t;

#include "gl_local.h"
#include <stdlib.h>
#include <jpeglib.h>
#include "../ref_common/r_image_common.h"

image_t		gltextures[MAX_GLTEXTURES];
int			numgltextures = 0;
int			base_textureid;		/* gltextures[i] = base_textureid+i */

static byte			 intensitytable[256];
static unsigned char gammatable[256];

cvar_t		*intensity;

unsigned	d_8to24table[256];

/* forward declarations */
static void GL_LightScaleTexture (uint32_t *in, int inwidth,
      int inheight, qboolean only_gamma );

/*
===============
GL_Upload32

Returns has_alpha
===============
*/
int		upload_width, upload_height;
qboolean uploaded_paletted;
uint32_t	trans[512*256];
uint32_t	scaled[1024*1024];

qboolean GL_Upload32 (uint32_t *data, int width, int height,  qboolean mipmap)
{
#ifdef DEBUG
	printf("GL_Upload32\n");
#endif
	int			samples;
	
	int			scaled_width, scaled_height;
	int			i, c;
	byte		*scan;
	int comp;

	uploaded_paletted = false;

	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	if (gl_round_down->value && scaled_width > width && mipmap)
		scaled_width >>= 1;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;
	if (gl_round_down->value && scaled_height > height && mipmap)
		scaled_height >>= 1;

	/* let people sample down the world textures for speed */
	if (mipmap)
	{
		scaled_width >>= (int)gl_picmip->value;
		scaled_height >>= (int)gl_picmip->value;
	}

	/* don't ever bother with >1024 textures */
	if (scaled_width > 1024)
		scaled_width = 1024;
	if (scaled_height > 1024)
		scaled_height = 1024;

	if (scaled_width < 1)
		scaled_width = 1;
	if (scaled_height < 1)
		scaled_height = 1;
	
	upload_width = scaled_width;
	upload_height = scaled_height;

	if (scaled_width * scaled_height > sizeof(scaled)/4)
		ri.Sys_Error (ERR_DROP, "GL_Upload32: too big");

	/* scan the texture for any non-255 alpha */
	c = width*height;
	scan = ((byte *)data) + 3;
	samples = gl_solid_format;
	for (i=0 ; i<c ; i++, scan += 4)
	{
		if ( *scan != 255 )
		{
			samples = gl_alpha_format;
			break;
		}
	}

	if (samples == gl_solid_format)
	    comp = gl_tex_solid_format;
	else if (samples == gl_alpha_format)
	    comp = gl_tex_alpha_format;
	else {
	    ri.Con_Printf (PRINT_ALL,
			   "Unknown number of texture components %i\n",
			   samples);
	    comp = samples;
	}

	if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
			qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			goto done;
		}
		memcpy (scaled, data, width*height*4);
	}
	else
		GL_ResampleTexture (data, width, height, scaled, scaled_width, scaled_height);

	GL_LightScaleTexture (scaled, scaled_width, scaled_height, !mipmap );

	qglTexImage2D( GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled );

done:
	if (mipmap)
	{
		qglGenerateMipmap(GL_TEXTURE_2D);
	}
   qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
   qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	return (samples == gl_alpha_format);
}

qboolean GL_Upload8 (byte *data, int width, int height,  qboolean mipmap, qboolean is_sky )
{
#ifdef DEBUG
	printf("GL_Upload8\n");
#endif
	
	int			i, s;
	int			p;

	s = width*height;

	if (s > sizeof(trans)/4)
		ri.Sys_Error (ERR_DROP, "GL_Upload8: too large");
	
	for (i=0 ; i<s ; i++)
	{
		p = data[i];
		trans[i] = d_8to24table[p];

		if (p == 255)
		{
         /* transparent, so scan around for another color
			 * to avoid alpha fringes
			 * FIXME: do a full flood fill so mips work...
          */
			if (i > width && data[i-width] != 255)
				p = data[i-width];
			else if (i < s-width && data[i+width] != 255)
				p = data[i+width];
			else if (i > 0 && data[i-1] != 255)
				p = data[i-1];
			else if (i < s-1 && data[i+1] != 255)
				p = data[i+1];
			else
				p = 0;
			/* copy RGB components */
			((byte *)&trans[i])[0] = ((byte *)&d_8to24table[p])[0];
			((byte *)&trans[i])[1] = ((byte *)&d_8to24table[p])[1];
			((byte *)&trans[i])[2] = ((byte *)&d_8to24table[p])[2];
		}
	}

	return GL_Upload32 (trans, width, height, mipmap);

}

int		gl_solid_format = GL_RGB;
int		gl_alpha_format = GL_RGBA;

int		gl_tex_solid_format = GL_RGB;
int		gl_tex_alpha_format = GL_RGBA;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

void GL_SetTexturePalette( unsigned palette[256] )
{
	int i;
	unsigned char temptable[768];

	for ( i = 0; i < 256; i++ )
	{
		temptable[i*3+0] = ( palette[i] >> 0 ) & 0xff;
		temptable[i*3+1] = ( palette[i] >> 8 ) & 0xff;
		temptable[i*3+2] = ( palette[i] >> 16 ) & 0xff;
	}

}

void GL_TexEnv( GLenum mode )
{
	static int lastmodes[2] = { -1, -1 };

	if ( mode != lastmodes[gl_state.currenttmu] )
	{
		qglTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		lastmodes[gl_state.currenttmu] = mode;
	}
}

extern	image_t	*draw_chars;

void GL_Bind (int texnum)
{
   if (gl_nobind->value && draw_chars)		/* performance evaluation option */
      texnum = draw_chars->texnum;
   if ( gl_state.currenttextures[gl_state.currenttmu] == texnum)
      return;
   gl_state.currenttextures[gl_state.currenttmu] = texnum;
   qglBindTexture (GL_TEXTURE_2D, texnum);
}

void GL_MBind( GLenum target, int texnum )
{
	if ( target == GL_TEXTURE0_SGIS )
	{
		if ( gl_state.currenttextures[0] == texnum )
			return;
	}
	else
	{
		if ( gl_state.currenttextures[1] == texnum )
			return;
	}
	GL_Bind( texnum );
}

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

typedef struct
{
	char *name;
	int mode;
} gltmode_t;

gltmode_t gl_alpha_modes[] = {
	{"default", 4},
	{"GL_RGBA", GL_RGBA}
};

#define NUM_GL_ALPHA_MODES (sizeof(gl_alpha_modes) / sizeof (gltmode_t))

gltmode_t gl_solid_modes[] = {
	{"default", 3},
	{"GL_RGB", GL_RGB}
};

#define NUM_GL_SOLID_MODES (sizeof(gl_solid_modes) / sizeof (gltmode_t))

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( char *string )
{
	int		i;
	image_t	*glt;

	for (i=0 ; i< NUM_GL_MODES ; i++)
	{
		if ( !Q_stricmp( modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_MODES)
	{
		ri.Con_Printf (PRINT_ALL, "bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	/* change all the existing mipmap texture objects */
	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (glt->type != it_pic && glt->type != it_sky )
		{
			GL_Bind (glt->texnum);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
GL_TextureAlphaMode
===============
*/
void GL_TextureAlphaMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_ALPHA_MODES ; i++)
	{
		if ( !Q_stricmp( gl_alpha_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_ALPHA_MODES)
	{
		ri.Con_Printf (PRINT_ALL, "bad alpha texture mode name\n");
		return;
	}

	gl_tex_alpha_format = gl_alpha_modes[i].mode;
}

/*
===============
GL_TextureSolidMode
===============
*/
void GL_TextureSolidMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_SOLID_MODES ; i++)
	{
		if ( !Q_stricmp( gl_solid_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_SOLID_MODES)
	{
		ri.Con_Printf (PRINT_ALL, "bad solid texture mode name\n");
		return;
	}

	gl_tex_solid_format = gl_solid_modes[i].mode;
}

/*
===============
GL_ImageList_f
===============
*/
void	GL_ImageList_f (void)
{
	int		i;
	image_t	*image;
	int		texels;
	const char *palstrings[2] =
	{
		"RGB",
		"PAL"
	};

	ri.Con_Printf (PRINT_ALL, "------------------\n");
	texels = 0;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->texnum <= 0)
			continue;
		texels += image->upload_width*image->upload_height;
		switch (image->type)
		{
		case it_skin:
			ri.Con_Printf (PRINT_ALL, "M");
			break;
		case it_sprite:
			ri.Con_Printf (PRINT_ALL, "S");
			break;
		case it_wall:
			ri.Con_Printf (PRINT_ALL, "W");
			break;
		case it_pic:
			ri.Con_Printf (PRINT_ALL, "P");
			break;
		default:
			ri.Con_Printf (PRINT_ALL, " ");
			break;
		}

		ri.Con_Printf (PRINT_ALL,  " %3i %3i %s: %s\n",
			image->upload_width, image->upload_height, palstrings[image->paletted], image->name);
	}
	ri.Con_Printf (PRINT_ALL, "Total texel count (not counting mipmaps): %i\n", texels);
}

/*
====================================================================

IMAGE FLOOD FILLING

====================================================================
*/


/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes
=================
*/

typedef struct
{
	short		x, y;
} floodfill_t;

/* must be a power of 2 */
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

void R_FloodFillSkin( byte *skin, int skinwidth, int skinheight )
{
	byte				fillcolor = *skin; /* assume this is the pixel to fill */
	floodfill_t			fifo[FLOODFILL_FIFO_SIZE];
	int					inpt = 0, outpt = 0;
	int					filledcolor = -1;
	int					i;

	if (filledcolor == -1)
	{
		filledcolor = 0;
		/* attempt to find opaque black */
		for (i = 0; i < 256; ++i)
			if (d_8to24table[i] == (255 << 0)) /* alpha 1.0 */
			{
				filledcolor = i;
				break;
			}
	}

	/* can't fill to filled color or to transparent color (used as visited marker) */
	if ((fillcolor == filledcolor) || (fillcolor == 255))
	{
		/*printf( "not filling skin from %d to %d\n", fillcolor, filledcolor ); */
		return;
	}

	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt)
	{
		int			x = fifo[outpt].x, y = fifo[outpt].y;
		int			fdc = filledcolor;
		byte		*pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)				FLOODFILL_STEP( -1, -1, 0 );
		if (x < skinwidth - 1)	FLOODFILL_STEP( 1, 1, 0 );
		if (y > 0)				FLOODFILL_STEP( -skinwidth, 0, -1 );
		if (y < skinheight - 1)	FLOODFILL_STEP( skinwidth, 0, 1 );
		skin[x + skinwidth * y] = fdc;
	}
}

/*======================================================= */

/*
================
GL_ResampleTextureLerpLine
from DarkPlaces
================
*/

void GL_ResampleTextureLerpLine (byte *in, byte *out, int inwidth, int outwidth) 
{ 
	int j, xi, oldx = 0, f, fstep, l1, l2, endx;

	fstep = (int) (inwidth*65536.0f/outwidth); 
	endx = (inwidth-1); 
	for (j = 0,f = 0;j < outwidth;j++, f += fstep) 
	{ 
		xi = (int) f >> 16; 
		if (xi != oldx) 
		{ 
			in += (xi - oldx) * 4; 
			oldx = xi; 
		} 
		if (xi < endx) 
		{ 
			l2 = f & 0xFFFF; 
			l1 = 0x10000 - l2; 
			*out++ = (byte) ((in[0] * l1 + in[4] * l2) >> 16);
			*out++ = (byte) ((in[1] * l1 + in[5] * l2) >> 16); 
			*out++ = (byte) ((in[2] * l1 + in[6] * l2) >> 16); 
			*out++ = (byte) ((in[3] * l1 + in[7] * l2) >> 16); 
		} 
		else /* last pixel of the line has no pixel to lerp to  */
		{ 
			*out++ = in[0]; 
			*out++ = in[1]; 
			*out++ = in[2]; 
			*out++ = in[3]; 
		} 
	} 
}

/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture (uint32_t *indata, int inwidth, int inheight, uint32_t *outdata, int outwidth, int outheight) 
{ 
	int i, j, yi, oldy, f, fstep, l1, l2, endy = (inheight-1);
	
	byte *inrow, *out, *row1, *row2; 
	out = (byte*)outdata; 
	fstep = (int) (inheight*65536.0f/outheight); 

	row1 = malloc(outwidth*4); 
	row2 = malloc(outwidth*4); 
	inrow = (byte*)indata; 
	oldy = 0; 
	GL_ResampleTextureLerpLine (inrow, row1, inwidth, outwidth); 
	GL_ResampleTextureLerpLine (inrow + inwidth*4, row2, inwidth, outwidth); 
	for (i = 0, f = 0;i < outheight;i++,f += fstep) 
	{ 
		yi = f >> 16; 
		if (yi != oldy) 
		{ 
			inrow = (byte *)indata + inwidth*4*yi; 
			if (yi == oldy+1) 
				memcpy(row1, row2, outwidth*4); 
			else 
				GL_ResampleTextureLerpLine (inrow, row1, inwidth, outwidth);

			if (yi < endy) 
				GL_ResampleTextureLerpLine (inrow + inwidth*4, row2, inwidth, outwidth); 
			else 
				memcpy(row2, row1, outwidth*4); 
			oldy = yi; 
		} 
		if (yi < endy) 
		{ 
			l2 = f & 0xFFFF; 
			l1 = 0x10000 - l2; 
			for (j = 0;j < outwidth;j++) 
			{ 
				*out++ = (byte) ((*row1++ * l1 + *row2++ * l2) >> 16); 
				*out++ = (byte) ((*row1++ * l1 + *row2++ * l2) >> 16); 
				*out++ = (byte) ((*row1++ * l1 + *row2++ * l2) >> 16); 
				*out++ = (byte) ((*row1++ * l1 + *row2++ * l2) >> 16); 
			} 
			row1 -= outwidth*4; 
			row2 -= outwidth*4; 
		} 
		else /* last line has no pixels to lerp to */
		{ 
			for (j = 0;j < outwidth;j++) 
			{ 
				*out++ = *row1++; 
				*out++ = *row1++; 
				*out++ = *row1++; 
				*out++ = *row1++; 
			} 
			row1 -= outwidth*4; 
		} 
	} 
	free(row1); 
	free(row2); 
}

/*
================
GL_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
static void GL_LightScaleTexture (uint32_t *in, int inwidth,
      int inheight, qboolean only_gamma )
{
	if ( only_gamma )
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;
		for (i=0 ; i<c ; i++, p+=4)
		{
			p[0] = gammatable[p[0]];
			p[1] = gammatable[p[1]];
			p[2] = gammatable[p[2]];
		}
	}
	else
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;
		for (i=0 ; i<c ; i++, p+=4)
		{
			p[0] = gammatable[intensitytable[p[0]]];
			p[1] = gammatable[intensitytable[p[1]]];
			p[2] = gammatable[intensitytable[p[2]]];
		}
	}
}

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap (byte *in, int width, int height)
{
	int		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0 ; j<width ; j+=8, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
		}
	}
}

/*
================
GL_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits)
{
	image_t		*image;
	int			i;
	
	/* find a free image_t */
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!image->texnum)
			break;
	}
	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			ri.Sys_Error (ERR_DROP, "MAX_GLTEXTURES");
		numgltextures++;
	}
	image = &gltextures[i];
	
	if (strlen(name) >= sizeof(image->name))
		ri.Sys_Error (ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);
	strcpy (image->name, name);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	if (type == it_skin && bits == 8)
		R_FloodFillSkin(pic, width, height);

	/* load little pics into the scrap */
	{
		image->texnum = TEXNUM_IMAGES + (image - gltextures);
		GL_Bind(image->texnum);
		if (bits == 8) {
			image->has_alpha = GL_Upload8 (pic, width, height, (image->type != it_pic) && (image->type != it_sky), image->type == it_sky );
		} else {
			image->has_alpha = GL_Upload32 ((uint32_t*)pic, width, height, (image->type != it_pic) && (image->type != it_sky) );	
		}
		image->upload_width = upload_width;		/* after power of 2 and scales */
		image->upload_height = upload_height;
		image->paletted = uploaded_paletted;
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;
	}

	return image;
}


/*
================
GL_LoadWal
================
*/
image_t *GL_LoadWal (char *name)
{
	miptex_t	*mt;
	int			width, height, ofs;
	image_t		*image;

	ri.FS_LoadFile (name, (void **)&mt);
	if (!mt)
	{
		ri.Con_Printf (PRINT_ALL, "GL_FindImage: can't load %s\n", name);
		return r_notexture;
	}

	width = LittleLong (mt->width);
	height = LittleLong (mt->height);
	ofs = LittleLong (mt->offsets[0]);

	image = GL_LoadPic (name, (byte *)mt + ofs, width, height, it_wall, 8);

	ri.FS_FreeFile ((void *)mt);

	return image;
}

/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t	*GL_FindImage (char *name, imagetype_t type, bool force)
{
   image_t	*image;
   int		i, len;
   byte	*pic, *palette;
   int		width, height;
   char	s[128];

   if (!name)
      return NULL;	/*	ri.Sys_Error (ERR_DROP, "GL_FindImage: NULL name"); */
   len = strlen(name);
   if (len<5)
      return NULL;	/*	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad name: %s", name);
      */
   if (!force)
   {
      /* look for it */
      for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
      {
         if (!strcmp(name, image->name))
         {
            image->registration_sequence = registration_sequence;
            return image;
         }
      }
   }

   /*
    * load the pic from disk
    */
   pic = NULL;
   palette = NULL;
   if (!strcmp(name+len-4, ".tga"))
   {
      LoadTGA (name, &pic, &width, &height);
      if (!pic)
         return NULL; /* ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name); */
      image = GL_LoadPic (name, pic, width, height, type, 32);
   }
   else if (!strcmp(name+len-4, ".jpg"))
   {
      LoadJPG(name, &pic, &width, &height);
      if (!pic)
         return NULL; /* ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name); */
      image = GL_LoadPic (name, pic, width, height, type, 32);
   }
   else if (!strcmp(name+len-4, ".pcx") || !strcmp(name+len-4, ".wal"))
   {
      strncpy (s, name, sizeof(s));
      s[len-3]='t'; s[len-2]='g'; s[len-1]='a';
      image = GL_FindImage(s,type,false);
      if (image) 
         return image;
      strncpy (s, name, sizeof(s));
      s[len-3]='j'; s[len-2]='p'; s[len-1]='g';
      image = GL_FindImage(s,type,false);
      if (image)
         return image;
      if (!strcmp(name+len-4, ".pcx")) {
         LoadPCX (name, &pic, &palette, &width, &height);
         if (!pic)
            return NULL; /* ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name); */
         image = GL_LoadPic (name, pic, width, height, type, 8);
      } else if (!strcmp(name+len-4, ".wal")) {
         image = GL_LoadWal (name);
      }
   }
   else
      return NULL; /*	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad extension on: %s", name); */

   if (pic)
      free(pic);
   if (palette)
      free(palette);

   return image;
}



/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin (char *name)
{
	return GL_FindImage (name, it_skin,false);
}


/*
================
GL_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void GL_FreeUnusedImages (void)
{
	int		i;
	image_t	*image;

	/* never free r_notexture or particle texture */
	r_notexture->registration_sequence = registration_sequence;
	r_particletexture->registration_sequence = registration_sequence;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		/* used this sequence */
		if (!image->registration_sequence)
			continue;		/* free image_t slot */
		if (image->type == it_pic)
			continue;		/* don't free pics */
		/* free it */
		qglDeleteTextures (1, (uint32_t*)&image->texnum);
		memset (image, 0, sizeof(*image));
	}
}


/*
===============
Draw_GetPalette
===============
*/
int Draw_GetPalette (void)
{
	int		i;
	int		r, g, b;
	unsigned	v;
	byte	*pic, *pal;
	int		width, height;

	/* get the palette */

	LoadPCX ("pics/colormap.pcx", &pic, &pal, &width, &height);
	if (!pal)
		ri.Sys_Error (ERR_FATAL, "Couldn't load pics/colormap.pcx");

	for (i=0 ; i<256 ; i++)
	{
		r = pal[i*3+0];
		g = pal[i*3+1];
		b = pal[i*3+2];
		
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		d_8to24table[i] = LittleLong(v);
	}

	d_8to24table[255] &= LittleLong(0xffffff);	/* 255 is transparent */

	free (pic);
	free (pal);

	return 0;
}


/*
===============
GL_InitImages
===============
*/
void	GL_InitImages (void)
{
	int		i, j;
	float	g = vid_gamma->value;

	registration_sequence = 1;

	/* init intensity conversions */
	intensity = ri.Cvar_Get ("intensity", "2", 0);

	if ( intensity->value <= 1 )
		ri.Cvar_Set( "intensity", "1" );

	gl_state.inverse_intensity = 1 / intensity->value;

	Draw_GetPalette ();

	for ( i = 0; i < 256; i++ )
	{
		if ( g == 1 )
		{
			gammatable[i] = i;
		}
		else
		{
			float inf;

			inf = 255 * pow ( (i+0.5)/255.5 , g ) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

	for (i=0 ; i<256 ; i++)
	{
		j = i*intensity->value;
		if (j > 255)
			j = 255;
		intensitytable[i] = j;
	}
}

/*
===============
GL_ShutdownImages
===============
*/
void	GL_ShutdownImages (void)
{
	int		i;
	image_t	*image;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		/* free image_t slot */
		/* free it */
		qglDeleteTextures (1, (uint32_t*)&image->texnum);
		memset (image, 0, sizeof(*image));
	}
}

void GL_ReloadPic (byte *pic, int bits, image_t *image)
{
	if (image->type == it_skin && bits == 8)
		R_FloodFillSkin(pic, image->width, image->height);

	{
		GL_Bind(image->texnum);
		if (bits == 8) {
			GL_Upload8 (pic, image->width, image->height, (image->type != it_pic) && (image->type != it_sky), image->type == it_sky );
		} else {
			GL_Upload32 ((uint32_t*)pic, image->width, image->height, (image->type != it_pic) && (image->type != it_sky) );	
		}
	}
}

void GL_ReloadWal (image_t *image)
{
	miptex_t	*mt;
	int		ofs;

	ri.FS_LoadFile (image->name, (void **)&mt);
	if (!mt)
		return;

	ofs = LittleLong (mt->offsets[0]);

	printf("Reuploading %s\n", image->name);
	GL_ReloadPic ((byte *)mt + ofs, 8, image);

	ri.FS_FreeFile ((void *)mt);
}

void GL_ReuploadImage(image_t *image)
{
	int		len = strlen(image->name);
	int		width, height;
	
	/*
	 * load the pic from disk
	 */
	byte *pic     = NULL;
	byte *palette = NULL;
	if (!strcmp((image->name)+len-4, ".tga"))
	{
		LoadTGA (image->name, &pic, &width, &height);
		if (!pic)
			return; /* ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name); */
		printf("Reuploading %s\n", image->name);
		GL_ReloadPic (pic, 32, image);
	}
	else if (!strcmp((image->name)+len-4, ".jpg"))
	{
		LoadJPG(image->name, &pic, &width, &height);
		if (!pic)
			return; /* ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name); */
		printf("Reuploading %s\n", image->name);
		GL_ReloadPic (pic, 32, image);
	}
	else if (!strcmp((image->name)+len-4, ".pcx") || !strcmp(image->name+len-4, ".wal"))
	{
		if (!strcmp((image->name)+len-4, ".pcx")) {
			LoadPCX (image->name, &pic, &palette, &width, &height);
			if (!pic)
				return; /* ri.Sys_Error (ERR_DROP, "GL_FindImage: can't load %s", name); */
			printf("Reuploading %s\n", image->name);
			GL_ReloadPic (pic, 8, image);
		} else if (!strcmp((image->name)+len-4, ".wal")) {
			GL_ReloadWal (image);
		}
	}
	else
		return; /*	ri.Sys_Error (ERR_DROP, "GL_FindImage: bad extension on: %s", name); */
	
	if (pic)
		free(pic);
	if (palette)
		free(palette);
}

extern byte	dottexture[8][8];

void restore_textures()
{
	int		i;
	image_t	*image;
	int		x,y;
	byte	data[8][8][4];
	
	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->texnum) {
			if ((!strcmp(image->name, "***r_notexture***"))||(!strcmp(image->name, "***particle***"))) continue;
			printf("Restoring texture %d (%s)\n", image->texnum, image->name);
			GL_ReuploadImage(image);
		}
	}

	/*
	 * particle texture
	 */
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]*255;
		}
	}
	GL_ReloadPic((byte*)data, 32, r_particletexture);
	
	/*
	 * also use this for bad textures, but without alpha
	 */
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = dottexture[x&3][y&3]*255;
			data[y][x][1] = 0; /* dottexture[x&3][y&3]*255; */
			data[y][x][2] = 0; /* dottexture[x&3][y&3]*255; */
			data[y][x][3] = 255;
		}
	}
	GL_ReloadPic((byte*)data, 32, r_notexture);
	
	GL_ReuploadImage(draw_chars);
}
