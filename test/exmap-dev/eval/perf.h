#pragma once
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

typedef uint64_t perf_event_id; // For readability only

// Structure to hold a perf group
struct perf_handle {
    int group_fd;   // First perf_event fd that we create
    int nevents;    // Number of registered events
    size_t rf_size; // How large is the read_format buffer (derived from nevents)
    struct read_format *rf; // heap-allocated buffer for the read event
};

#ifdef __cplusplus
extern "C" {
#endif

perf_event_id perf_event_add(struct perf_handle *p, int type, int config);
void perf_event_start(struct perf_handle *p);
void perf_event_stop(struct perf_handle *p);
uint64_t perf_event_get(struct perf_handle *p, perf_event_id id);
void perf_event_reset(struct perf_handle *p);

#ifdef __cplusplus
}
#endif
