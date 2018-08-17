#include "image.h"

struct image* create_image() {
  struct image* ret = (struct image*)malloc(sizeof(struct image));
  ret->dat = NULL;
  ret->w = 0;ret->h = 0;
  return ret;
}

void reset_dat(struct image* im) {
  if (im->dat != NULL) {
    free(im->dat);
  }
  im->dat = malloc(4*im->w*im->h);
}

void delete_image(struct image* im) {
  if (im->dat != NULL) {
    free(im->dat);
  }
  free(im);
}

struct image* create_empty_image(unsigned int w, unsigned int h, unsigned char mode) {
  struct image* ret = (struct image*)malloc(sizeof(struct image));
  ret->w = w;
  ret->h = h;
  int numpix = w*h;
  if (mode & COL_ALPH) {
    numpix = numpix*4;
  } else {
    numpix = numpix*3;
  }
  ret->dat = malloc(sizeof(unsigned char)*numpix);
  return ret;
}

void decode(const char* filename, struct image* im)
{
  unsigned error;
  LodePNGState* st = malloc(sizeof(LodePNGState));

  lodepng_inspect(&(im->w), &(im->h), st, (const unsigned char*)filename, 1);
  reset_dat(im);

  error = lodepng_decode32_file(&(im->dat), &(im->w), &(im->h), filename);
  if(error) printf("error %u: %s\n", error, lodepng_error_text(error));
  im->mode = COL_RGB | COL_ALPH;

  free(st);
}

void encode(const char* filename, const unsigned char* image,
    unsigned int width, unsigned int height)
{
  /*Encode the image*/
  unsigned error = lodepng_encode32_file(filename, image, width, height);

  /*if there's an error, display it*/
  if(error) printf("error %u: %s\n", error, lodepng_error_text(error));
}

void read_from_raw(struct image* im,
    unsigned int x, unsigned int y, struct color* col) {
	col->r = im->dat[x*4 + y*(im->w)*4];
	col->g = im->dat[x*4+1 + y*(im->w)*4];
	col->b = im->dat[x*4+2 + y*(im->w)*4];
	col->a = im->dat[x*4+3 + y*(im->w)*4];
}

void write_to_raw(struct image* im,
    unsigned int x, unsigned int y, struct color col) {
	im->dat[x*4 + y*(im->w)*4] = col.r;
	im->dat[x*4+1 + y*(im->w)*4] = col.g;
	im->dat[x*4+2 + y*(im->w)*4] = col.b;
	im->dat[x*4+3 + y*(im->w)*4] = col.a;
}

void read_from_raw24(struct image* im,
    unsigned int x, unsigned int y, struct color* col) {
	col->r = im->dat[x*3 + y*(im->w)*3];
	col->g = im->dat[x*3+1 + y*(im->w)*3];
	col->b = im->dat[x*3+2 + y*(im->w)*3];

}

void write_to_raw24(struct image* im,
    unsigned int x, unsigned int y, struct color col) {
	im->dat[x*3 + y*(im->w)*3] = col.r;
	im->dat[x*3+1 + y*(im->w)*3] = col.g;
	im->dat[x*3+2 + y*(im->w)*3] = col.b;
}
