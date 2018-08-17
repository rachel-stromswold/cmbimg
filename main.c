#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "manipulation.h"
#include "image.h"
#include "util.h"
#define BUF_SIZE 20

#define MERGE_HORIZ 1
#define MERGE_VERT  2
#define MERGE_FORCE 4

#define MAX_PICS  4

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

void read_args(int argc, char** argv, struct flags* args) {
  if (argc < 3) {
    printf("error: proper usage is:\n	pngCombine fname1.png fname2.png\n");
    exit(0);
  }

  //set defaults
  args->merge_mode = MERGE_HORIZ | MERGE_VERT;
  args->columns = 2;
  args->num_pics = -1; 
  args->outputName = "cmbimg_out.png";
  args->pre_proc_args = create_list(BUF_SIZE);

  int n = argc - 1;
  int flagnum = 0;

  struct str_list* curr = args->pre_proc_args;
  for (int i = 1; i<n; i++) {
    flagnum = 0;
    if (argv[i][0] == '-') {
      //read special hyphenated option
      if (argv[i][1] == 'v') {
	flagnum += 1;
	args->merge_mode = (args->merge_mode | MERGE_VERT);
	args->columns = 1;
      } else if (argv[i][1] == 'h') {
	flagnum += 1;
	args->merge_mode = (args->merge_mode | MERGE_HORIZ);
      } else if (argv[i][1] == 'f') {
	flagnum += 1;
	args->merge_mode = (args->merge_mode | MERGE_FORCE);
      } else if (argv[i][1] == 's') {
	flagnum += 2;
	curr->n = i-1;
	if ( i >= argc-1) {
	  printf("Error: if you pass -s the following argument must be a width and height scale factor\n");
	  printf("i.e. : cmbimg -s w=1.0,h=2.0 foo.png bar.png out.png -c 2\n");
	  exit(1);
	}
	sprintf(curr->dat, "s%s", argv[i+1]);
	curr = append_empty(curr, BUF_SIZE);
      } else if (argv[i][1] == 'c') {
	flagnum += 2;
	if (i == argc-1) {
	  printf("ERROR: if you pass -c you must pass an integer after\n");
	  printf("i.e. : cmbimg foo.png bar.png out.png -c 2\n");
	  exit(1);
	}
	args->columns = strtol(argv[i+1], NULL, 10);
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
	args->num_pics = strtol(argv[i+1], NULL, 10);
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
	args->outputName = argv[i+1];
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
  if (args->num_pics < 0) {
    args->num_pics = n;
  } else if (args->num_pics < n) {
    printf("Too many pictures supplied, using the first %d.\n", args->num_pics);
    args->outputName = argv[n];
  }

  #ifdef DEBUG
  printf("found %d images\n", n);
  for (i = 0; i < argc; i++) {
    printf("arg %d: %s\n", i, argv[i]);
  }
  printf("cols: %d, n: %d, mode: %d\n", args->columns, args->num_pics, args->merge_mode);
  #endif 
}

void pre_process(struct image** inputs, struct str_list* pre_proc_args) {
  struct str_list* curr = pre_proc_args;
  char* tmp = NULL;
  int i = 0;
  while (curr != NULL) {
    i = curr->n;
    if (curr->dat[0] == 's') {
      double x = 1.0;
      double y = 1.0;
      tmp = parse_string_args(curr->dat, "x");
      if (tmp != NULL) {
	x = atof(tmp);
      } else {
	tmp = parse_string_args(curr->dat, "w");
	if (tmp != NULL) {
	  x = atof(tmp)/inputs[i]->w;
	} else {
	  printf("ERROR: '%s' does not seem to be a valid scaling argument string\n", curr->dat+sizeof(char*));
	}
      }
      tmp = parse_string_args(curr->dat, "y");
      if (tmp != NULL) {
	y = atof(tmp);
      } else {
      	tmp = parse_string_args(curr->dat, "h");
	if (tmp != NULL) {
	  x = atof(tmp)/inputs[i]->w;
	} else {
	  printf("ERROR: '%s' does not seem to be a valid scaling argument string\n", curr->dat+sizeof(char*));
	}
      }
      printf("now scaling image %d, by a factor of (%f,%f)\n", i, x, y);
      struct image* tmp_img = scale(inputs[i], x, y);
      delete_image(inputs[i]);
      inputs[i] = tmp_img;
    }
    curr = curr->next;
  }
}

int main(int argc, char** argv){
  struct flags args;
  read_args(argc, argv, &args);

  /*
   * =============================== LOAD AND PROCESS IMAGES ===============================
   */
  if (args.num_pics > MAX_PICS && !(args.merge_mode & MERGE_FORCE)) {
    printf("ERROR: exceeded max number of pictures.\n(You can supress this message by using the -f flag)\n");
    exit(1);
  }
  struct image **inputs = malloc(sizeof(struct image*)*args.num_pics);
  for (int i = 0; i < args.num_pics; i++) {
    inputs[i] = create_image();
    decode(argv[i+1], inputs[i]);
    printf("decoded file %s, w:%d, h:%d\n", argv[i+1], inputs[i]->w, inputs[i]->h);
  }
  pre_process(inputs, args.pre_proc_args);

  //create the output image
  struct image* out = merge(args.columns, args.num_pics, args.merge_mode, inputs, 0);
  
  encode(args.outputName, out->dat, out->w, out->h);

  for (int i = 0; i < args.num_pics; i++) {
    delete_image(inputs[i]);
  }
  //delete_image(out);

  return 0;
}
