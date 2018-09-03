#ifndef IMGLOAD_H
#define IMGLOAD_H

#include <string.h> /*for size_t*/
#define WRITE_BUFF_SIZE	20
#define EPS_RANGE	30//the minimum distance between the nearest pixel in order to save (colors treated as 3d vectors)
#define BREAK_EPS	30//the distance at which we don't look for any better candidates (optimization)
#define CHANGE_EPS	0//the minimum epsilon difference between adjacent colors before we change
#define BYTES_PER_COL	3
#define TAB_SIZE	256
#define DICT_SIZE	512

#define DICT_MSG_RES	-1
#define DICT_MSG_GREW	-2
#define DICT_MSG_NULL	-3

#define GIF_MAX_CODE_LEN	12

typedef struct LodeIMGState {
  size_t w;
  size_t h;
} LodeIMGState;

struct palatte {
  unsigned char n;//number of bits used to store palatte (maximum of 256)
  unsigned char* dat;
};

//set num_cols to a negative value to use the default
struct palatte* create_palatte(int num_cols) {
  if (num_cols < 0) {
    num_cols = TAB_SIZE;
  }
  struct palatte* ret = (struct palatte*)malloc(sizeof(struct palatte));
  //we must have a power of 2 number of colors
  if (num_cols <= 4) {
    ret->n = 2;
  } else if (num_cols <= 8) {
    ret->n = 3;
  } else if (num_cols <= 16) {
    ret->n = 4;
  } else if (num_cols <= 32) {
    ret->n = 5;
  } else if (num_cols <= 64) {
    ret->n = 6;
  } else  if (num_cols <= 128) {
    ret->n = 7;
  } else {
    ret->n = 8;
  }
  ret->dat = (unsigned char*)calloc(num_cols, BYTES_PER_COL);
  return ret;
}

//creates a new palate by reading 3 bytes at a time (r, g, b) from the file file
struct palatte* read_palatte(FILE* re, int N) {
  struct palatte* ret = (struct palatte*)malloc(sizeof(struct palatte));
  ret->n = N + 1;
  int n_cols = (0x01 << (N+1));
  ret->dat = (unsigned char*)malloc(n_cols*BYTES_PER_COL);
  unsigned char buf[3];

  for (int i = 0; i < n_cols; i++) {
    fread(buf, 1, 3, re);
    ret->dat[BYTES_PER_COL*i] = buf[0];
    ret->dat[BYTES_PER_COL*i + 1] = buf[1];
    ret->dat[BYTES_PER_COL*i + 2] = buf[2];
  }
  return ret;
}

void print_palatte(struct palatte* pal) {
  for (int i = 0; i < 0x01 << pal->n; i++) {
    printf("color %d: (%d, %d, %d)\n", i, pal->dat[BYTES_PER_COL*i], pal->dat[BYTES_PER_COL*i+1], pal->dat[BYTES_PER_COL*i+2]);
  }
}

void delete_palatte(struct palatte* pal) {
  if (pal != NULL) {
    free(pal->dat);
    free(pal);
  }
}

struct dictionary {
  size_t entries;
  size_t next_write;
  //the first entries are implicit
  size_t imp_ents;
  size_t size;
  unsigned char** dat;//an int is at least 16>12 bits so we may safely store any code
};

//specify the palatte which will contain initial entries
struct dictionary* create_dictionary(struct palatte* pal) {
  struct dictionary* ret = (struct dictionary*)malloc(sizeof(struct dictionary));
  ret->size = 0x01 << (pal->n + 1);
  ret->dat = (unsigned char**)calloc(ret->size, sizeof(char*));
  ret->imp_ents = (0x01 << pal->n) + 2;//plus 2 for the clear and end codes
/*  for (int i = 0; i < (0x01 << pal->n); i++) {
    ret->dat[i] = i;//1 byte for length and 1 for data
  }*/
  ret->entries = ret->imp_ents;
  ret->next_write = 0;
  return ret;
}

void delete_dictionary(struct dictionary* dict) {
  if (dict != NULL) {
    if (dict->dat != NULL) {
      size_t n = dict->entries - dict->imp_ents;
      for (int i = 0; i < n; i++) {
	free(dict->dat[i]);
      }
      free(dict->dat);
    }
    free(dict);
  }
}

