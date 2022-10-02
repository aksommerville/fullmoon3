#include "tool_cmdline.h"
#include "opt/serial/fmn_serial.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

/* Cleanup.
 */
 
static void tool_option_cleanup(struct tool_option *option) {
  if (option->k) free(option->k);
  if (option->v) free(option->v);
}

void tool_cmdline_cleanup(struct tool_cmdline *cmdline) {
  if (!cmdline) return;
  if (cmdline->srcpathv) {
    while (cmdline->srcpathc-->0) free(cmdline->srcpathv[cmdline->srcpathc]);
    free(cmdline->srcpathv);
  }
  if (cmdline->dstpath) free(cmdline->dstpath);
  if (cmdline->optionv) {
    while (cmdline->optionc-->0) tool_option_cleanup(cmdline->optionv+cmdline->optionc);
    free(cmdline->optionv);
  }
}

/* Set output path.
 */
 
static int tool_cmdline_set_output(struct tool_cmdline *cmdline,const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (!srcc) return -1;
  if (cmdline->dstpath) {
    fprintf(stderr,"%s: Multiple output paths.\n",cmdline->exename);
    return -2;
  }
  if (!(cmdline->dstpath=malloc(srcc+1))) return -1;
  memcpy(cmdline->dstpath,src,srcc);
  cmdline->dstpath[srcc]=0;
  return 0;
}

/* Add input path.
 */
 
static int tool_cmdline_add_srcpath(struct tool_cmdline *cmdline,const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (!srcc) return -1;
  
  if (cmdline->srcpathc>=cmdline->srcpatha) {
    int na=cmdline->srcpatha+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(cmdline->srcpathv,sizeof(void*)*na);
    if (!nv) return -1;
    cmdline->srcpathv=nv;
    cmdline->srcpatha=na;
  }
  
  char *nv=malloc(srcc+1);
  if (!nv) return -1;
  memcpy(nv,src,srcc);
  nv[srcc]=0;
  
  cmdline->srcpathv[cmdline->srcpathc++]=nv;
  return 0;
}

/* --help, if the app doesn't implement it.
 */
 
static void tool_cmdline_print_generic_help(const char *exename) {
  fprintf(stderr,"Usage: %s -oOUTPUT -iINPUT [OPTIONS]\n",exename);
}

/* Receive option.
 */
 
static int tool_cmdline_add_option(struct tool_cmdline *cmdline,const char *k,int kc,const char *v,int vc) {
  if (!k) kc=0; else if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!v) vc=0; else if (vc<0) { vc=0; while (v[vc]) vc++; }
  
  /* There's a few options we handle automatically.
   */
  if ((kc==1)&&!memcmp(k,"i",1)) return tool_cmdline_add_srcpath(cmdline,v,vc);
  if ((kc==1)&&!memcmp(k,"o",1)) return tool_cmdline_set_output(cmdline,v,vc);
  if ((kc==2)&&!memcmp(k,"in",2)) return tool_cmdline_add_srcpath(cmdline,v,vc);
  if ((kc==3)&&!memcmp(k,"out",3)) return tool_cmdline_set_output(cmdline,v,vc);
  if ((kc==4)&&!memcmp(k,"help",4)) {
    if (cmdline->print_help) cmdline->print_help(cmdline->exename);
    else tool_cmdline_print_generic_help(cmdline->exename);
    return -2;
  }
  
  if (cmdline->optionc>=cmdline->optiona) {
    int na=cmdline->optiona+8;
    if (na>INT_MAX/sizeof(struct tool_option)) return -1;
    void *nv=realloc(cmdline->optionv,sizeof(struct tool_option)*na);
    if (!nv) return -1;
    cmdline->optionv=nv;
    cmdline->optiona=na;
  }
  
  char *nk=malloc(kc+1);
  if (!nk) return -1;
  char *nv=malloc(vc+1);
  if (!nv) { free(nk); return -1; }
  memcpy(nk,k,kc);
  nk[kc]=0;
  memcpy(nv,v,vc);
  nv[vc]=0;
  int vn=0;
  if (fmn_int_eval(&vn,v,vc)<1) vn=0;
  
  struct tool_option *option=cmdline->optionv+cmdline->optionc++;
  option->k=nk;
  option->kc=kc;
  option->v=nv;
  option->vc=vc;
  option->vn=vn;
  return 0;
}

/* Receive argv.
 */

