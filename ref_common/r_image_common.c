#include <stdio.h>
#include <stdint.h>
#include "../jpeg-8c/jpeglib.h"

#include "r_image_common.h"

extern refimport_t ri;
char	*suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

/*
=================================================================

PCX LOADING

=================================================================
*/


/*
==============
LoadPCX
==============
*/
void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height)
{
	byte	*raw;
	pcx_t	*pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte	*out, *pix;

	*pic     = NULL;
	*palette = NULL;

	/*
	 * load the file
	 */
	len = ri.FS_LoadFile (filename, (void **)&raw);
	if (!raw)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Bad pcx file %s\n", filename);
		return;
	}

	/*
	 * parse the PCX file
	 */
	pcx = (pcx_t *)raw;

    pcx->xmin = LittleShort(pcx->xmin);
    pcx->ymin = LittleShort(pcx->ymin);
    pcx->xmax = LittleShort(pcx->xmax);
    pcx->ymax = LittleShort(pcx->ymax);
    pcx->hres = LittleShort(pcx->hres);
    pcx->vres = LittleShort(pcx->vres);
    pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
    pcx->palette_type = LittleShort(pcx->palette_type);

	raw = &pcx->data;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 640
		|| pcx->ymax >= 480)
	{
		ri.Con_Printf (PRINT_ALL, "Bad pcx file %s\n", filename);
		return;
	}

	out = malloc ( (pcx->ymax+1) * (pcx->xmax+1) );

	*pic = out;

	pix = out;

	if (palette)
	{
		*palette = malloc(768);
		memcpy (*palette, (byte *)pcx + len - 768, 768);
	}

	if (width)
		*width = pcx->xmax+1;
	if (height)
		*height = pcx->ymax+1;

	for (y=0 ; y<=pcx->ymax ; y++, pix += pcx->xmax+1)
	{
		for (x=0 ; x<=pcx->xmax ; )
		{
			dataByte = *raw++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while(runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if ( raw - (byte *)pcx > len)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "PCX file %s was malformed", filename);
		free (*pic);
		*pic = NULL;
	}

	ri.FS_FreeFile (pcx);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;

/*
=============
LoadTGA
=============
*/
void LoadTGA (char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	int		length;
	TargaHeader		targa_header;
	byte			*targa_rgba;
	byte tmp[2];

	*pic = NULL;

	/*
	 * load the file
	 */
	length = ri.FS_LoadFile (name, (void **)&buffer);
   (void)length;
	if (!buffer)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Bad tga file %s\n", name);
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;
	
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_index = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_length = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.y_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.width = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.height = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type!=2 
		&& targa_header.image_type!=10) 
		ri.Sys_Error (ERR_DROP, "LoadTGA: Only type 2 and 10 targa RGB images supported\n");

	if (targa_header.colormap_type !=0 
		|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24))
		ri.Sys_Error (ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = malloc (numPixels*4);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  /* skip TARGA image comment */
	
	if (targa_header.image_type==2)
   {
      /* Uncompressed, RGB images */
		for(row=rows-1; row>=0; row--)
      {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++)
         {
            unsigned char red       = 0;
            unsigned char green     = 0;
            unsigned char blue      = 0;
            unsigned char alphabyte = 0;
            switch (targa_header.pixel_size)
            {
               case 24:

                  blue = *buf_p++;
                  green = *buf_p++;
                  red = *buf_p++;
                  *pixbuf++ = red;
                  *pixbuf++ = green;
                  *pixbuf++ = blue;
                  *pixbuf++ = 255;
                  break;
               case 32:
                  blue = *buf_p++;
                  green = *buf_p++;
                  red = *buf_p++;
                  alphabyte = *buf_p++;
                  *pixbuf++ = red;
                  *pixbuf++ = green;
                  *pixbuf++ = blue;
                  *pixbuf++ = alphabyte;
                  break;
            }
         }
		}
	}
	else if (targa_header.image_type==10)
   {
      /* Runlength encoded RGB images */
      unsigned char red          = 0;
      unsigned char green        = 0;
      unsigned char blue         = 0;
      unsigned char alphabyte    = 0;
      unsigned char packetHeader = 0;
      unsigned char packetSize   = 0;
      unsigned char j;

      for(row=rows-1; row>=0; row--)
      {
         pixbuf = targa_rgba + row*columns*4;
         for(column=0; column<columns; )
         {
            packetHeader= *buf_p++;
            packetSize = 1 + (packetHeader & 0x7f);
            if (packetHeader & 0x80)
            {
               /* run-length packet */
               switch (targa_header.pixel_size)
               {
                  case 24:
                     blue = *buf_p++;
                     green = *buf_p++;
                     red = *buf_p++;
                     alphabyte = 255;
                     break;
                  case 32:
                     blue = *buf_p++;
                     green = *buf_p++;
                     red = *buf_p++;
                     alphabyte = *buf_p++;
                     break;
               }

               for(j=0;j<packetSize;j++)
               {
                  *pixbuf++=red;
                  *pixbuf++=green;
                  *pixbuf++=blue;
                  *pixbuf++=alphabyte;
                  column++;
                  if (column==columns)
                  {
                     /* run spans across rows */
                     column=0;
                     if (row>0)
                        row--;
                     else
                        goto breakOut;
                     pixbuf = targa_rgba + row*columns*4;
                  }
               }
            }
            else
            {
               /* non run-length packet */
               for(j=0;j<packetSize;j++)
               {
                  switch (targa_header.pixel_size)
                  {
                     case 24:
                        blue = *buf_p++;
                        green = *buf_p++;
                        red = *buf_p++;
                        *pixbuf++ = red;
                        *pixbuf++ = green;
                        *pixbuf++ = blue;
                        *pixbuf++ = 255;
                        break;
                     case 32:
                        blue = *buf_p++;
                        green = *buf_p++;
                        red = *buf_p++;
                        alphabyte = *buf_p++;
                        *pixbuf++ = red;
                        *pixbuf++ = green;
                        *pixbuf++ = blue;
                        *pixbuf++ = alphabyte;
                        break;
                  }
                  column++;
                  if (column==columns)
                  {
                     /* pixel packet run spans across rows */
                     column=0;
                     if (row>0)
                        row--;
                     else
                        goto breakOut;
                     pixbuf = targa_rgba + row*columns*4;
                  }						
               }
            }
         }
breakOut:;
      }
   }

	ri.FS_FreeFile (buffer);
}

