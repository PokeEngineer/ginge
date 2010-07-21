#include "../common/host_fb.h"

#define PFX "ginge: "
#define err(f, ...) fprintf(stderr, PFX f, ##__VA_ARGS__)
#define log(f, ...) fprintf(stdout, PFX f, ##__VA_ARGS__)
#if 1
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
};
extern struct dev_fd_t emu_interesting_fds[];
enum {
	IFD_SOUND = 0,
};

enum {
  FAKEDEV_MEM = 10001,
  FAKEDEV_GPIO,
  FAKEDEV_FB0,
  FAKEDEV_FB1,
  FAKEDEV_MMUHACK,
  FAKEDEV_FD_BOUNDARY,
};

void do_patches(void *ptr, unsigned int size);

void  emu_init(void *map_bottom);
void *emu_do_mmap(unsigned int length, int prot, int flags, int fd, unsigned int offset);
int   emu_do_ioctl(int fd, int request, void *argp);
int   emu_read_gpiodev(void *buf, int count);

int   host_read_btns(void);

enum  { GP2X_UP = 0,      GP2X_LEFT = 2,      GP2X_DOWN = 4,  GP2X_RIGHT = 6,
        GP2X_START = 8,   GP2X_SELECT = 9,    GP2X_L = 10,    GP2X_R = 11,
        GP2X_A = 12,      GP2X_B = 13,        GP2X_X = 14,    GP2X_Y = 15,
        GP2X_VOL_UP = 16, GP2X_VOL_DOWN = 17, GP2X_PUSH = 18 };

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

