#ifndef R_IMAGE_COMMON_H
#define R_IMAGE_COMMON_H

#include "../client/ref.h"

void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);
void LoadTGA (char *name, byte **pic, int *width, int *height);
void LoadJPG (char *filename, byte **pic, int *width, int *height);

#endif
