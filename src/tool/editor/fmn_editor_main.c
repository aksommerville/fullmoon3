/* fmn_editor_main.c
 * The editor's important parts are a web app, see src/tool/editor/www/.
 * This C app is mostly a static HTTP file server, plus a few Full Moon specific REST services:
 *   GET /api/restoc : List of resources [{type,name,path},...]
 *   GET /api/resall : Every resource including payload [{type,name,path,base64|text|desc},...] (desc) explains why we didn't load it.
 *   GET,PUT,DELETE /src/* : Direct access to src/
 *   GET,PUT,DELETE /* : Direct access to src/tool/editor/www/
 */

#include "fmn_editor_internal.h"
#include <signal.h>

struct fmn_editor fmn_editor={0};

/* Cleanup.
 */
 
static void fmn_editor_cleanup() {
  http_context_del(fmn_editor.http);
  memset(&fmn_editor,0,sizeof(struct fmn_editor));
}

/* Signal handler.
 */
 
static void fmn_editor_rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++(fmn_editor.sigc)>=3) {
        fprintf(stderr,"Too many unprocessed signals.\n");
        exit(1);
      } break;
  }
}

/* Initialize HTTP server.
 */
 
static int fmn_editor_init_http() {

  if (!(fmn_editor.http=http_context_new())) return -1;
  
  // TODO Configurable host and port.
  const char *host="localhost";
  int port=8080;
  if (http_context_serve_tcp(fmn_editor.http,host,-1,port)<0) {
    fprintf(stderr,"Failed to open server socket on %s:%d.\n",host,port);
    return -2;
  }
  
  if (
    !http_context_spawn_listener(fmn_editor.http,"GET","/api/restoc",fmn_editor_GET_restoc,0)||
    !http_context_spawn_listener(fmn_editor.http,"GET","/api/resall",fmn_editor_GET_resall,0)||
    !http_context_spawn_listener(fmn_editor.http,"","*",fmn_editor_serve_static,0)|| // GET,PUT,DELETE
  0) return -1;
  
  return 0;
}

/* Main.
 */
 
int main(int argc,char **argv) {
  int err;
  
  signal(SIGINT,fmn_editor_rcvsig);
  
  if ((err=fmn_editor_init_http())<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error initializing server.\n",argv[0]);
    fmn_editor_cleanup();
    return 1;
  }
  
  char serverdesc[256];
  int serverdescc=http_context_describe_servers(serverdesc,sizeof(serverdesc),fmn_editor.http);
  if ((serverdescc>0)&&(serverdescc<=sizeof(serverdesc))) {
    fprintf(stderr,"%s: Listening on %.*s. SIGINT to quit.\n",argv[0],serverdescc,serverdesc);
  } else {
    fprintf(stderr,"%s: Running but unable to describe servers. None attached? Anywho, SIGINT to quit.\n",argv[0]);
  }
  
  while (1) {
    if (fmn_editor.sigc) {
      fprintf(stderr,"%s: Terminate due to SIGINT.\n",argv[0]);
      break;
    }
    if (http_context_update(fmn_editor.http,1000)<0) {
      fprintf(stderr,"%s: Error updating server.\n",argv[0]);
      fmn_editor_cleanup();
      return 1;
    }
  }
  
  fmn_editor_cleanup();
  return 0;
}
