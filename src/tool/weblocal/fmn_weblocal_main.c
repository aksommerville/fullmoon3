/* fmn_weblocal_main.c
 * Web browsers are not so happy about loading Wasm etc via file scheme, what a nuisance.
 * So this is a trivial static web server we can use during development.
 * Goes without saying I'm sure, but ***this is not suitable for production use!***
 */

#include "fmn_weblocal_internal.h"
#include <signal.h>

struct fmn_weblocal fmn_weblocal={0};

/* Cleanup.
 */
 
static void fmn_weblocal_cleanup() {
  http_context_del(fmn_weblocal.http);
  if (fmn_weblocal.htdocsv) {
    while (fmn_weblocal.htdocsc-->0) free(fmn_weblocal.htdocsv[fmn_weblocal.htdocsc]);
    free(fmn_weblocal.htdocsv);
  }
  if (fmn_weblocal.host) free(fmn_weblocal.host);
  memset(&fmn_weblocal,0,sizeof(struct fmn_weblocal));
}

/* Signal handler.
 */
 
static void fmn_weblocal_rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++(fmn_weblocal.sigc)>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

/* Initialize HTTP server.
 */
 
static int fmn_weblocal_init_http() {

  if (!(fmn_weblocal.http=http_context_new())) return -1;
  
  if (http_context_serve_tcp(fmn_weblocal.http,fmn_weblocal.host,-1,fmn_weblocal.port)<0) {
    fprintf(stderr,"Failed to open server socket on %s:%d.\n",fmn_weblocal.host,fmn_weblocal.port);
    return -2;
  }
  
  if (
    !http_context_spawn_listener(fmn_weblocal.http,"GET","*",fmn_weblocal_serve_static,0)||
  0) return -1;
  
  return 0;
}

/* Argv.
 */
 
static void fmn_weblocal_print_help(const char *exename) {
  fprintf(stderr,"Usage: %s --htdocs=PATH [--htdocs=PATH...] --host=HOSTNAME --port=INTEGER\n",exename);
}

static int fmn_weblocal_add_htdocs(const char *src) {
  if (fmn_weblocal.htdocsc>=fmn_weblocal.htdocsa) {
    int na=fmn_weblocal.htdocsa+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(fmn_weblocal.htdocsv,sizeof(void*)*na);
    if (!nv) return -1;
    fmn_weblocal.htdocsv=nv;
    fmn_weblocal.htdocsa=na;
  }
  if (!(fmn_weblocal.htdocsv[fmn_weblocal.htdocsc]=strdup(src))) return -1;
  fmn_weblocal.htdocsc++;
  return 0;
}

static int fmn_weblocal_set_host(const char *src) {
  if (fmn_weblocal.host) free(fmn_weblocal.host);
  if (!(fmn_weblocal.host=strdup(src))) return -1;
  return 0;
}

static int fmn_weblocal_set_port(const char *src) {
  if ((fmn_int_eval(&fmn_weblocal.port,src,-1)<2)||(fmn_weblocal.port<1)||(fmn_weblocal.port>0xffff)) {
    fprintf(stderr,"Expected integer in 1..65535, found '%s'\n",src);
    return -1;
  }
  return 0;
}

/* Main.
 */
 
int main(int argc,char **argv) {
  int err;
  
  signal(SIGINT,fmn_weblocal_rcvsig);
  
  int argp=1; while (argp<argc) {
    const char *arg=argv[argp++];
    if (!strcmp(arg,"--help")) { fmn_weblocal_print_help(argv[0]); return 0; }
    if (!memcmp(arg,"--htdocs=",9)) { if (fmn_weblocal_add_htdocs(arg+9)<0) return 1; }
    else if (!memcmp(arg,"--host=",7)) { if (fmn_weblocal_set_host(arg+7)<0) return 1; }
    else if (!memcmp(arg,"--port=",7)) { if (fmn_weblocal_set_port(arg+7)<0) return 1; }
    else {
      fprintf(stderr,"%s: Unexpected argument '%s'\n",argv[0],arg);
      return 1;
    }
  }
  if (!fmn_weblocal.htdocsc) { fmn_weblocal_print_help(argv[0]); return 1; }
  if (!fmn_weblocal.host) fmn_weblocal_set_host("localhost");
  if (!fmn_weblocal.port) fmn_weblocal.port=8080;
  
  if ((err=fmn_weblocal_init_http())<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error initializing server.\n",argv[0]);
    fmn_weblocal_cleanup();
    return 1;
  }
  
  char serverdesc[256];
  int serverdescc=http_context_describe_servers(serverdesc,sizeof(serverdesc),fmn_weblocal.http);
  if ((serverdescc>0)&&(serverdescc<=sizeof(serverdesc))) {
    fprintf(stderr,"%s: Listening on %.*s. SIGINT to quit.\n",argv[0],serverdescc,serverdesc);
  } else {
    fprintf(stderr,"%s: Running but unable to describe servers. None attached? Anywho, SIGINT to quit.\n",argv[0]);
  }
  
  while (1) {
    if (fmn_weblocal.sigc) {
      fprintf(stderr,"%s: Terminate due to SIGINT.\n",argv[0]);
      break;
    }
    if (http_context_update(fmn_weblocal.http,1000)<0) {
      fprintf(stderr,"%s: Error updating server.\n",argv[0]);
      fmn_weblocal_cleanup();
      return 1;
    }
  }
  
  fmn_weblocal_cleanup();
  return 0;
}