void dict_grow(struct dictionary* dict) {
  unsigned char** old_dat = dict->dat;
  dict->dat = (unsigned char**)calloc((dict->size*2), sizeof(char*));
  for (int i = 0; i < dict->size; i++) {
    dict->dat[i] = old_dat[i];
  }
  dict->size *= 2;
  free(old_dat);
}

//n = number of bytes to copy, be sure this is right or you will get a segfault
//index = the index to write the data to or 0 to write to the next spot
//returns index on success, -2 if the dictionary grew and -1 if the dictionary was reset (a clear code must be added to the stream, a code of -4 means the end of the file was reached
int add_dict_code(struct dictionary* dict, size_t index, unsigned char* code, size_t n) {
  int ret = DICT_MSG_NULL;
  if (dict != NULL) {
    size_t l0 = dict->entries - dict->imp_ents;
    if (l0 >= dict->size) {
      if (dict->entries > (0x01 << GIF_MAX_CODE_LEN)) {
	return DICT_MSG_RES;
      }
      printf("Dictionary outgrew its size of %lu entries.\n", dict->size);
      dict_grow(dict);
      ret = DICT_MSG_GREW;
    }

    //if we specify an index add the next entry there, otherwise add it at the next slot
    if (index < dict->imp_ents) {
      l0 = dict->next_write;
      dict->dat[dict->next_write] = (unsigned char*)malloc(n+1);
      dict->dat[dict->next_write][0] = n;
    } else {
      l0 = index - dict->imp_ents;
      if (index > dict->size) {
	printf("Error: tried to add a dictionary entry at an invalid index %lu, entries: %lu.\n", index, dict->size);
	dict_grow(dict);
	ret = DICT_MSG_GREW;
	//exit(0);
      }
      dict->dat[index - dict->imp_ents] = (unsigned char*)malloc(n+1);
      dict->dat[index - dict->imp_ents][0] = n; 
    }
    //move the next write slot to the next available spot
    while (dict->dat[dict->next_write] != NULL) {
      dict->next_write += 1;
    }

#ifdef DEBUG_WRITE
    printf("Next write: %lu\nAdded dictionary code %lu: ", dict->next_write, dict->entries);
#endif
    for (int i = 0; i < n; i++) {
      dict->dat[l0][i+1] = code[i];
#ifdef DEBUG_WRITE
      printf("%d ", code[i]);
#endif
    }
#ifdef DEBUG_WRITE
    printf("\n");
#endif
    dict->entries++;
  }
  return ret;
}

//returns the index of the code that matches the one specified or -1 if no code is found
//returns -1 if not found or -2 if there is an error
long int get_dict_code(struct dictionary* dict, unsigned char* pat, size_t n) {
  if (dict != NULL) {
    if (n == 1 && pat[0] < dict->imp_ents) {
      return (long int)(pat[0]);
    }
    for (int i = 0; i < (dict->entries - dict->imp_ents); i++) {
      //if the codes are of a different length then obviously they won't match
      if (n == dict->dat[i][0]) {
	char found = 1;
	for (int j = 0; j < n; j++) {
	  //if there is one bit different, then we have not found it
	  if (pat[j] != dict->dat[i][j+1]) {
	    found = 0;
	    break;
	  }
        }
	if (found) {
	  return i + dict->imp_ents;
	}
      }
    }
    return -1;
  }
  return -2;
}

//returns the length of the given code in bytes or -1 if the code isn't in the dictionary
int get_code_len(struct dictionary* dict, long int code) {
  if (code > dict->entries) {
    return -1;
  }
  if (code < dict->imp_ents) {
    return 1;
  }
  if (dict->dat[code - dict->imp_ents] == NULL) {
    return -1;
  }
  return (int)(dict->dat[code - dict->imp_ents][0]);
}

//returns the jth element in the pattern for the corresponding code
size_t get_pattern(struct dictionary* dict, size_t code, int j) {
  if (code < dict->imp_ents) {
    return code;
  }
  if (dict->dat[code - dict->imp_ents] == NULL) {
    printf("Warning, tried to access null table entry\n");
    return 0;
  }
  return dict->dat[code - dict->imp_ents][j + 1];
}

