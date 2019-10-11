/* swimp.c */
#include "../ref_soft/r_local.h"
#define RGB8_to_565(r,g,b)  (((b)>>3)&0x1f)|((((g)>>2)&0x3f)<<5)|((((r)>>3)&0x1f)<<11)

uint16_t d_8to16table[256];
uint32_t start_palette[256];
uint16_t palette_tbl[256];

extern int scr_width;
extern int scr_height;
extern void *tex_buffer;

void SWimp_BeginFrame( float camera_separation )
{
}

void SWimp_EndFrame (void)
{
	uint16_t* rgb565_buffer = (uint16_t*)tex_buffer;
	int x,y;
	for(x=0; x<scr_width; x++){
		for(y=0; y<scr_height;y++){
			rgb565_buffer[x+y*scr_width] = palette_tbl[vid.buffer[y*scr_width + x]];
		}
	}
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

	uint8_t* pal = (uint8_t*)palette;
	unsigned char r, g, b;

	for(i = 0; i < 256; i++){
		r = pal[0];
		g = pal[1];
		b = pal[2];
		palette_tbl[i] = RGB8_to_565(r, g, b);
		pal += 4;
	}
}

void		SWimp_Shutdown( void )
{
	free(vid.buffer);
	free(tex_buffer);
}

rserr_t		SWimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	if (vid.buffer != NULL) SWimp_Shutdown();
	
	vid.height = scr_height;
	vid.width = scr_width;
	vid.rowbytes = scr_width;
	vid.buffer = malloc(scr_width*scr_height);
	
	tex_buffer = malloc(scr_width*scr_height*2);
	
	SWimp_SetPalette((const unsigned char*)start_palette);
	
	*pwidth = scr_width;
	*pheight = scr_height;
	VID_NewWindow(scr_width,scr_height);
	
	return rserr_ok;
}

void		SWimp_AppActivate( qboolean active )
{
}
