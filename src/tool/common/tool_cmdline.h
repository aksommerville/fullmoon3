/* tool_cmdline.h
 * Helpers for digesting argv.
 * Tools are encouraged to use this to promote uniformity.
 */
 
#ifndef TOOL_CMDLINE_H
#define TOOL_CMDLINE_H

struct tool_cmdline {
  const char *exename;
  char **srcpathv;
  int srcpathc,srcpatha;
  char *dstpath;
  struct tool_option {
    char *k,*v;
    int kc,vc,vn;
  } *optionv;
  int optionc,optiona;
// Caller should set this before tool_cmdline_argv:
  void (*print_help)(const char *exename);
};

void tool_cmdline_cleanup(struct tool_cmdline *cmdline);

int tool_cmdline_argv(struct tool_cmdline *cmdline,int argc,char **argv);

int tool_cmdline_one_input(struct tool_cmdline *cmdline);
int tool_cmdline_one_or_more_input(struct tool_cmdline *cmdline);
// "zero_or_more_input" is a valid use case, but doesn't require any assertion.
int tool_cmdline_one_output(struct tool_cmdline *cmdline);

/* Getting as int returns zero if missing or malformed.
 * As boolean, missing is zero, empty is 1, malformed is -1.
 * Option values that exist are always NUL-terminated.
 */
struct tool_option *tool_cmdline_get_option_struct(const struct tool_cmdline *cmdline,const char *k,int kc);
int tool_cmdline_get_option(void *dstpp,const struct tool_cmdline *cmdline,const char *k,int kc);
int tool_cmdline_get_option_int(const struct tool_cmdline *cmdline,const char *k,int kc);
int tool_cmdline_get_option_boolean(const struct tool_cmdline *cmdline,const char *k,int kc);

#endif
