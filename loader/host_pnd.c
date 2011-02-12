// vim:shiftwidth=2:expandtab

struct in_default_bind in_evdev_defbinds[] = {
  { KEY_UP,         IN_BINDTYPE_PLAYER12, GP2X_UP },
  { KEY_PAGEUP,     IN_BINDTYPE_PLAYER12, GP2X_Y },
  { KEY_END,        IN_BINDTYPE_PLAYER12, GP2X_B },
  { KEY_PAGEDOWN,   IN_BINDTYPE_PLAYER12, GP2X_X },
  { KEY_HOME,       IN_BINDTYPE_PLAYER12, GP2X_A },
  { KEY_RIGHTSHIFT, IN_BINDTYPE_PLAYER12, GP2X_L },
  { KEY_RIGHTCTRL,  IN_BINDTYPE_PLAYER12, GP2X_R },
  { KEY_LEFTALT,    IN_BINDTYPE_PLAYER12, GP2X_START },
  { KEY_LEFTCTRL,   IN_BINDTYPE_PLAYER12, GP2X_SELECT },
  { KEY_COMMA,      IN_BINDTYPE_PLAYER12, GP2X_VOL_DOWN },
  { KEY_DOT,        IN_BINDTYPE_PLAYER12, GP2X_VOL_UP },
  { KEY_1,          IN_BINDTYPE_PLAYER12, GP2X_PUSH },
  { KEY_Q,          IN_BINDTYPE_EMU, 0 },
  { 0, 0, 0 },
};

static void host_actions(int actions[IN_BINDTYPE_COUNT])
{
  if (actions[IN_BINDTYPE_EMU] & 1)
    host_forced_exit();
}

// todo: rm when generic code works
#if 0
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/input.h>

#include "header.h"
#include "realfuncs.h"

static int ifds[2] = { -1, -1 };
static int keystate;

int host_init(void)
{
  char buff[64];
  int i, ifd, ret;

  for (ifd = -1, i = 0; ifds[0] == -1 || ifds[1] == -1; i++) {
    snprintf(buff, sizeof(buff), "/dev/input/event%i", i);
    ifd = open(buff, O_RDONLY | O_NONBLOCK);
    if (ifd == -1)
      break;

    ret = ioctl(ifd, EVIOCGNAME(sizeof(buff)), buff);
    if (ret < 0)
      break;

    if (strcasestr(buff, "gpio") != NULL) {
      ifds[0] = ifd;
      continue;
    }
    if (strcasestr(buff, "keypad") != NULL) {
      ifds[1] = ifd;
      continue;
    }
    close(ifd);
  }

  if (ifds[0] < 0)
    fprintf(stderr, PFX "missing buttons\n");
  if (ifds[1] < 0)
    fprintf(stderr, PFX "missing keypad\n");

  return 0;
}

static const struct {
  unsigned short key;
  unsigned short btn;
} key_map[] = {
  { KEY_LEFT,       GP2X_LEFT },
  { KEY_RIGHT,      GP2X_RIGHT },
  { KEY_UP,         GP2X_UP },
  { KEY_DOWN,       GP2X_DOWN },
  { KEY_PAGEUP,     GP2X_Y },
  { BTN_BASE,       GP2X_Y },
  { KEY_END,        GP2X_B },
  { BTN_BASE2,      GP2X_B },
  { KEY_PAGEDOWN,   GP2X_X },
  { BTN_BASE3,      GP2X_X },
  { KEY_HOME,       GP2X_A },
  { BTN_BASE4,      GP2X_A },
  { KEY_RIGHTSHIFT, GP2X_L },
  { BTN_TL,         GP2X_L },
  { KEY_RIGHTCTRL,  GP2X_R },
  { BTN_TR,         GP2X_R },
  { KEY_LEFTALT,    GP2X_START },
  { BTN_START,      GP2X_START },
  { KEY_LEFTCTRL,   GP2X_SELECT },
  { BTN_SELECT,     GP2X_SELECT },
  { KEY_COMMA,      GP2X_VOL_DOWN },
  { KEY_DOT,        GP2X_VOL_UP },
  { KEY_1,          GP2X_PUSH },
};

int host_read_btns(void)
{
  struct input_event ev;
  int i, ret;

  while (1)
  {
    ret = read(ifds[0], &ev, sizeof(ev));
    if (ret < (int) sizeof(ev)) {
      if (errno != EAGAIN && errno != EWOULDBLOCK)
        perror(PFX "read error");

      ret = read(ifds[1], &ev, sizeof(ev));
      if (ret < (int) sizeof(ev))
        if (errno != EAGAIN && errno != EWOULDBLOCK)
          perror(PFX "read error");
    }

    if (ret < (int) sizeof(ev))
      return keystate;

    if (ev.type != EV_KEY)
      continue;

    if (ev.code == KEY_Q && ev.value) {
      // exit() might not be enough because loader and app data is out of sync,
      // and other threads (which are really processes) might not exit properly.
      system("killall ginge_sloader");
      usleep(300000);
      system("killall -9 ginge_sloader");
      exit(1);
    }

    for (i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++) {
      if (key_map[i].key != ev.code)
        continue;
      if (ev.value)
        keystate |=  (1 << key_map[i].btn);
      else
        keystate &= ~(1 << key_map[i].btn);
      break;
    }
  }

  return keystate;
}
#endif