void print_dict_debug(struct dictionary* dict, size_t code) {
#ifdef DEBUG_READ
  printf("code = %lu: ", code);
  int n = get_code_len(dict, code);
  for (int i = 0; i < n; i++) {
    printf("%lu ", get_pattern(dict, code, i));
  }
  printf("\n");
#endif
}

int get_dist(unsigned char r, unsigned char g, unsigned char b, unsigned char ro, unsigned char go, unsigned char bo) {
  return (int)(r-ro)*(int)(r-ro)+(int)(g-go)*(int)(g-go)+(int)(b-bo)*(int)(b-bo);
}

//writes code to the bit stream pointed to by buf
//this uses the really weird "little endian" schema employed by gif
void write_stream(unsigned char* buf, size_t offset, size_t code) {
  size_t n_bytes = offset / 8;
  size_t n_bits = offset % 8;
  
  //hooray!
  unsigned char mask = 0xff & ~((0x01 << n_bits) - 1);
  buf[n_bytes] = (buf[n_bytes] & ~mask) | (mask & (code << n_bits));
  buf[n_bytes+1] = (code >> (8 - n_bits)) & 0xff;
  buf[n_bytes+2] = (code >> (16 - n_bits)) & 0xff;
  buf[n_bytes+3] = (code >> (24 - n_bits)) & 0xff;
}

//writes code to the bit stream pointed to by buf
//this uses the really weird "little endian" schema employed by gif
size_t read_stream(unsigned char* buf, size_t offset, unsigned char code_size) {
  size_t n_bytes = offset / 8;
  size_t n_bits = offset % 8;

  size_t ret = (size_t)buf[n_bytes] >> n_bits;
  ret += (size_t)buf[n_bytes+1] << (8-n_bits);
  ret += (size_t)buf[n_bytes+2] << (16-n_bits);
  ret += (size_t)buf[n_bytes+3] << (24-n_bits);
  ret = ret & ((0x01 << code_size) - 1);
  return ret;
}

//takes the image data pointed to by dat and recolorizes it to use a 256 color palatte
//returns the color palatte used
struct palatte* tabulate_img(unsigned char* dat, size_t bpc, size_t w, size_t h) {
//  unsigned char *col_tab = (unsigned char*)malloc(TAB_SIZE*BYTES_PER_COL);
  struct palatte* pal = create_palatte(-1);
  unsigned char r,g,b,ro,go,bo;
  char save = 1;
  //for(int i = 0;i<COL_TAB_SIZE;i++) {
  int i = 0;
  for(int j = 0; j < w*h && i < TAB_SIZE; j++) {
    save = 1;
    r = dat[bpc*j];
    g = dat[bpc*j+1];
    b = dat[bpc*j+2];
    for (int k = 0; k < i; k++) {
      ro = pal->dat[BYTES_PER_COL*k];//we use 3 since gif doesn't support alpha chanel
      go = pal->dat[BYTES_PER_COL*k+1];
      bo = pal->dat[BYTES_PER_COL*k+2];
      if (get_dist(r,g,b,ro,go,bo) < EPS_RANGE) {
	save = 0;
	break;
      }
    }
    if (save) {
#ifdef DEBUG_WRITE
      printf("New color to be added: %d, %d, %d\n", r, g, b);
#endif
      pal->dat[BYTES_PER_COL*i] = r;
      pal->dat[BYTES_PER_COL*i+1] = g;
      pal->dat[BYTES_PER_COL*i+2] = b;
      i++;
    }
  }
  if (i < TAB_SIZE-1) {
    /*int rat = 255/(TAB_SIZE-i);
    for (;i<TAB_SIZE;i++) {
      pal->dat[BYTES_PER_COL*i] = i*rat;
      pal->dat[BYTES_PER_COL*i+1] = pal->dat[BYTES_PER_COL*i];
      pal->dat[BYTES_PER_COL*i+2] = pal->dat[BYTES_PER_COL*i];
    }*/
    struct palatte* newpal = create_palatte(i);
    for (int j = 0; j < i; j++) {
      newpal->dat[BYTES_PER_COL*j] = pal->dat[BYTES_PER_COL*j];
      newpal->dat[BYTES_PER_COL*j+1] = pal->dat[BYTES_PER_COL*j+1];
      newpal->dat[BYTES_PER_COL*j+2] = pal->dat[BYTES_PER_COL*j+2];
    }
    delete_palatte(pal);
    pal = newpal;
  }
  printf("Finished creating palatte with %d colors.\n", 0x01 << pal->n);

