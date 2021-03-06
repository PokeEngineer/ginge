#ifndef INCLUDE_sQt5fY5eUJn5tKV0IBTDxK0zqQutTqTp
#define INCLUDE_sQt5fY5eUJn5tKV0IBTDxK0zqQutTqTp 1

#include "llibc.h"

#define PFX "ginge: "
#define err(f, ...) g_fprintf(2, PFX f, ##__VA_ARGS__)
#define log(f, ...) g_fprintf(1, PFX f, ##__VA_ARGS__)
#ifdef DBG
#define dbg log
#define dbg_c printf
#else
#define dbg(...)
#define dbg_c(...)
#endif

void do_entry(unsigned long entry, void *stack_frame, int stack_frame_cnt, void *exitf);

struct dev_fd_t {
  const char *name;
  int fd;
  void (*open_cb)(int fd);
};
extern struct dev_fd_t emu_interesting_fds[];
enum {
	IFD_SOUND = 0,
};

// note: __FD_SETSIZE is 1024, fd_set stuff will crash if made larger
enum {
  FAKEDEV_MEM = 1001,
  FAKEDEV_GPIO,
  FAKEDEV_FB0,
  FAKEDEV_FB1,
  FAKEDEV_MMUHACK,  // 1005
  FAKEDEV_WM97XX,
  FAKEDEV_WM97XX_P,
  FAKEDEV_TTY,      // 1008
  FAKEDEV_TTY0,
  FAKEDEV_FD_BOUNDARY,
};

void do_patches(void *ptr, unsigned int size);

struct op_context;

void  emu_init(void *map_bottom[2], int is_dl);
void  emu_call_handle_op(struct op_context *op_ctx);
long  emu_do_mmap(unsigned int length, int prot, int flags, int fd, unsigned int offset);
long  emu_do_munmap(void *addr, unsigned int length);
long  emu_do_ioctl(int fd, int request, void *argp);
long  emu_do_read(int fd, void *buf, int count);
void *emu_do_fopen(const char *path, const char *mode);
int   emu_do_system(const char *command);
long  emu_do_execve(const char *filename, char *const argv[], char *const envp[]);
const char *emu_wrap_path(const char *path);
void  emu_wrap_path_free(const char *w_path, const char *old_path);

int   host_init(void);
int   host_read_btns(void);
int   host_read_ts(int *pressed, int *x1024, int *y1024);
void  host_forced_exit(int status);

enum  { GP2X_UP = 0,      GP2X_LEFT = 2,      GP2X_DOWN = 4,  GP2X_RIGHT = 6,
        GP2X_START = 8,   GP2X_SELECT = 9,    GP2X_L = 10,    GP2X_R = 11,
        GP2X_A = 12,      GP2X_B = 13,        GP2X_X = 14,    GP2X_Y = 15,
        GP2X_VOL_UP = 16, GP2X_VOL_DOWN = 17, GP2X_PUSH = 18 };

extern char *bin_path;
extern char **g_argv;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#endif /* INCLUDE_sQt5fY5eUJn5tKV0IBTDxK0zqQutTqTp */
