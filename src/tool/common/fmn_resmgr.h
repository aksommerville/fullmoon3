/* fmn_resmgr.h
 * Uniform access to source resource files, ie src/data/.
 * Most tools don't use this, they get everything at the command line.
 * But available to those that need it, esp editor.
 *
 * There are some pecularities in naming of resource files.
 * Don't try to guess; use this API.
 */
 
#ifndef FMN_RESMGR_H
#define FMN_RESMGR_H

/* "restype" is a concept that only exists at compile time.
 * Resources at run time are not generic.
 */
#define FMN_RESTYPE_image     1
#define FMN_RESTYPE_song      2
#define FMN_RESTYPE_waves     3
#define FMN_RESTYPE_map       4
#define FMN_RESTYPE_tileprops 5 /* appendix; joins 'image' */
#define FMN_RESTYPE_adjust    6 /* appendix; joins 'song' */

#define FMN_FOR_EACH_RESTYPE \
  _(image) \
  _(song) \
  _(waves) \
  _(map) \
  _(tileprops) \
  _(adjust)
  
int fmn_restype_eval(const char *src,int srcc);
const char *fmn_restype_repr(int restype);

/* Discover the full set of resources by scanning the filesystem.
 * Ignores files of unknown resource type.
 */
int fmn_resmgr_list_files(
  int (*cb)(const char *path,int type,const char *name,int namec,void *userdata),
  void *userdata
);

#endif
