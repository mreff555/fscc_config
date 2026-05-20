#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#define FCR_ASYNC_VALUE "03000000"
#define FSCC_CLASS_PATH "/sys/class/fscc"

static void print_usage(const char *prog) {
    fprintf(stdout,
        "Usage:\n"
        "  %s --list                  List all fscc devices and their FCR status\n"
        "  %s /dev/ttySx [baud]       Configure tty port + auto-detect async fscc device\n\n"
        "Examples:\n"
        "  sudo %s --list\n"
        "  sudo %s /dev/ttyS4 921600\n",
        prog, prog, prog, prog);
}

/* Read FCR value */
static int read_fcr(const char *fscc_name, char *out, size_t len) {
    char path[256];
    FILE *f;

    snprintf(path, sizeof(path), FSCC_CLASS_PATH "/%s/registers/fcr", fscc_name);
    f = fopen(path, "r");
    if (!f) return -1;

    if (!fgets(out, len, f)) {
        fclose(f);
        return -1;
    }
    fclose(f);
    out[strcspn(out, "\n")] = '\0';
    return 0;
}

/* Write to FCR */
static int write_fcr(const char *fscc_name, const char *value) {
    char path[256];
    FILE *f;

    snprintf(path, sizeof(path), FSCC_CLASS_PATH "/%s/registers/fcr", fscc_name);
    f = fopen(path, "w");
    if (!f) return -1;

    int ret = fprintf(f, "%s", value);
    fclose(f);
    return (ret > 0) ? 0 : -1;
}

/* List all fscc devices and their FCR status */
static void list_fscc_devices(void) {
    DIR *dir;
    struct dirent *entry;
    char value[32];

    printf("FSCC Devices:\n");
    printf("-------------\n");

    dir = opendir(FSCC_CLASS_PATH);
    if (!dir) {
        fprintf(stdout, "No fscc devices found (is the fscc driver loaded?)\n");
        return;
    }

    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "fscc", 4) != 0) continue;

        count++;
        if (read_fcr(entry->d_name, value, sizeof(value)) == 0) {
            const char *status = (strcmp(value, FCR_ASYNC_VALUE) == 0) 
                                 ? "ASYNC (16C950)" 
                                 : "SYNC / Other";
            printf("  %-8s  FCR: %-10s  Mode: %s\n", entry->d_name, value, status);
        } else {
            printf("  %-8s  FCR: (read error)\n", entry->d_name);
        }
    }
    closedir(dir);

    if (count == 0) {
        printf("No fscc devices found.\n");
    }
}

/* Auto-detect fscc device with async mode enabled */
static char* find_async_fscc_device(void) {
    DIR *dir;
    struct dirent *entry;
    static char name[32];
    char value[32];

    dir = opendir(FSCC_CLASS_PATH);
    if (!dir) return NULL;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "fscc", 4) != 0) continue;

        if (read_fcr(entry->d_name, value, sizeof(value)) == 0 &&
            strcmp(value, FCR_ASYNC_VALUE) == 0) {
            strncpy(name, entry->d_name, sizeof(name));
            closedir(dir);
            return name;
        }
    }
    closedir(dir);
    return NULL;
}

/* Enable + verify async mode */
static int enable_async_mode(const char *fscc_name) {
    char current[32];

    if (read_fcr(fscc_name, current, sizeof(current)) != 0) {
        fprintf(stdout, "Warning: Could not read FCR for %s\n", fscc_name);
        return 0;
    }

    printf("Using fscc device: %s (current FCR: %s)\n", fscc_name, current);

    if (strcmp(current, FCR_ASYNC_VALUE) == 0) {
        printf("Async mode already enabled.\n");
        return 1;
    }

    printf("Writing %s to enable async mode...\n", FCR_ASYNC_VALUE);
    if (write_fcr(fscc_name, FCR_ASYNC_VALUE) != 0) {
        fprintf(stdout, "Error: Failed to write FCR: %s\n", strerror(errno));
        return 0;
    }

    if (read_fcr(fscc_name, current, sizeof(current)) == 0 &&
        strcmp(current, FCR_ASYNC_VALUE) == 0) {
        printf("Success! Async mode enabled on %s\n", fscc_name);
        return 1;
    }

    fprintf(stdout, "Warning: FCR verification failed.\n");
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    /* === Handle --list === */
    if (strcmp(argv[1], "--list") == 0 || strcmp(argv[1], "-l") == 0) {
        if (geteuid() != 0) {
            fprintf(stdout, "Note: Some information may require root.\n");
        }
        list_fscc_devices();
        return 0;
    }

    if (geteuid() != 0) {
        fprintf(stdout, "Error: This program must be run as root.\n");
        return 1;
    }

    const char *device = argv[1];
    unsigned int desired_baud = (argc >= 3) ? strtoul(argv[2], NULL, 10) : 921600;

    /* === Configure tty port === */
    int fd = open(device, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        fprintf(stdout, "Error opening %s: %s\n", device, strerror(errno));
        return 1;
    }

    struct serial_struct serinfo;
    if (ioctl(fd, TIOCGSERIAL, &serinfo) == 0) {
        printf("Kernel reported baud_base: %d\n", serinfo.baud_base);
    }

    serinfo.type = PORT_16C950;
    serinfo.flags &= ~ASYNC_SPD_MASK;
    serinfo.flags |= ASYNC_SPD_CUST;
    serinfo.custom_divisor = serinfo.baud_base / desired_baud;

    if (ioctl(fd, TIOCSSERIAL, &serinfo) < 0) {
        fprintf(stdout, "Error setting PORT_16C950 + custom baud: %s\n", strerror(errno));
        close(fd);
        return 1;
    }

    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, B38400);
    cfsetospeed(&options, B38400);
    options.c_cflag = (options.c_cflag & ~CSIZE) | CS8 | CLOCAL | CREAD;
    options.c_cflag &= ~(PARENB | CSTOPB);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;
    tcsetattr(fd, TCSANOW, &options);

    printf("Configured %s @ %u baud (PORT_16C950 mode)\n", device, desired_baud);
    close(fd);

    /* === Auto FCR configuration === */
    printf("\n--- Detecting async FSCC device ---\n");
    char *fscc_name = find_async_fscc_device();

    if (fscc_name) {
        enable_async_mode(fscc_name);
    } else {
        fprintf(stdout, "No fscc device with async FCR (03000000) found.\n");
        fprintf(stdout, "Use '%s --list' to see available devices.\n", argv[0]);
    }

    return 0;
}