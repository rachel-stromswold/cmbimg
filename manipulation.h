#ifndef MANIPULATION_H
#define MANIPULATION_H

#include <math.h>

#include "image.h"

//This is a helper function that scales down by a factor of sc_w and sc_h. If im is a 640x480 image and sc_w=sc_h=2.0 then the output will be a scaled down 320x240 image. If either sc_w<1.0 or sc_h<1.0 an error will be given.
struct image* scale_down(struct image* im, double sc_w, double sc_h) {
  int wn = round(im->w/sc_w);
  int hn = round(im->h/sc_h);

  int xn = floor(sc_w);
  int yn = floor(sc_h);
  struct image* tmp = create_empty_image(wn, hn, im->mode);

  struct color c;
  unsigned int r, g, b, a;
  for (int x = 0; x < wn; x++) {
    for (int y = 0; y < hn; y++) {
      r = 0;g = 0;b = 0;a = 0;
      for (int i = 0; i < xn; i++) {
	for (int j = 0; j < yn; j++) {
	  read_from_raw(im, x*xn+i, y*yn+j, &c);
	  r += (unsigned int)c.r;
	  g += (unsigned int)c.g;
	  b += (unsigned int)c.b;
	  a += (unsigned int)c.a;
	}
      }
      c.r = (unsigned char)(r/(xn*yn));
      c.g = (unsigned char)(g/(xn*yn));
      c.b = (unsigned char)(b/(xn*yn));
      c.a = (unsigned char)(a/(xn*yn));
      write_to_raw(tmp, x, y, c);
    }
  }
  return tmp;
}

struct image* scale_up(struct image* im, double sc_w, double sc_h) {
  int bw = floor(sc_w);
  int bh = floor(sc_h);
  struct image* tmp = create_empty_image(bw*im->w, bh*im->h, im->mode);

  struct color c;
  for (int x = 0; x < im->w; x++) {
    for (int y = 0; y < im->h; y++) {
      read_from_raw(im, x, y, &c);
      for (int i = 0; i < bw; i++) {
	for (int j = 0; j < bh; j++) {
	  write_to_raw(tmp, x*bw+i, y*bh+j, c);
	}
      }
    }
  }

  return tmp;
}

struct image* scale(struct image* im, double sc_w, double sc_h) {
  struct image* ret = NULL;
  if (sc_w >= 1.0 && sc_h >= 1.0) {
    ret = scale_up(im, sc_w, sc_h);
  } else if (sc_w <= 1.0 && sc_h <= 1.0) {
    if (sc_w == 0.0 || sc_h == 0.0) {
      printf("Error: cannot scale down to image of zero size\n");
      exit(1);
    }
    ret = scale_down(im, 1/sc_w, 1/sc_h);
  } else {
    printf("ERROR: can't simultaneously scale up along one axis and down along another\n");
    exit(1);
  }
  return ret;
}

struct image* crop(struct image* im, int x, int y, int w, int h) {
  if (x < 0 || y < 0 || w <0 || h <0) {
    printf("ERROR: invalid argument passed to crop, (x,y)=(%d,%d), (w,h)=(%d,%d)\n", x, y, w, h);
    exit(1);
  }
  if (x+w > im->w || y+h > im->h) {
    printf("WARNING: the selected rectangle goes out of image bounds\n");
  }
  struct image* tmp = create_empty_image(w, h, im->mode);

  struct color c;
  for (int xp = x; xp < im->w && xp < x+w; xp++) {
    for (int yp= y; yp < im->h && yp < y+h; yp++) {
      read_from_raw(im, xp, yp, &c);
      write_to_raw(tmp, xp-x, yp-y, c);
    }
  }

  return tmp;
}

#endif //MANIPULATION_H
