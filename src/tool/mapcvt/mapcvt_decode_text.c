#include "mapcvt_internal.h"
#include "api/fmn_common.h"
#include "game/map/fmn_map.h"

/* Flag names.
 * Not sure how I'm going implement these... C objects? compile-time-only symbols?
 * At least, we're going to do something better than plain numbers.
 */
 
static int mapcvt_flagid_eval(const char *src,int srcc) {
  int n;
  if (fmn_int_eval(&n,src,srcc)>=2) return n;
  return -1;
}

/* Intern to external references.
 */
 
static int mapcvt_refv_intern(const char *prefix,const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (!prefix) prefix="";
  char name[256];
  int namec=snprintf(name,sizeof(name),"%s%.*s",prefix,srcc,src);
  if ((namec<1)||(namec>=sizeof(name))) return -1;
  int i=mapcvt.refc;
  while (i-->0) {
    if (!strcmp(name,mapcvt.refv[i])) return i;
  }
  if (mapcvt.refc>=0xff) return -1;
  if (mapcvt.refc>=mapcvt.refa) {
    int na=mapcvt.refa+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(mapcvt.refv,sizeof(void*)*na);
    if (!nv) return -1;
    mapcvt.refv=nv;
    mapcvt.refa=na;
  }
  char *nv=malloc(namec+1);
  if (!nv) return -1;
  memcpy(nv,name,namec);
  nv[namec]=0;
  mapcvt.refv[mapcvt.refc]=nv;
  return mapcvt.refc++;
}

/* General tokenization.
 * After the last few data converters, I'm getting tired of ad-hoc parsing per command.
 */
 
struct mapcvt_token {
  const char *src;
  int srcc;
  int v;
  int vok;
};

/* Simple object-reference commands: tilesheet,song,neighbor*,home
 */
 
static int mapcvt_cmd_single_object(
  const char *prefix,
  uint8_t cmd,
  const struct mapcvt_token *tokenv,int tokenc,
  const char *path,int lineno
) {
  if (tokenc!=1) {
    fprintf(stderr,"%s:%d: Expected a single argument\n",path,lineno);
    return -2;
  }
  int refid=mapcvt_refv_intern(prefix,tokenv[0].src,tokenv[0].srcc);
  if (refid<0) {
    if (mapcvt.refc>=0xff) fprintf(stderr,"%s:%d: Too many external references, limit 255\n",path,lineno);
    else fprintf(stderr,"%s:%d: Failed to add external refernce\n",path,lineno);
    return -2;
  }
  uint8_t serial[]={cmd,refid};
  if (fmn_encode_raw(&mapcvt.bin,serial,sizeof(serial))<0) return -1;
  return 0;
}
 
static int mapcvt_cmd_tilesheet(const struct mapcvt_token *tokenv,int tokenc,const char *path,int lineno) {
  return mapcvt_cmd_single_object("fmnr_image_",FMN_MAP_CMD_TILESHEET,tokenv,tokenc,path,lineno);
}
 
static int mapcvt_cmd_song(const struct mapcvt_token *tokenv,int tokenc,const char *path,int lineno) {
  return mapcvt_cmd_single_object("fmnr_song_",FMN_MAP_CMD_SONG,tokenv,tokenc,path,lineno);
}
 
static int mapcvt_cmd_neighborw(const struct mapcvt_token *tokenv,int tokenc,const char *path,int lineno) {
  return mapcvt_cmd_single_object("fmnr_map_",FMN_MAP_CMD_NEIGHBORW,tokenv,tokenc,path,lineno);
}
 
static int mapcvt_cmd_neighbore(const struct mapcvt_token *tokenv,int tokenc,const char *path,int lineno) {
  return mapcvt_cmd_single_object("fmnr_map_",FMN_MAP_CMD_NEIGHBORE,tokenv,tokenc,path,lineno);
}
 
static int mapcvt_cmd_neighborn(const struct mapcvt_token *tokenv,int tokenc,const char *path,int lineno) {
  return mapcvt_cmd_single_object("fmnr_map_",FMN_MAP_CMD_NEIGHBORN,tokenv,tokenc,path,lineno);
}
 
static int mapcvt_cmd_neighbors(const struct mapcvt_token *tokenv,int tokenc,const char *path,int lineno) {
  return mapcvt_cmd_single_object("fmnr_map_",FMN_MAP_CMD_NEIGHBORS,tokenv,tokenc,path,lineno);
}
 