  //modify the image to use the palatte we just created
  int best_k = 0;
  for (i = 0; i < w*h; i++) {
    int cur_eps = get_dist(r, g, b,
			   dat[bpc*i],
			   dat[bpc*i+1],
			   dat[bpc*i+2]);

    if (i == 0 || cur_eps > CHANGE_EPS) {
      r = dat[bpc*i] + ro;//add the previous offset for dithering
      g = dat[bpc*i+1] + go;
      b = dat[bpc*i+2] + bo;
      int eps = 0xffff;

      for (int k = 0; k < TAB_SIZE; k++) {
	cur_eps = get_dist(r, g, b,
			       pal->dat[BYTES_PER_COL*k],
			       pal->dat[BYTES_PER_COL*k+1],
			       pal->dat[BYTES_PER_COL*k+2]);
	if (cur_eps < eps) {
	  eps = cur_eps;
	  best_k = k;
	  if (cur_eps < BREAK_EPS) {
	    break;
	  }
	}
      }
    }
    ro = pal->dat[BYTES_PER_COL*best_k] - dat[bpc*i];
    go = pal->dat[BYTES_PER_COL*best_k+1] - dat[bpc*i+1];
    bo = pal->dat[BYTES_PER_COL*best_k+2] - dat[bpc*i+2];
    dat[i] = best_k;//kludge over existing data
//    dat[bpc*i+1] = pal->dat[bpc*k_best+1];
//    dat[bpc*i+2] = pal->dat[bpc*k_best+2];
  }
  return pal;
}

