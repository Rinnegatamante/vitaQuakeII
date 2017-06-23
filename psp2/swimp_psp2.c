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

#include "../ref_soft/r_local.h"
#include <vitasdk.h>
#include <vita2d.h>

vita2d_texture* tex_buffer;
uint16_t d_8to16table[256];
uint32_t start_palette[256];
float scale_val;

void SWimp_BeginFrame( float camera_separation )
{
}

void SWimp_EndFrame (void)
{
	vita2d_start_drawing();
	vita2d_draw_texture_scale(tex_buffer, 0, 0, scale_val, scale_val);
	vita2d_end_drawing();
	//vita2d_wait_rendering_done();
	vita2d_swap_buffers(); 
}

int			SWimp_Init( void *hInstance, void *wndProc )
{
	return 0;
}

void		SWimp_SetPalette( const unsigned char *palette)
{
	
	if(palette==NULL)
		return;
	
	// SetPalette seems to be called before SetMode, so we save the palette on a temp location
	if (tex_buffer == NULL){
		memcpy(start_palette, palette, sizeof(uint32_t)*256);
		return;
	}
	
	int i;
	uint32_t* palette_tbl = vita2d_texture_get_palette(tex_buffer);

	uint8_t* pal = (uint8_t*)palette;
	unsigned char r, g, b;

	for(i = 0; i < 256; i++){
		r = pal[0];
		g = pal[1];
		b = pal[2];
		palette_tbl[i] = r | (g << 8) | (b << 16) | (0xFF << 24);
		pal += 4;
	}
}

void		SWimp_Shutdown( void )
{
	vita2d_free_texture(tex_buffer);
	tex_buffer = NULL;
}

extern int vidwidth;
extern int vidheight;

rserr_t		SWimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	switch (vidheight){
		case 272:
			scale_val = 2.0;
			break;
		case 362:
			scale_val = 1.5;
			break;
		case 408:
			scale_val = 1.3333;
			break;
		case 544:
			scale_val = 1.0;
			break;
		default:
			scale_val = 2.0;
			break;
	}
	vid.height = vidheight;
	vid.width = vidwidth;
	vid.rowbytes = vidwidth;
	vita2d_texture_set_alloc_memblock_type(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW);
	tex_buffer = vita2d_create_empty_texture_format_advanced(vidwidth, vidheight, SCE_GXM_TEXTURE_BASE_FORMAT_P8);
	vid.buffer = vita2d_texture_get_datap(tex_buffer);
	
	SWimp_SetPalette((const unsigned char*)start_palette);
	
	return rserr_ok;
}

void		SWimp_AppActivate( qboolean active )
{
}