static int mapcvt_cmd_home(const struct mapcvt_token *tokenv,int tokenc,const char *path,int lineno) {
  return mapcvt_cmd_single_object("fmnr_map_",FMN_MAP_CMD_HOME,tokenv,tokenc,path,lineno);
}

/* "hero"
 */
 
static int mapcvt_cmd_hero(const struct mapcvt_token *tokenv,int tokenc,const char *path,int lineno) {
  if ((tokenc!=2)||!tokenv[0].vok||!tokenv[1].vok) {
    fprintf(stderr,"%s:%d: Expected 'X Y' after 'hero'\n",path,lineno);
    return -2;
  }
  if (
    (tokenv[0].v<0)||(tokenv[0].v>=FMN_COLC)||
    (tokenv[1].v<0)||(tokenv[1].v>=FMN_ROWC)
  ) {
    fprintf(stderr,
      "%s:%d: Hero position must be in 0..(%d,%d)-1, found (%d,%d)\n",
      path,lineno,FMN_COLC,FMN_ROWC,tokenv[0].v,tokenv[1].v
    );
    return -2;
  }
  uint8_t cmd[]={FMN_MAP_CMD_HERO,(tokenv[0].v<<4)|tokenv[1].v};
  if (fmn_encode_raw(&mapcvt.bin,cmd,sizeof(cmd))<0) return -1;
  return 0;
}

/* "event" EVID HOOKID [ARGS...]
 */
 
static int mapcvt_cmd_event(const struct mapcvt_token *tokenv,int tokenc,const char *path,int lineno) {
  fprintf(stderr,"TODO %s\n",__func__);//TODO
  uint8_t cmd[]={FMN_MAP_CMD_EVENT1,0,0,0,0};
  if (fmn_encode_raw(&mapcvt.bin,cmd,sizeof(cmd))<0) return -1;
  return 0;
}

/* "cellif" FLAGID X Y TILEID
 */
 
static int mapcvt_cmd_cellif(const struct mapcvt_token *tokenv,int tokenc,const char *path,int lineno) {
  if ((tokenc!=4)||!tokenv[1].vok||!tokenv[2].vok||!tokenv[3].vok) {
    fprintf(stderr,
      "%s:%d: Expected 'FLAGID X Y TILEID' after 'cellif'\n",
      path,lineno
    );
    return -2;
  }
  int flagid=mapcvt_flagid_eval(tokenv[0].src,tokenv[0].srcc);
  if ((flagid<0)||(flagid>=0x100)) {
    fprintf(stderr,
      "%s:%d: Failed to evaluate '%.*s' as flag ID\n",
      path,lineno,tokenv[0].srcc,tokenv[0].src
    );
    return -2;
  }
  if (
    (tokenv[1].v<0)||(tokenv[1].v>=FMN_COLC)||
    (tokenv[2].v<0)||(tokenv[2].v>=FMN_ROWC)||
    (tokenv[3].v<0)||(tokenv[3].v>0xff)
  ) {
    fprintf(stderr,
      "%s:%d: cellif params (%d,%d,%d) out of range 0..(%d,%d,%d)-1\n",
      path,lineno,tokenv[1].v,tokenv[2].v,tokenv[3].v,FMN_COLC,FMN_ROWC,0x100
    );
    return -2;
  }
  uint8_t cmd[]={FMN_MAP_CMD_CELLIF,flagid,(tokenv[1].v<<4)|tokenv[2].v,tokenv[3].v,0};
  if (fmn_encode_raw(&mapcvt.bin,cmd,sizeof(cmd))<0) return -1;
  return 0;
}

/* "door" X Y MAPID DSTX DSTY
 */
 
