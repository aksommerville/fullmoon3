/* http.h
 * Minimal HTTP client and server.
 *  - No encryption.
 *  - Not hardened for security. It won't take much imagination to break it, I'm sure.
 *  - Not much optimization, eg no cache of static files to serve.
 *  - We don't correctly handled continued headers, footers, odd cases like that.
 */
 
#ifndef HTTP_H
#define HTTP_H

struct fmn_encoder;
struct fmn_decoder;
struct http_context;
struct http_listener;
struct http_conn;
struct http_xfer;

/* Context.
 * There should be just one per program run, for clients and servers alike.
 ************************************************************************/
 
void http_context_del(struct http_context *context);
int http_context_ref(struct http_context *context);
struct http_context *http_context_new();

int http_context_serve_tcp(
  struct http_context *context,
  const char *host,int hostc,
  int port
);

int http_context_update(struct http_context *context,int to_ms);

int http_context_add_listener(struct http_context *context,struct http_listener *listener);
int http_context_add_conn(struct http_context *context,struct http_conn *conn);

// Convenience. Returns a WEAK reference to the new listener.
struct http_listener *http_context_spawn_listener(
  struct http_context *context,
  const char *method,const char *path,
  int (*serve)(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp),
  void *userdata
);

/* Generates a log-friendly string describing any listening servers, eg "localhost:8080".
 */
int http_context_describe_servers(char *dst,int dsta,const struct http_context *context);

struct http_listener *http_context_find_listener_for_request(const struct http_context *context,const struct http_xfer *req);

/* Listener.
 * All listeners listen on all server sockets.
 * Listeners are tested in the order you add them.
 * So if you have a fallback match-all listener, add it last.
 ***********************************************************************/
 
void http_listener_del(struct http_listener *listener);
int http_listener_ref(struct http_listener *listener);

struct http_listener *http_listener_new(
  int (*serve)(struct http_listener *listener,struct http_xfer *req,struct http_xfer *rsp),
  void *userdata
);

void *http_listener_get_userdata(const struct http_listener *listener);
struct http_context *http_listener_get_context(const struct http_listener *listener);

// '*' matches any amount of anything. Empty matches everything.
int http_listener_set_path(struct http_listener *listener,const char *path,int pathc);

// Empty matches everything. Case-insensitive.
// You can match all methods, or just one. Can't do multiple specific methods.
int http_listener_set_method(struct http_listener *listener,const char *method,int methodc);

int http_listener_match_request(const struct http_listener *listener,const struct http_xfer *req);

/* Connection.
 * Wrapper around a socket, either server or stream.
 *********************************************************************/
 
#define HTTP_CONN_ROLE_SERVER    1
#define HTTP_CONN_ROLE_STREAM    2
 
void http_conn_del(struct http_conn *conn);
int http_conn_ref(struct http_conn *conn);

struct http_conn *http_conn_new();

int http_conn_listen_tcp(struct http_conn *conn,const char *host,int hostc,int port);
int http_conn_set_fd(struct http_conn *conn,int fd,int role); // HANDOFF
int http_conn_set_saddr(struct http_conn *conn,const void *saddr,int saddrc);

void http_conn_close(struct http_conn *conn);

int http_conn_rbuf_require(struct http_conn *conn,int addc);
void *http_conn_wbuf_require(struct http_conn *conn,int addc);

int http_conn_describe(char *dst,int dsta,const struct http_conn *conn);

/* Transfer.
 * One half of an HTTP transaction, either request or response. In either direction.
 *********************************************************************/
 
#define HTTP_XFER_ROLE_CLIENT_REQUEST   1
#define HTTP_XFER_ROLE_CLIENT_RESPONSE  2
#define HTTP_XFER_ROLE_SERVER_REQUEST   3
#define HTTP_XFER_ROLE_SERVER_RESPONSE  4
 
void http_xfer_del(struct http_xfer *xfer);
int http_xfer_ref(struct http_xfer *xfer);
struct http_xfer *http_xfer_new(int role);

/* Returns the length consumed, never more than (srcc).
 * Call in a loop until we return zero.
 */
int http_xfer_receive(struct http_xfer *xfer,const void *src,int srcc);

int http_xfer_set_request_line(struct http_xfer *xfer,const char *src,int srcc);
int http_xfer_set_status_line(struct http_xfer *xfer,const char *src,int srcc);
int http_xfer_add_header_line(struct http_xfer *xfer,const char *src,int srcc);
int http_xfer_add_header(struct http_xfer *xfer,const char *k,int kc,const char *v,int vc);
int http_xfer_set_body(struct http_xfer *xfer,const void *src,int srcc);

int http_xfer_is_request(const struct http_xfer *xfer);
int http_xfer_is_response(const struct http_xfer *xfer);
int http_xfer_is_client(const struct http_xfer *xfer);
int http_xfer_is_server(const struct http_xfer *xfer);

int http_xfer_get_method(void *dstpp,const struct http_xfer *xfer);
int http_xfer_get_path(void *dstpp,const struct http_xfer *xfer);
int http_xfer_get_header(void *dstpp,const struct http_xfer *xfer,const char *k,int kc);
int http_xfer_get_body(void *dstpp,const struct http_xfer *xfer);

int http_xfer_respond(struct http_xfer *xfer,int status,const char *fmt,...); // status only

/* Text.
 *****************************************************************/
 
int http_method_from_request_line(void *dstpp,const char *src,int srcc);
int http_path_from_request_line(void *dstpp,const char *src,int srcc);
int http_full_path_from_request_line(void *dstpp,const char *src,int srcc);
int http_query_from_request_line(void *dstpp,const char *src,int srcc);
int http_status_from_status_line(void *dstpp,const char *src,int srcc);
int http_message_from_status_line(void *dstpp,const char *src,int srcc);

// Never fails to return a valid MIME type.
const char *http_guess_content_type(const char *path,const void *src,int srcc);

#endif