//bpc = bytes per color
void imgload_encode_gif(const char* fname, size_t w, size_t h, unsigned char* dat, size_t bpc) {
  FILE *wr = fopen(fname, "wb");
  unsigned char *buf = (unsigned char*)malloc(WRITE_BUFF_SIZE);
  unsigned char min_code_size = 0;
  buf[0] = 0x47;buf[1] = 0x49;buf[2] = 0x46;//GIF
  buf[3] = 0x38;buf[4] = 0x39;buf[5] = 0x61;//89a
  buf[6] = (unsigned char)(0x00ff & w);
  buf[7] = (unsigned char)((0xff00 & w) >> 8);//encode width
  buf[8] = (unsigned char)(0x00ff & h);
  buf[9] = (unsigned char)((0xff00 & h) >> 8);//encode height
  buf[10] = 0xf0;//packed byte specifies color table present, with 8 bits per color and an unsorted color table of size 2^8=256
  buf[11] = 0;//set the background to be 0
  buf[12] = 0;//set the pixel aspect ratio to the default

  struct palatte* pal = tabulate_img(dat, bpc, w, h);
  min_code_size = pal->n;
  //make sure that the size of the color table is correct (minus 1 because gif is weird)
  buf[10] += min_code_size - 1;
  fwrite(buf, 1, 13, wr);fseek(wr, 0, SEEK_END); 
  fwrite(pal->dat, BYTES_PER_COL, ((0x01 << pal->n)), wr);fseek(wr, 0, SEEK_END);

  //We now use lzw compression
  unsigned char* out_tab = &(dat[w*h]);
  struct dictionary* dict = create_dictionary(pal);
  unsigned char cur_code_size = pal->n + 1;
  unsigned int def_clear, def_end;//the value of these two codes depends on the palatte size
  //set clear and end to be 1 and 2 codes after the last color respectively
  def_clear = (0x01 << pal->n);def_end = (0x01 << pal->n)+1;
  long int cur_code, prev_code;
  int res = -1;
  size_t offset = 0;
  size_t i = 0, j = 0;
  //always start by writing a clear code to the stream
  write_stream(out_tab, offset, def_clear);
  offset += cur_code_size;
  while (i < w*h) {
    cur_code = get_dict_code(dict, &(dat[i]), 1);
    if (i == w*h - 1) {
      printf("Reached end of file\n");
      i += 1;
      prev_code = cur_code;
      res = -4;
    }
#ifdef DEBUG_WRITE
    printf("off = %lu, i = %lu: K = %d", offset, i, dat[i]);
#endif
    for (j = 1; i+j < w*h; j++) {
#ifdef DEBUG_WRITE
      printf(" K = %d", dat[i+j]);
#endif
      prev_code = cur_code;
      cur_code = get_dict_code(dict, &(dat[i]), j+1);
      if (cur_code == -1) {
	res = add_dict_code(dict, 0, &(dat[i]), j+1);
	i += get_code_len(dict, prev_code);
	break;
      }
      if (i+j == w*h - 1) {
	printf("Reached end of file\n");
	i += get_code_len(dict, cur_code);
	prev_code = cur_code;//we should write whatever code we need here
	res = -4;
	break;
      }
    }
#ifdef DEBUG_WRITE
    printf("\n");
#endif

    if (res == DICT_MSG_RES) {
      cur_code_size = pal->n + 1;
      write_stream(out_tab, offset, def_clear);
      offset += cur_code_size;
      write_stream( out_tab, offset, (size_t)(&(dat[i])) );
      offset += cur_code_size;
    }
    if (res > (0x01 << cur_code_size) || res == DICT_MSG_GREW){
      cur_code_size ++;
    }
    if (offset / 16 > w*h) {
      printf("Error: the output has grown too large. off = %lu, i = %lu\n", offset, i);
      exit(0);
    }
    write_stream(out_tab, offset, prev_code);
    offset += cur_code_size;
  }
  write_stream(out_tab, offset, def_end);
  offset += cur_code_size;
  delete_dictionary(dict);

  delete_palatte(pal);

  //write GCE
  buf[0] = 0x21;
  buf[1] = 0xf9; //preceding indicator for GCE block
  buf[2] = 0x04; //4 bytes follow
  buf[3] = 0x00; //disable transparency
  buf[4] = 0x00;buf[5] = 0x00; //we don't use animation so the delay time is irrelevant
  buf[6] = (0x01 << buf[10]) - 1; //set the last color to be transparent
  buf[7] = 0x00; //block terminator
//  buf[8] = 0x02; //set the minimum code size to 2
  buf[8] = 0x2c; //image descriptor
  buf[9] = 0x00;buf[10] = 0x00;buf[11] = 0x00;buf[12] = 0x00; //this image has an origin at (0,0)
  buf[13] = (unsigned char)(0x00ff & w);
  buf[14] = (unsigned char)((0xff00 & w) >> 8);//encode width
  buf[15] = (unsigned char)(0x00ff & h);
  buf[16] = (unsigned char)((0xff00 & h) >> 8);//encode height
  buf[17] = 0x00;//no local color table
  fwrite(buf, 1, 18, wr);//write this to the file
  fseek(wr, 0, SEEK_END);

  //write the compressed data to the file
  int n = offset / (8 * 256);//8 bits/byte times max number of bytes/block
  for (int i = 0; i < n; i++) {
    buf[0] = min_code_size;
    buf[1] = 0xff;//we have a full block of data coming
    fwrite(buf, 1, 2, wr);
    fseek(wr, 0, SEEK_END);
    fwrite(&(out_tab[i*255]), 1, 255, wr);
    fseek(wr, 0, SEEK_END);
  }
  buf[0] = min_code_size;
  buf[1] = ((offset / 8 + 1) % 256);//we have a full block of data coming
  fwrite(buf, 1, 2, wr);
  fseek(wr, 0, SEEK_END);
  fwrite(out_tab, 1, buf[1], wr);
  fseek(wr, 0, SEEK_END);
  buf[0] = 0x00;//no more image data follows
  buf[1] = 0x3b;
  fwrite(buf, 1, 2, wr);
  //}
}