static int mapcvt_cmd_door(const struct mapcvt_token *tokenv,int tokenc,const char *path,int lineno) {
  if (
    (tokenc!=5)||
    !tokenv[0].vok||!tokenv[1].vok||
    !tokenv[3].vok||!tokenv[4].vok||
    (tokenv[0].v<0)||(tokenv[0].v>=FMN_COLC)||
    (tokenv[1].v<0)||(tokenv[1].v>=FMN_ROWC)||
    (tokenv[3].v<0)||(tokenv[3].v>=FMN_COLC)||
    (tokenv[4].v<0)||(tokenv[4].v>=FMN_ROWC)
  ) {
    fprintf(stderr,
      "%s:%d: Expected 'X Y MAPID DSTX DSTY' after 'door'\n",
      path,lineno
    );
    return -2;
  }
  int refid=mapcvt_refv_intern("fmnr_map_",tokenv[2].src,tokenv[2].srcc);
  if (refid<0) {
    if (mapcvt.refc>=0xff) fprintf(stderr,"%s:%d: Too many external references, limit 255\n",path,lineno);
    else fprintf(stderr,"%s:%d: Failed to add external refernce for map '%.*s'\n",path,lineno,tokenv[2].srcc,tokenv[2].src);
    return -2;
  }
  uint8_t srcpos=(tokenv[0].v<<4)|tokenv[1].v;
  uint8_t dstpos=(tokenv[3].v<<4)|tokenv[4].v;
  uint8_t cmd[]={FMN_MAP_CMD_DOOR,srcpos,refid,dstpos,0};
  if (fmn_encode_raw(&mapcvt.bin,cmd,sizeof(cmd))<0) return -1;
  return 0;
}

/* "sprite" X Y SPRITECTLID [ARGS...]
 */
 
static int mapcvt_cmd_sprite(const struct mapcvt_token *tokenv,int tokenc,const char *path,int lineno) {
  if (tokenc<3) {
    fprintf(stderr,
      "%s:%d: Expected 'X Y SPRITECTLID [ARGS...]' after 'sprite'\n",
      path,lineno
    );
    return -2;
  }
  if (!tokenv[0].vok||!tokenv[1].vok||(tokenv[0].v<0)||(tokenv[1].v<0)||(tokenv[0].v>=FMN_COLC)||(tokenv[1].v>=FMN_ROWC)) {
    fprintf(stderr,
      "%s:%d: Invalid sprite position '%.*s','%.*s', must be integers <(%d,%d)\n",
      path,lineno,tokenv[0].srcc,tokenv[0].src,tokenv[1].srcc,tokenv[1].src,FMN_COLC,FMN_ROWC
    );
    return -2;
  }
  int i=3; for (;i<tokenc;i++) {
    if (!tokenv[i].vok||(tokenv[i].v<0)||(tokenv[i].v>0xff)) {
      fprintf(stderr,
        "%s:%d: sprite params must be in 0..255, found '%.*s'\n",
        path,lineno,tokenv[i].srcc,tokenv[i].src
      );
      return -2;
    }
  }
  if (tokenc>7) {
    fprintf(stderr,
      "%s:%d:WARNING: Limit 4 sprite args. Ignoring %d\n",
      path,lineno,tokenc-7
    );
    tokenc=8;
  }
  int spritectlid=mapcvt_refv_intern("fmn_sprite_type_",tokenv[2].src,tokenv[2].srcc);
  if (spritectlid<0) {
    fprintf(stderr,
      "%s:%d: Invalid sprite name '%.*s'\n",
      path,lineno,tokenv[2].srcc,tokenv[2].src
    );
    return -2;
  }
  uint8_t cmd[7]={FMN_MAP_CMD_SPRITE,(tokenv[0].v<<4)|tokenv[1].v,spritectlid};
  for (i=3;i<tokenc;i++) cmd[i]=tokenv[i].v;
  if (fmn_encode_raw(&mapcvt.bin,cmd,sizeof(cmd))<0) return -1;
  return 0;
}

/* Decode a command.
 */
 
