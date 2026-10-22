#ifndef BLAZE_FILES_H
#define BLAZE_FILES_H
#include <stdio.h>
#include <stddef.h>
/* UTF-8 paths. Writes always use exact names; only immutable asset reads may
   resolve a unique ASCII case-insensitive match on case-sensitive filesystems. */
FILE *blaze_fopen(const char *path,const char *mode);
FILE *blaze_asset_open(const char *path);
int blaze_asset_path(char *out,size_t capacity,const char *path);
int blaze_file_replace(const char *from,const char *to);
int blaze_file_remove(const char *path);
#endif