//this is a helper function that writes {CODE} to the image and appends the palatte color K if ap is not equal to 0
//RETURNS: the new value of the index offset
int out_to_img(unsigned char* img, int bpc, int i, int n,
	       struct palatte* pal, struct dictionary* dict, size_t code, unsigned char K/*, char ap*/) {
  int j = 0;
  for (; j < n; j++) {
    unsigned char seq = get_pattern(dict, code, j);
    img[bpc*(i+j)] = pal->dat[BYTES_PER_COL*seq];
    img[bpc*(i+j)+1] = pal->dat[BYTES_PER_COL*seq+1];
    img[bpc*(i+j)+2] = pal->dat[BYTES_PER_COL*seq+2];
  }
/*  if (ap) {
    img[bpc*(i+j)] = pal->dat[BYTES_PER_COL*K];
    img[bpc*(i+j)+1] = pal->dat[BYTES_PER_COL*K+1];
    img[bpc*(i+j)+2] = pal->dat[BYTES_PER_COL*K+2];
  }*/
  return i+j;
}

//this is a helper function that adds a new dictionary entry {CODE}+K to the table
int save_to_dict(struct dictionary* dict, size_t index, size_t code, unsigned char K, int n) {
  n = get_code_len(dict, code);
  unsigned char* seq = malloc(n+1);
  int j = 0;
  for (; j < n; j++) {
    seq[j] = get_pattern(dict, code, j);
  }
  seq[j] = K;
  int ret = add_dict_code(dict, index, seq, n+1);
  free(seq);
  return ret;
}