static int mapcvt_decode_command(const char *src,int srcc,const char *path,int lineno) {
  #define tokena 16 /* Keep this large enough for the longest legal command. */
  struct mapcvt_token tokenv[tokena];
  int tokenc=0,srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    if (tokenc>=tokena) {
      fprintf(stderr,
        "%s:%d: Too many tokens, limit %d\n",
        path,lineno,tokena
      );
      return -2;
    }
    struct mapcvt_token *token=tokenv+tokenc++;
    token->src=src+srcp;
    token->srcc=0;
    while ((srcp<srcc)&&((unsigned char)src[srcp]>0x20)) { srcp++; token->srcc++; }
    if (fmn_int_eval(&token->v,token->src,token->srcc)<2) token->vok=0;
    else token->vok=1;
  }
  #undef tokena
  if (tokenc<1) return 0;
  
  const char *kw=tokenv[0].src;
  int kwc=tokenv[0].srcc;
  if ((kwc==9)&&!memcmp(kw,"tilesheet",9)) return mapcvt_cmd_tilesheet(tokenv+1,tokenc-1,path,lineno);
  if ((kwc==4)&&!memcmp(kw,"song",4)) return mapcvt_cmd_song(tokenv+1,tokenc-1,path,lineno);
  if ((kwc==4)&&!memcmp(kw,"hero",4)) return mapcvt_cmd_hero(tokenv+1,tokenc-1,path,lineno);
  if ((kwc==9)&&!memcmp(kw,"neighborw",9)) return mapcvt_cmd_neighborw(tokenv+1,tokenc-1,path,lineno);
  if ((kwc==9)&&!memcmp(kw,"neighbore",9)) return mapcvt_cmd_neighbore(tokenv+1,tokenc-1,path,lineno);
  if ((kwc==9)&&!memcmp(kw,"neighborn",9)) return mapcvt_cmd_neighborn(tokenv+1,tokenc-1,path,lineno);
  if ((kwc==9)&&!memcmp(kw,"neighbors",9)) return mapcvt_cmd_neighbors(tokenv+1,tokenc-1,path,lineno);
  if ((kwc==4)&&!memcmp(kw,"home",4)) return mapcvt_cmd_home(tokenv+1,tokenc-1,path,lineno);
  if ((kwc==5)&&!memcmp(kw,"event",5)) return mapcvt_cmd_event(tokenv+1,tokenc-1,path,lineno);
  if ((kwc==6)&&!memcmp(kw,"cellif",6)) return mapcvt_cmd_cellif(tokenv+1,tokenc-1,path,lineno);
  if ((kwc==4)&&!memcmp(kw,"door",4)) return mapcvt_cmd_door(tokenv+1,tokenc-1,path,lineno);
  if ((kwc==6)&&!memcmp(kw,"sprite",6)) return mapcvt_cmd_sprite(tokenv+1,tokenc-1,path,lineno);
  
  fprintf(stderr,"%s:%d: Unknown command '%.*s'\n",path,lineno,kwc,kw);
  return 0;
}

/* Decode a row of cells.
 */
 
static int mapcvt_decode_cells(const char *src,int srcc,const char *path,int lineno) {
  if (srcc!=FMN_COLC<<1) {
    fprintf(stderr,
      "%s:%d: Cells row length must be exactly %d, found %d\n",
      path,lineno,FMN_COLC<<1,srcc
    );
    return -2;
  }
  
  uint8_t v[FMN_COLC];
  uint8_t *dst=v;
  for (;srcc>=2;srcc-=2,src+=2,dst++) {
    int hi=fmn_digit_eval(src[0]);
    if ((hi<0)||(hi>=0x10)) {
      fprintf(stderr,"%s:%d: Unexpected char '%c', must be hexadecimal digits only\n",path,lineno,src[0]);
      return -2;
    }
    int lo=fmn_digit_eval(src[1]);
    if ((lo<0)||(lo>=0x10)) {
      fprintf(stderr,"%s:%d: Unexpected char '%c', must be hexadecimal digits only\n",path,lineno,src[1]);
      return -2;
    }
    *dst=(hi<<4)|lo;
  }
  
  if (fmn_encode_raw(&mapcvt.bin,v,sizeof(v))<0) return -1;
  
  return 0;
}

/* Decode text.
 */
 
int mapcvt_decode_text(const char *src,int srcc,const char *path) {
  mapcvt.bin.c=0;
  while (mapcvt.refc>0) {
    mapcvt.refc--;
    free(mapcvt.refv[mapcvt.refc]);
  }
  
  int srcp=0,lineno=0,err;
  while (srcp<srcc) {
    lineno++;
    const char *line=src+srcp;
    int linec=0,comment=0;
    while (srcp<srcc) {
      if (src[srcp]==0x0a) { srcp++; break; }
      if (src[srcp]=='#') comment=1;
      else if (!comment) linec++;
      srcp++;
    }
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    
    if (lineno<=FMN_ROWC) {
      if ((err=mapcvt_decode_cells(line,linec,path,lineno))<0) {
        if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error decoding cells row\n",path,lineno);
        return -2;
      }
    } else if (!linec) {
    } else {
      if ((err=mapcvt_decode_command(line,linec,path,lineno))<0) {
        if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error decoding command\n",path,lineno);
        return -2;
      }
    }
  }
  
  if (mapcvt.bin.c<FMN_COLC*FMN_ROWC) {
    fprintf(stderr,"%s: Incomplete cells\n",path);
    return -2;
  }
  
  if (fmn_encode_u8(&mapcvt.bin,0)<0) return -1; // terminate command list (important!)
  return 0;
}
