/*
 * liuqin_touchctl: minimal ioctl client for /dev/xiaomi-touch.
 *
 * The liuqin Novatek (nt36532) touch driver keeps the NVTCapacitivePen input
 * device disabled until pen_bluetooth_connect is set. That bit is updated by
 * the mode-20 feature path as:
 *
 *   pen_bluetooth_connect = (trigger flag == 0) && (pen counter != 0)
 *
 * where value 1 clears the trigger flag and value 2 makes the counter
 * non-zero (it starts at 0 on every boot). MIUI frameworks drive this from
 * their bluetooth stack when the stylus connects; AOSP-based ROMs have no
 * equivalent caller, so the pen input device stays disabled after every boot
 * (kernel log: "DISABLE pen input device").
 *
 * "pen-enable" issues the verified sequence below; init.liuqin.pen.rc calls
 * it once per boot, after which the driver logs "ENABLE pen input device".
 * With no stylus present the enabled input device simply reports no events,
 * so the state is safe to force.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define TOUCH_MAGIC 0x54
#define TOUCH_IOC_SET_CUR_VALUE _IO(TOUCH_MAGIC, 0)
#define TOUCH_IOC_GET_CUR_VALUE _IO(TOUCH_MAGIC, 1)

#define PEN_BT_TRIGGER_CLEAR 1 /* mode 20, value 1: clear trigger flag */
#define PEN_COUNT_BUMP 2       /* mode 20, value 2: counter becomes non-zero */

static int touch_set(int mode, int value)
{
    int fd, req[3] = { 0, mode, value };

    fd = open("/dev/xiaomi-touch", O_RDWR);
    if (fd < 0) {
        perror("open /dev/xiaomi-touch");
        return 1;
    }
    if (ioctl(fd, TOUCH_IOC_SET_CUR_VALUE, req) < 0) {
        perror("ioctl SET_CUR_VALUE");
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2 && !strcmp(argv[1], "pen-enable")) {
        /* Order matters: clear the trigger flag first, then bump the
         * counter so pen_bluetooth_connect evaluates to 1. */
        if (touch_set(20, PEN_BT_TRIGGER_CLEAR))
            return 1;
        return touch_set(20, PEN_COUNT_BUMP);
    }

    if (argc < 4 || strcmp(argv[1], "set")) {
        fprintf(stderr, "usage: liuqin_touchctl set <mode> <value> | pen-enable\n");
        return 2;
    }
    return touch_set(atoi(argv[2]), atoi(argv[3]));
}
