/* fmn_fs.h
 */
 
#ifndef FMN_FS_H
#define FMN_FS_H

int fmn_file_read(void *dstpp,const char *path);
int fmn_file_write(const char *path,const void *src,int srcc);
int fmn_file_rm(const char *path);
char fmn_file_get_type(const char *path);
int fmn_dir_read(const char *path,int (*cb)(const char *path,const char *base,char type,void *userdata),void *userdata);

#endif