/*
==============
LoadJPG
==============
*/
void LoadJPG (char *filename, byte **pic, int *width, int *height)
{
	struct jpeg_decompress_struct	cinfo;
	struct jpeg_error_mgr			jerr;
	byte							*rawdata, *rgbadata, *scanline, *p, *q;
	int							i;

	/* Load JPEG file into memory */
	int rawsize = ri.FS_LoadFile(filename, (void **)&rawdata);
	if (!rawdata)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Bad jpg file %s\n", filename);
		return;	
	}

	/* Knightmare- check for bad data */
	if (	rawdata[6] != 'J'
		||	rawdata[7] != 'F'
		||	rawdata[8] != 'I'
		||	rawdata[9] != 'F')
   {
      ri.Con_Printf(PRINT_ALL, "Bad jpg file %s\n", filename);
      ri.FS_FreeFile(rawdata);
      return;
   }

	/* Initialise libJpeg Object */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	/* Feed JPEG memory into the libJpeg Object */
	jpeg_mem_src(&cinfo, rawdata, rawsize);

	/* Process JPEG header */
	jpeg_read_header(&cinfo, true); /* bombs out here */

	/* Start Decompression */
	jpeg_start_decompress(&cinfo);

	/* Check Color Components */
	if(cinfo.output_components != 3)
	{
		ri.Con_Printf(PRINT_ALL, "Invalid JPEG color components\n");
		jpeg_destroy_decompress(&cinfo);
		ri.FS_FreeFile(rawdata);
		return;
	}

	/* Allocate Memory for decompressed image */
	rgbadata = malloc(cinfo.output_width * cinfo.output_height * 4);
	if(!rgbadata)
	{
		ri.Con_Printf(PRINT_ALL, "Insufficient RAM for JPEG buffer\n");
		jpeg_destroy_decompress(&cinfo);
		ri.FS_FreeFile(rawdata);
		return;
	}

	/* Pass sizes to output */
	*width = cinfo.output_width; *height = cinfo.output_height;

	/* Allocate Scanline buffer */
	scanline = malloc(cinfo.output_width * 3);
	if(!scanline)
	{
		ri.Con_Printf(PRINT_ALL, "Insufficient RAM for JPEG scanline buffer\n");
		free(rgbadata);
		jpeg_destroy_decompress(&cinfo);
		ri.FS_FreeFile(rawdata);
		return;
	}

	/* Read Scanlines, and expand from RGB to RGBA */
	q = rgbadata;
	while(cinfo.output_scanline < cinfo.output_height)
	{
		p = scanline;
		jpeg_read_scanlines(&cinfo, &scanline, 1);

		for(i=0; i<cinfo.output_width; i++)
		{
			q[0] = p[0];
			q[1] = p[1];
			q[2] = p[2];
			q[3] = 255;

			p+=3; q+=4;
		}
	}

	/* Free the scanline buffer */
	free(scanline);

	/* Finish Decompression */
	jpeg_finish_decompress(&cinfo);

	/* Destroy JPEG object */
	jpeg_destroy_decompress(&cinfo);

	/* Free raw data buffer */
	FS_FreeFile(rawdata);

	/* Return the 'rgbadata' */
	*pic = rgbadata;
}
