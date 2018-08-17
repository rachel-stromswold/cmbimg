#ifndef IMAGE_H
#define IMAGE_H

#include <stdlib.h>
#include <stdio.h>

#include "lodepng.h"
#define COL_RGB		1
#define COL_ALPH	2
#define COL_YCbCr	4

struct color {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};

struct image {
  unsigned char mode;
  unsigned char* dat;
  unsigned int w;
  unsigned int h;
};

struct image* create_image();
void reset_dat(struct image* im);
void delete_image(struct image* im);
//produces an empty image of size w h. WARNING: does not free any previously created images
struct image* create_empty_image(unsigned int w, unsigned int h, unsigned char mode);
void decode(const char* filename, struct image* im);
void encode(const char* filename, const unsigned char* image, unsigned int width, unsigned int height);
void read_from_raw(struct image* im, unsigned int x, unsigned int y, struct color* col);
void write_to_raw(struct image* im, unsigned int x, unsigned int y, struct color col);
void read_from_raw24(struct image* im, unsigned int x, unsigned int y, struct color* col);
void write_to_raw24(struct image* im, unsigned int x, unsigned int y, struct color col);

#endif //IMAGE_H
