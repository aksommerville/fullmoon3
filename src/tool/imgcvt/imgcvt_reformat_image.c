#include "imgcvt_internal.h"

/* Determine whether we should emit with colorkey or opaque.
 */
 
static int imgcvt_choose_alpha() {

  // Command line trumps all, if not "auto".
  const char *opt=0;
  int optc=tool_cmdline_get_option(&opt,&imgcvt.cmdline,"alpha",5);
  if (optc>=0) {
    if ((optc==6)&&!memcmp(opt,"always",6)) return 1;
    if ((optc==5)&&!memcmp(opt,"never",5)) return 0;
    if ((optc!=4)||memcmp(opt,"auto",4)) {
      fprintf(stderr,
        "%s: Unexpected value '%.*s' for '--alpha'. Must be 'always', 'never', or 'auto'\n",
        imgcvt.cmdline.exename,optc,opt
      );
      return -2;
    }
  }
  
  // Image formats that don't have an alpha channel probably don't have any zeroes in the alpha channel.
  switch (imgcvt.image.fmt) {
    case FMN_IMAGE_FMT_Y1:
    case FMN_IMAGE_FMT_Y2:
    case FMN_IMAGE_FMT_Y4:
    case FMN_IMAGE_FMT_Y8:
    case FMN_IMAGE_FMT_Y16:
    case FMN_IMAGE_FMT_RGB8:
    case FMN_IMAGE_FMT_RGB16:
      return 0;
  }
  
  // Finally, read the whole image and return true if at least one pixel has zero alpha.
  // This seems expensive, and it is, but I feel the risk of saving images with an unintended alpha channel is pretty high.
  int y=imgcvt.image.h; while (y-->0) {
    int x=imgcvt.image.w; while (x-->0) {
      struct fmn_pixel pixel=fmn_image_get_pixel(&imgcvt.image,x,y);
      if (!pixel.a) return 1;
    }
  }
  
  // OK it's opaque.
  return 0;
}

/* Select output format.
 */
 
static int imgcvt_choose_format() {

  // Command line trumps all if present.
  const char *opt=0;
  int optc=tool_cmdline_get_option(&opt,&imgcvt.cmdline,"fmt",3);
  if (optc>=0) {
    if ((optc!=4)||memcmp(opt,"auto",4)) { // A pointless "auto" option to say "pretend I didn't say anything"
      #define _(tag) if ((optc==sizeof(#tag)-1)&&!memcmp(opt,#tag,optc)) return FMN_IMAGE_FMT_##tag;
      FMN_FOR_EACH_IMAGE_FMT
      FMN_FOR_EACH_EXTENDED_IMAGE_FMT
      #undef _
      fprintf(stderr,
        "%s: Unknown image format '%.*s'. Available:\n",
        imgcvt.cmdline.exename,optc,opt
      );
      #define _(tag) fprintf(stderr,"  %s\n",#tag);
      FMN_FOR_EACH_IMAGE_FMT
      FMN_FOR_EACH_EXTENDED_IMAGE_FMT
      #undef _
      return -2;
    }
  }
  
  // If it's already using a base format, keep it.
  switch (imgcvt.image.fmt) {
    #define _(tag) case FMN_IMAGE_FMT_##tag:
    FMN_FOR_EACH_IMAGE_FMT
    #undef _
    return imgcvt.image.fmt;
  }
  
  // Select the base format most amenable to input.
  switch (imgcvt.image.fmt) {
    case FMN_IMAGE_FMT_Y1:
      return FMN_IMAGE_FMT_Y2;
    case FMN_IMAGE_FMT_Y4:
    case FMN_IMAGE_FMT_Y16:
    case FMN_IMAGE_FMT_YA8:
    case FMN_IMAGE_FMT_YA16:
      return FMN_IMAGE_FMT_Y8;
    case FMN_IMAGE_FMT_RGB8:
    case FMN_IMAGE_FMT_RGB16:
    case FMN_IMAGE_FMT_RGBA16:
      return FMN_IMAGE_FMT_RGBA;
  }
  
  // Unknown? Some new format I forgot to call out here. Whatever, use RGBA.
  return FMN_IMAGE_FMT_RGBA;
}

/* Rewrite (imgcvt.image) in place, using the new format.
 * You must call this if alpha in play: Input uses an alpha channel but output uses a colorkey.
 */
 
static int imgcvt_rewrite_image(int dstfmt,int alpha) {

  struct fmn_image dst={0};
  if (fmn_image_init(&dst,dstfmt,imgcvt.image.w,imgcvt.image.h)<0) return -1;
  if (alpha) dst.flags|=FMN_IMAGE_FLAG_TRANSPARENT;
  
  int y=imgcvt.image.h; while (y-->0) {
    int x=imgcvt.image.w; while (x-->0) {
      struct fmn_pixel pixel=fmn_image_get_pixel(&imgcvt.image,x,y);
      fmn_image_set_pixel(&dst,x,y,pixel);
    }
  }

  free(imgcvt.image.v);
  imgcvt.image=dst;
  return 0;
}

/* Reformat image, main entry point.
 */
 
int imgcvt_reformat_image() {
  
  int alpha=imgcvt_choose_alpha();
  if (alpha<0) return alpha;
  
  int dstfmt=imgcvt_choose_format();
  if (dstfmt<0) return dstfmt;
  if (alpha||(dstfmt!=imgcvt.image.fmt)) {
    int err=imgcvt_rewrite_image(dstfmt,alpha);
    if (err<0) {
      if (err!=-2) fprintf(stderr,
        "%s: Unspecified error converting to format %d from %d\n",
        imgcvt.cmdline.srcpathv[0],dstfmt,imgcvt.image.fmt
      );
      return -2;
    }
  }
  
  return 0;
}
