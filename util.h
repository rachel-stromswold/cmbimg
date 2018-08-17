#ifndef UTIL_H
#define UTIL_H

/*
 * =============================== STRUCT STR_LIST ===============================
 */

struct str_list {
  char* dat;
  int n;
  struct str_list* next;
};

struct str_list* create_list(size_t len) {
  struct str_list* ret = (struct str_list*)malloc(sizeof(struct str_list*));
  ret->dat = (char*)malloc(sizeof(char)*len);
  ret->next = NULL;
  return ret;
}

void delete_list(struct str_list* lst) {
  if (lst != NULL) {
    if (lst->dat != NULL) {
      free(lst->dat);
    }
    delete_list(lst->next);
    free(lst);
  }
}

struct str_list* append_empty(struct str_list* lst, size_t len) {
  lst->next = create_list(len);
  return lst->next;
}

/*
 * =============================== STRUCT FLAGS ===============================
 */
struct flags {
  char merge_mode;
  int columns;
  int num_pics;
  char* outputName;
  struct str_list* pre_proc_args;
};

//for a string of the form var1=abc, var2=def, ... parse_string_args will return a string from the appropriate variable
//i.e. parse_string_args("var1=abc, var2=def", "var1") will return "abc"
//NOTE: the caller is responsible for freeing the returned string
char* parse_string_args(char* str, char* var_name) {
  size_t len = strlen(str);
  size_t varlen = strlen(var_name);
  for (size_t i = 0; i < len; i++) {
    for (size_t j = 0; i+j < len && str[i+j] == var_name[j] && j < varlen; j++) {
      if (var_name[j+1] == 0 && str[i+j+1] == '=') {
	char* ret = (char*)malloc(sizeof(char)*(len - i - j));
	i = i+j+2;
	int k = 0;
	while (str[i] != 0 && str[i] != ',' && str[i] != ';') {
	  ret[k] = str[i];
	  i++;
	  k++;
	}
	ret[k] = 0;
	return ret;
      }
    }
  }
  
  return NULL;
}
#endif
