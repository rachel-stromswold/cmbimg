#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "lodepng.h"
#define BUFF_SIZE 10

#define MERGE_HORIZ 1
#define MERGE_VERT  2
#define MERGE_FORCE 4

#define MAX_PICS  4

struct image {
  unsigned char* dat;
  unsigned int w;
  unsigned int h;
};

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

struct color {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};

void decode(const char* filename, struct image* im)
{
  unsigned error;
  LodePNGState* st = malloc(sizeof(LodePNGState));

  lodepng_inspect(&(im->w), &(im->h), st, (const unsigned char*)filename, 1);
  reset_dat(im);

  error = lodepng_decode32_file(&(im->dat), &(im->w), &(im->h), filename);
  if(error) printf("error %u: %s\n", error, lodepng_error_text(error));

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

struct image* merge_horiz(struct image* im1, struct image* im2) {
  struct image* ret = create_image();
  ret->w = im1->w + im2->w;
  if (im1->h >= im2->h) {
    ret->h = im1->h;
  } else {
    ret->h = im2->h;
  }
  reset_dat(ret);

  #ifdef DEBUG
  printf("creating output, w:%d, h:%d\n", ret->w, ret->h);
  #endif
  unsigned int x, y;
  struct color c;
  for (y = 0; y < ret->h; y++) {
    if (y < im1->h) {
      for (x = 0; x < im1->w; x++) {
	read_from_raw(im1, x, y, &c);
	write_to_raw(ret, x, y, c);
      }
    }
    if (y < im2->h) {
      for (x=0; x < im2->w; x++) {
	read_from_raw(im2, x, y, &c);
	write_to_raw(ret, x+im1->w, y, c);
      }
    }
  }

  return ret;
  delete_image(im1);
  delete_image(im2);
}

struct image* merge_vert(struct image* im1, struct image* im2) {
  struct image* ret = create_image();
  ret->h = im1->h + im2->h;
  if (im1->w >= im2->w) {
    ret->w = im1->w;
  } else {
    ret->w = im2->w;
  }

  reset_dat(ret);
  #ifdef DEBUG
  printf("creating output, w:%d, h:%d\n", ret->w, ret->h);
  #endif
  unsigned int x, y;
  struct color c;
  for (x = 0; x < ret->w; x++) {
    if (x < im1->w) {
      for (y = 0; y < im1->h; y++) {
	read_from_raw(im1, x, y, &c);
	write_to_raw(ret, x, y, c);
      }
    }
    if (x < im2->w) {
      for (y=0; y < im2->h; y++) {
	read_from_raw(im2, x, y, &c);
	write_to_raw(ret, x, y+im1->h, c);
      }
    }
  }
  delete_image(im1);
  delete_image(im2);
  return ret;
}

struct image* merge(int columns, int num_pics, char merge_mode, struct image** inputs, int off) {
  #ifdef DEBUG
  printf("cols: %d, n: %d, mode: %d, off: %d\n", columns, num_pics, merge_mode, off);
  #endif
  //only merge vertically if we are told to do so
  if (merge_mode == MERGE_VERT) {
    columns = 1;
  } else if (merge_mode == MERGE_HORIZ) {
    columns = num_pics;
  }
  //base case
  if (num_pics <= columns) {
    if (num_pics > 1) {
      struct image* working_old = merge_horiz(inputs[off+0], inputs[off+1]);
      struct image* working_new = NULL;
      for (int i = 2; i < num_pics; i++) {
        working_new = merge_horiz(working_old, inputs[off+i]);
	delete_image(working_old);
	working_old = working_new;
      }
      return working_old;
    } else if (num_pics == 1) {
      //if we only have one image in this row then we are done
      return inputs[off];
    } else {
      return NULL;
    }
  }
  //recursion
  struct image* im1 = merge(columns, columns, merge_mode, inputs, 0);
  struct image* im2 = merge(columns, num_pics - columns, merge_mode, inputs, columns);
  return merge_vert(im1, im2);
}

//move all elements in argv after i to the previous element and put the current at the end
void move_args(int i, int ntimes, int argc, char** argv) {
  for(int k = 0; k<ntimes; k++) {
    if (i < argc-1) {
      //move all arguments
      int j = i;
      char* tmp = argv[i];
      for (; j < argc-1; j++) {
	argv[j] = argv[j+1];
      }
      argv[argc-1] = tmp;
    }
  }
}

int main(int argc, char** argv){
  if (argc < 3) {
    printf("error: proper usage is:\n	pngCombine fname1.png fname2.png\n");
    exit(0);
  }
  //1 = merge horizontally, 2 = merge vertically
  char merge_mode = MERGE_HORIZ | MERGE_VERT;
  int columns = 2;
  int num_pics = -1;
  int n = argc - 1;
  int flagnum = 0;
  int i = 1;
  char* outputName = "cmbimg_out.png";
  for (; i<n; i++) {
    flagnum = 0;
    if (argv[i][0] == '-') {
      //read special hyphenated option
      if (argv[i][1] == 'v') {
	flagnum += 1;
	merge_mode = (merge_mode | MERGE_VERT);
	columns = 1;
      } else if (argv[i][1] == 'h') {
	flagnum += 1;
	merge_mode = (merge_mode | MERGE_HORIZ);
      } else if (argv[i][1] == 'f') {
	flagnum += 1;
	merge_mode = (merge_mode | MERGE_FORCE);
      } else if (argv[i][1] == 'c') {
	flagnum += 2;
	if (i == argc-1) {
	  printf("ERROR: if you pass -c you must pass an integer after\n");
	  printf("i.e. : comb_png foo.png bar.png out.png -c 2");
	  exit(1);
	}
	columns = strtol(argv[i+1], NULL, 10);
	int errsav = errno;
	if (errsav == EINVAL) {
	  printf("invalid column number supplied '%s'\n", argv[i+1]);
	  exit(1);
	} else if (errsav == ERANGE) {
	  printf("column number out of range '%s'\n", argv[i+1]);
	  exit(1);
	}
      } else if (argv[i][1] == 'n') {
	flagnum += 2;
	if (i == argc-1) {
	  printf("ERROR: if you pass -n you must pass an integer after\n");
	  printf("i.e. : cmbimg -n 2 foo.png bar.png -o out.png");
	  exit(1);
	}
	num_pics = strtol(argv[i+1], NULL, 10);
	int errsav = errno;
	if (errsav == EINVAL) {
	  printf("invalid column number supplied '%s'\n", argv[i+1]);
	  exit(1);
	} else if (errsav == ERANGE) {
	  printf("column number out of range '%s'\n", argv[i+1]);
	  exit(1);
	}
      } else if (argv[i][1] == 'o') {
	flagnum += 2;
	if (i == argc-1) {
	  printf("ERROR: if you pass -o you must pass a file name after\n");
	  printf("i.e. : cmbimg foo.png bar.png out.png -c 2");
	  exit(1);
	}
	outputName = argv[i+1];
      } else {
	printf("unrecognized option '%s'", argv[i]+1);
	exit(0);
      }
    }

    move_args(i, flagnum, argc, argv);
    //decrease the number of arguments that are files
    n -= flagnum;
    if (flagnum > 0) {
      i -= 1;
    }
  }
  printf("found %d images\n", n);
  if (num_pics < 0) {
    num_pics = n;
  } else if (num_pics < n) {
    printf("Too many pictures supplied, using the first %d.\n", num_pics);
    outputName = argv[n];
  }

  #ifdef DEBUG
  for (i = 0; i < argc; i++) {
    printf("arg %d: %s\n", i, argv[i]);
  }
  printf("cols: %d, n: %d, mode: %d\n", columns, num_pics, merge_mode);
  #endif
  /*const char* file1 = argv[1];
  const char* file2 = argv[2];*/
  
  /*if (argc >= 4) {
    outputName = malloc(strlen(argv[3]));
    strcpy(outputName, argv[3]);
  } else {
    outputName = "comb_png_output.png"
    outputName = malloc(strlen(file1)+strlen(file2)+6);
    strncpy(outputName, file1, strlen(file1)-4);
    outputName[strlen(file1)-3]=0;
    strcat(outputName, "_and_");
    strcat(outputName, file2);
  }*/

  if (num_pics > MAX_PICS && !(merge_mode & MERGE_FORCE)) {
    printf("ERROR: exceeded max number of pictures.\n(You can supress this message by using the -f flag)\n");
    exit(1);
  }
  struct image **inputs = malloc(sizeof(struct image*)*num_pics);
  for (i = 0; i < num_pics; i++) {
    inputs[i] = create_image();
    decode(argv[i+1], inputs[i]);
    printf("decoded file %s, w:%d, h:%d\n", argv[i+1], inputs[i]->w, inputs[i]->h);
  }

  //create the output image
  struct image* out = merge(columns, num_pics, merge_mode, inputs, 0);
  
  //printf("finished creating output, w:%d, h%d\n", out->w, out->h);
  encode(outputName, out->dat, out->w, out->h);

//  free(outputName);
  for (i = 0; i < num_pics; i++) {
    delete_image(inputs[i]);
  }
  delete_image(out);

  return 0;
}