int tool_cmdline_argv(struct tool_cmdline *cmdline,int argc,char **argv) {
  int argi=0,err;
  
  if (argi<argc) {
    cmdline->exename=argv[argi++];
  }
  if (!cmdline->exename||!cmdline->exename[0]) cmdline->exename="(fullmoon tool)";
  
  while (argi<argc) {
    const char *arg=argv[argi++];
    
    if (!arg||!arg[0]) goto _unexpected_;
    
    if (arg[0]!='-') {
      if ((err=tool_cmdline_add_srcpath(cmdline,arg,-1))<0) return err;
      continue;
    }
    
    if (!arg[1]) goto _unexpected_;
    
    if (arg[1]!='-') {
      const char *k=arg+1;
      const char *v=arg+2;
      if (!v[0]&&(argi<argc)&&argv[argi]&&argv[argi][0]&&(argv[argi][0]!='-')) v=argv[argi++];
      if ((err=tool_cmdline_add_option(cmdline,k,1,v,-1))<0) {
        if (err!=-2) fprintf(stderr,"%s: Unspecified error receiving option '%c'='%s'\n",cmdline->exename,k[0],v);
        return -2;
      }
      continue;
    }
    
    while (arg[0]=='-') arg++;
    if (!arg[0]) goto _unexpected_;
    
    const char *k=arg,*v;
    int kc=0;
    while (k[kc]&&(k[kc]!='=')) kc++;
    if (k[kc]=='=') v=k+kc+1;
    else if ((argi<argc)&&argv[argi]&&argv[argi][0]&&(argv[argi][0]!='-')) v=argv[argi++];
    if ((err=tool_cmdline_add_option(cmdline,k,kc,v,-1))<0) {
      if (err!=-2) fprintf(stderr,"%s: Unspecified error receiving option '%.*s'='%s'\n",cmdline->exename,kc,k,v);
      return -2;
    }
    continue;
    
   _unexpected_:;
    fprintf(stderr,"%s: Unexpected argument '%s'\n",cmdline->exename,arg);
    return -2;
  }
  return 0;
}

/* Assertions.
 */

int tool_cmdline_one_input(struct tool_cmdline *cmdline) {
  if (cmdline->srcpathc<1) {
    fprintf(stderr,"%s: No inputs. Please specify with '-iPATH'\n",cmdline->exename);
    return -2;
  } else if (cmdline->srcpathc>1) {
    fprintf(stderr,"%s: Multiple inputs. We can only do one.\n",cmdline->exename);
    return -2;
  }
  return 0;
}

int tool_cmdline_one_or_more_input(struct tool_cmdline *cmdline) {
  if (cmdline->srcpathc<1) {
    fprintf(stderr,"%s: No inputs. Please specify with '-iPATH'\n",cmdline->exename);
    return -2;
  }
  return 0;
}

int tool_cmdline_one_output(struct tool_cmdline *cmdline) {
  if (!cmdline->dstpath) {
    fprintf(stderr,"%s: Please indicate output path with '-oPATH'\n",cmdline->exename);
    return -2;
  }
  return 0;
}

/* Get option.
 */
 
struct tool_option *tool_cmdline_get_option_struct(const struct tool_cmdline *cmdline,const char *k,int kc) {
  if (!cmdline) return 0;
  if (!k) return 0;
  if (kc<0) { kc=0; while (k[kc]) kc++; }
  if (!kc) return 0;
  struct tool_option *option=cmdline->optionv;
  int i=cmdline->optionc;
  for (;i-->0;option++) {
    if (option->kc!=kc) continue;
    if (memcmp(option->k,k,kc)) continue;
    return option;
  }
  return 0;
}
 
int tool_cmdline_get_option(void *dstpp,const struct tool_cmdline *cmdline,const char *k,int kc) {
  struct tool_option *option=tool_cmdline_get_option_struct(cmdline,k,kc);
  if (!option) return -1;
  if (dstpp) *(void**)dstpp=option->v;
  return option->vc;
}

int tool_cmdline_get_option_int(const struct tool_cmdline *cmdline,const char *k,int kc) {
  struct tool_option *option=tool_cmdline_get_option_struct(cmdline,k,kc);
  if (!option) return -1;
  return option->vn;
}

int tool_cmdline_get_option_boolean(const struct tool_cmdline *cmdline,const char *k,int kc) {
  struct tool_option *option=tool_cmdline_get_option_struct(cmdline,k,kc);
  if (!option) return 0;
  if (!option->vc) return 1;
  if (option->vn) return 1; // Nonzero (vn) means it's an integer (and not zero)
  
  // We'd be remiss in calling ourselves "boolean" if we don't accept "true" and "false"...
  if ((option->vc==4)&&!memcmp(option->v,"true",4)) return 1;
  if ((option->vc==5)&&!memcmp(option->v,"false",5)) return 0;
  
  if (option->v[0]=='0') return 0; // assume it's a well formed zero
  return -1; // assume malformed
}