unsigned char* imgload_decode_gif(const char* fname, unsigned int* w, unsigned int* h, size_t bpc) {
  FILE *re = fopen(fname, "rb"); 
  unsigned char buf[WRITE_BUFF_SIZE];
  fread(buf, 1, 13, re);
  //GIF89a
  if (buf[0] != 0x47 || buf[1] != 0x49 || buf[2] != 0x46 ||
      buf[3] != 0x38 || buf[4] != 0x39 || buf[5] != 0x61) {
    printf("Error: '%s' is not a gif 89 file.\n", fname);
    exit(1);
  }
  //read the width and height and create the return image
  *w = buf[6] + ((size_t)(buf[7]) << 8);
  *h = buf[8] + ((size_t)(buf[9]) << 8);
  unsigned char* ret = malloc(bpc * (*w)*(*h));

  struct palatte* glob_pal = NULL;
  unsigned char back_col = buf[11];

  //if the file specifies a global color table is present
  if (buf[10] & 0x80) {
    glob_pal = read_palatte(re, buf[10] & 0x07);
    print_palatte(glob_pal);
  }
  fread(buf, 1, 1, re);
  for (int i = 0; i < (*w)*(*h); i++) {
    ret[i*bpc] = glob_pal->dat[BYTES_PER_COL*back_col];
    ret[i*bpc + 1] = glob_pal->dat[BYTES_PER_COL*back_col + 1];
    ret[i*bpc + 2] = glob_pal->dat[BYTES_PER_COL*back_col + 2];
    //if there is an alpha chanel
    if (bpc > 3)
      ret[i*bpc + 3] = 255;
  }

  //if the next block specifies a graphics control extension
  if (buf[0] == 0x21) {
    //we don't actually support any special options here, so skip past it
    fread(buf, 1, 2, re);
    fread(buf, 1, buf[1] + 1, re);
    //we need to read the next byte to properly detect an image block
    fread(buf, 1, 1, re);
  }

  //if we are at the start of an image block
  while (buf[0] == 0x2c) {
    fread(buf, 1, 9, re);
    int lx = buf[0] + ((size_t)(buf[1]) << 8);
    int ly = buf[2] + ((size_t)(buf[3]) << 8);
    int lw = buf[4] + ((size_t)(buf[5]) << 8);
    int lh = buf[6] + ((size_t)(buf[7]) << 8);
    struct palatte* pal = glob_pal;
    //if the local color table flag is set
    if (buf[8] & 0x80) {
      pal = read_palatte(re, buf[8] & 0x07);
    }

    struct dictionary* dict = create_dictionary(pal);

    fread(buf, 1, 2, re);
    //actually begin reading the image
    unsigned char min_code_size = buf[0] + 1;
    //set clear and end to be 1 and 2 codes after the last color respectively
    unsigned int def_clear, def_end;
    def_clear = (0x01 << pal->n);def_end = (0x01 << pal->n)+1;

    unsigned char cur_code_size = min_code_size;
    int ents = dict->imp_ents - 2;//we ignore the first 2 codes
    //read the number of data bytes following
    int n_bytes = buf[1];
    size_t offset = 0;
    int i = lw*ly+lx;int j = 0;

    size_t code, prev_code;
    code = 0;

    //parse data from the image block into an array of data codes
    size_t f_stream_size = n_bytes;
    unsigned char* f_stream = malloc(sizeof(unsigned char)*n_bytes);
    //information for the data stream

    while (n_bytes > 0) {
      printf("Now reading %d bytes\n", n_bytes);
      if (j + n_bytes > f_stream_size) {
	printf("Resizing data stream.\n");
	unsigned char* tmp = f_stream;
	f_stream = malloc(sizeof(unsigned char)*f_stream_size*2);
	for (int k = 0; k < j; k++) {
//	  tmp[k] = f_stream[k];
	  f_stream[k] = tmp[k];
	}
	if (f_stream != NULL)
	  free(tmp);
	f_stream_size *= 2;
      }

      fread(&(f_stream[j]), 1, n_bytes, re);
      j += n_bytes;
      fread(buf, 1, 1, re);
      n_bytes = buf[0];
    }
    n_bytes = j;
    j = 0;
    offset = 0;
    cur_code_size = min_code_size;

    //the array that will hold the actual data
    size_t dat_stream_size = n_bytes*8/min_code_size + 1;
    size_t* dat_stream = malloc(sizeof(size_t)*dat_stream_size);

    //read the data out from the file and place each entry into an array
    while (offset < n_bytes*8) {
      if (ents == 0x01 << cur_code_size) {
	  cur_code_size++;
	  offset++;
#ifdef DEBUG_READ
	  printf("increased code size: now %d bits\n", cur_code_size);
#endif
      }
      code = read_stream(f_stream, offset, cur_code_size);
      dat_stream[j] = code;
#ifdef DEBUG_READ
      printf("%lu, ", code);
#endif
      offset += cur_code_size;
      ents++;
      //if we received a clear signal then reinitialize
      if (code == def_clear) {
	cur_code_size = min_code_size;
	ents = dict->imp_ents - 2;
      }
      j++;
    }
    free(f_stream);//we only need the dat_stream now

    size_t dat_stream_entries = j + 1;
    j = 0;

    //the initial step differs from later ones
    code = dat_stream[j];
    printf("Successfully decoded image data.\n");
    print_dict_debug(dict, code);
    //we don't care about an initial clear statement
    if (code == def_clear) {
      j++;
      code = dat_stream[j];
    print_dict_debug(dict, code);
    }
    i = out_to_img(ret, bpc, i, 1, pal, dict, code, 0);
    j++;
    //end of initial step
    while (j < dat_stream_entries && i < lw*lh) {
      prev_code = code;
      code = dat_stream[j];
      printf("i = %d, j = %d, ", i, j);
      print_dict_debug(dict, code);

      if (code == def_clear) {
	printf("Recieved clear dictionary signal.\n");
	delete_dictionary(dict);
	dict = create_dictionary(pal);
      } else if (code == def_end) {
	n_bytes = 0;//make sure we don't do any more looping 
	printf("Reached the end of the current image block.\n");
	break;
      //if we already have the code in our table
      } else if (code < dict->entries) {
	int n = get_code_len(dict, code);
	unsigned char K = get_pattern(dict, code, 0);
	save_to_dict(dict, 0, prev_code, K, n);
	i = out_to_img(ret, bpc, i, n, pal, dict, code, K);
      //if we don't have the code in our table
      } else {
	int n = get_code_len(dict, prev_code);
	unsigned char K = get_pattern(dict, prev_code, 0);
	save_to_dict(dict, code, prev_code, K, get_code_len(dict, prev_code));
	//i = out_to_img(ret, bpc, i, n, pal, dict, prev_code, K, 1);
	i = out_to_img(ret, bpc, i, n+1, pal, dict, code, K);
      }

      /*if (res > (0x01 << cur_code_size) || res == DICT_MSG_GREW){
	cur_code_size ++;
      }*/

      j++;
    }
    free(dat_stream);
    delete_dictionary(dict);
    if (pal != glob_pal) {
      delete_palatte(pal);
    }
  }

  delete_palatte(glob_pal);
  return ret;
}

#endif //IMGLOAD_H
