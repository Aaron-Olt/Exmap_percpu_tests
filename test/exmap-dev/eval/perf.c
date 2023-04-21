#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <emmintrin.h>
#include <limits.h>


#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*(arr)))
#define die(msg) do { perror(msg); exit(EXIT_FAILURE); } while(0)

// Matrix Multiplication Functions
#include "perf.h"

// When reading from the perf descriptor, the kernel returns an
// event record in the following format (if PERF_FORMAT_GROUP |
// PERF_FORMAT_ID are enabled).
// Example (with id0=100, id1=200): {.nr = 2, .values = {{41433, 200}, {42342314, 100}}}
struct read_format {
    uint64_t nr;
    struct {
        uint64_t value;
        perf_event_id id; // PERF_FORMAT_ID
    } values[];
};


// Syscall wrapper for perf_event_open(2), as glibc does not have one
static int sys_perf_event_open(struct perf_event_attr *attr,
                    pid_t pid, int cpu, int group_fd,
                    unsigned long flags) {
    return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

// Add a perf event of given (type, config) with default config. If p
// is not yet initialized (p->group_fd <=0), the perf_event becomes
// the group leader. The function returns an id that can be used in
// combination with perf_event_get.
perf_event_id perf_event_add(struct perf_handle *p, int type, int config) {
    // SOL_IF
    struct perf_event_attr attr;

    memset(&attr, 0, sizeof(struct perf_event_attr));
    attr.type = type;
    attr.size = sizeof(struct perf_event_attr);
    attr.config = config;
    attr.inherit = 1;
    attr.inherit_stat = 1;
    attr.disabled = 1;
    attr.exclude_kernel = 0;
    attr.exclude_hv = 1;
    attr.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
    int fd = sys_perf_event_open(&attr, 0, -1,
                             p->group_fd > 0 ? p->group_fd : -1,
                             0);
    if (fd < 0) die("perf_event_open");
    if (p->group_fd <= 0)
        p->group_fd = fd;

    p->nevents ++;

    perf_event_id id;
    if (ioctl(fd, PERF_EVENT_IOC_ID, &id) < 0)
        die("perf/IOC_ID");
    return id;
    // SOL_ELSE
    // FIXME: Create event with perf_event_open
    // FIXME: Get perf_event_id with PERF_EVENT_IOC_ID
    return -1;
    // SOL_END
}

// Resets and starts the perf measurement
void perf_event_start(struct perf_handle *p) {
    // SOL_IF
    // Reset and enable the event group
    ioctl(p->group_fd, PERF_EVENT_IOC_RESET,  PERF_IOC_FLAG_GROUP);
    ioctl(p->group_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    // SOL_ELSE
    // FIXME: PERF_EVENT_IOC_{RESET, ENABLE}
    // SOL_END
}

// Resets and starts the perf measurement
void perf_event_reset(struct perf_handle *p) {
    ioctl(p->group_fd, PERF_EVENT_IOC_RESET,  PERF_IOC_FLAG_GROUP);
}

// Stops the perf measurement and reads out the event
void perf_event_stop(struct perf_handle *p) {
    // SOL_IF
    // Stop the tracing for the whole event group
    ioctl(p->group_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);

    // Allocate a read_format buffer if not done yet.
    if (p->rf == NULL) {
        p->rf_size = sizeof(uint64_t) + 2 * p->nevents * sizeof(uint64_t);
        p->rf = malloc(p->rf_size);
    }

    // get the event from the kernel. Our buffer should be sized exactly righ
    if (read(p->group_fd, p->rf, p->rf_size) < 0)
        die("read");
    // SOL_ELSE
    // FIXME: PERF_EVENT_IOC_DISABLE
    // FIXME: Read event from the group_fd into an allocated buffer
    // SOL_END
}


// After the measurement, this helper extracts the event counter for
// the given perf_event_id (which was returned by perf_event_add)
uint64_t perf_event_get(struct perf_handle *p, perf_event_id id) {
    // SOL_IF
    for (unsigned i = 0; i < p->rf->nr; i++) {
        if (p->rf->values[i].id == id) {
            return p->rf->values[i].value;
        }
    }
    // SOL_END
    return -1;
}
