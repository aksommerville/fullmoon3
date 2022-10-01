/* fmn_fs.h
 */
 
#ifndef FMN_FS_H
#define FMN_FS_H

int fmn_file_read(void *dstpp,const char *path);
int fmn_file_write(const char *path,const void *src,int srcc);

#endif
