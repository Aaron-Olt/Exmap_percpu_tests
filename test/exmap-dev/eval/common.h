#pragma once

#define die(fmt) do { perror(fmt); exit(EXIT_FAILURE); } while(0)
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(*arr))

const unsigned pageSize = 4096;

int open_file(const char *fn, size_t *file_size);
