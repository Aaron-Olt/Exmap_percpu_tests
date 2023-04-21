#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <errno.h>
#include <iostream>
#include <vector>
#include <thread>
#include <set>
#include <atomic>
#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <cstdio>
#include "bench_common.h"

#include "perf.h"
#include "common.h"
int batch_size = EXMAP_USER_INTERFACE_PAGES;

uint16_t prepare_vector(struct exmap_user_interface *interface, unsigned offset, unsigned count) {

	for (unsigned i = 0; i < count;  i++) {
		interface->iov[i].page = offset + i;
		interface->iov[i].len = 1;
	}
	return count;
}

int touch_vector(char *exmap, unsigned offset, unsigned count) {

	for (unsigned i = 0; i < count;  i++) {
		exmap[(offset + i) * PAGE_SIZE] ++;
	}
	return count;
}




int main(int argc, char *argv[]) {

    // Create and initialize a new perf handle
    struct perf_handle perf;
    memset(&perf, 0, sizeof(perf));

	
    // Create three new perf events that we want to monitor for our
    // matrix multiplication algorithms
    perf_event_id id_instrs =
        perf_event_add(&perf, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS);
    perf_event_id id_cycles =
        perf_event_add(&perf, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES);
    perf_event_id id_cache_miss =
        perf_event_add(&perf, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES);
    perf_event_id id_cache_refs =
       perf_event_add(&perf, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES);
   perf_event_id id_cpu_migrations =
       perf_event_add(&perf, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS);
    
	
	long long int acc_shotdowns=0;
	long long int acc_lastReadCnt=0;
	std::atomic<uint64_t> readCnt(0);

	int thread_count = atoi(getenv("THREADS") ?: "1");
	thread_count = atoi(argv[1]);
	std::cout << "thread_count "<< thread_count << " \n";
	//thread_count=4;
	std::atomic_bool stop_workers(false);
    char *BATCH_SIZE = getenv("BATCH_SIZE");
    if (BATCH_SIZE) {
        batch_size = atoi(BATCH_SIZE);
        printf("BATCH_SIZE: %d\n", batch_size);
        if ((batch_size < 0) || (batch_size > EXMAP_USER_INTERFACE_PAGES))
            batch_size = EXMAP_USER_INTERFACE_PAGES;
    }

    int fd = open("/dev/exmap", O_RDWR);
    if (fd < 0) die("open");

	const size_t MAP_SIZE = thread_count * 8 * 1024 * 1024;
	char *map = (char*) mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) die("mmap");

    printf("# BATCH_SIZE: %d\n", batch_size);
	printf("# MAP: %p-%p\n", map, map + MAP_SIZE);

	// must fail!
	void *tmp = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	assert(tmp == MAP_FAILED && errno == EBUSY);

	struct exmap_ioctl_setup buffer;
	buffer.fd             = -1; // Not baked by a file
	buffer.max_interfaces = thread_count;
	buffer.buffer_size    = thread_count * 2 * 512;
	if (ioctl(fd, EXMAP_IOCTL_SETUP, &buffer) < 0) {
		perror("ioctl: exmap_setup");
		return 0;
		/* handle error */
	}

	std::vector<std::thread> threads;
	std::cout<< "# time(s), reads or allocs(*1e6/s), shootdowns, shootdowns/IOP," << ", instrs(*1e6/s) " << ", cycles(*1e6/s) "<< ", llc_refs(*1e6/s) "<<", llc_misses "<<", cpu_migrations"<<"\n";
	////////////////////////////////////////////////////////////////
	// Alloc/Free Thread
	for (uint16_t thread_id=0; thread_id < thread_count; thread_id++) {
		threads.emplace_back([&, thread_id]() {
			// Allocate an interface
			auto interface = (struct exmap_user_interface *)
				mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE,
					 MAP_SHARED, fd, EXMAP_OFF_INTERFACE(thread_id));
			if (interface == MAP_FAILED) die("mmap");

			unsigned base_offset = thread_id * 2 * EXMAP_USER_INTERFACE_PAGES;
			while(!stop_workers) {
				unsigned offset = 0;
				while (offset < EXMAP_USER_INTERFACE_PAGES) {
					unsigned start = base_offset + offset;
					unsigned count = std::min((unsigned)batch_size, EXMAP_USER_INTERFACE_PAGES - offset);

					if (count == 1) {
						if (pread(fd, &map[start * PAGE_SIZE], PAGE_SIZE, thread_id | (EXMAP_OP_ALLOC << 8)) != PAGE_SIZE) {
							perror("pread: exmap_alloc");
						}
					} else {
						uint16_t nr_pages = prepare_vector(interface, start, count);
						struct exmap_action_params params_alloc = {
							.interface = thread_id,
							.iov_len   = nr_pages,
							.opcode    = EXMAP_OP_ALLOC,
						};
						if (ioctl(fd, EXMAP_IOCTL_ACTION, &params_alloc) < 0) {
							perror("ioctl: exmap_action");
						}
					}

					touch_vector(map, start, count);

					readCnt += count;
					offset  += count;
				}
				assert(offset == EXMAP_USER_INTERFACE_PAGES);
				uint16_t nr_pages = prepare_vector(interface, base_offset, EXMAP_USER_INTERFACE_PAGES);
				struct exmap_action_params params_free = {
					.interface = thread_id,
					.iov_len   = nr_pages,
					.opcode    = EXMAP_OP_FREE,
				};
				if (ioctl(fd, EXMAP_IOCTL_ACTION, &params_free) < 0) {
					perror("ioctl: exmap_action");
				}
			}
		});
	}

	auto last_shootdowns = readTLBShootdownCount();
	int secs = 0;
	//output_legend();
	while (secs < 300) {
	        perf_event_start(&perf);
			struct timespec req = {.tv_sec =1, .tv_nsec = 0 * 1000 * 1000};
			nanosleep(&req, NULL);
			//sleep(1);
        	perf_event_stop(&perf);
		secs++;
		//sleep(1);
	        double instrs = perf_event_get(&perf, id_instrs);
        	double cycles = perf_event_get(&perf, id_cycles);
        	double llc_refs = perf_event_get(&perf, id_cache_refs);
        	long int llc_misses = perf_event_get(&perf, id_cache_miss);
		double cpu_migs = perf_event_get(&perf, id_cpu_migrations);
		
		auto shootdowns = readTLBShootdownCount();
		auto diff = (shootdowns-last_shootdowns);
		auto lastReadCnt = (unsigned)readCnt.exchange(0);
		//output_line(secs++, lastReadCnt, diff);
		std::cout<< secs<< ", "<<lastReadCnt/1e6<<", "<<diff<<", "<<(diff / (double) lastReadCnt)<<", "<<instrs/1e6<< ", "<<cycles/1e6<< ", "<<llc_refs/1e6<<", "<<llc_misses <<", "<< cpu_migs<<"\n"<< std::flush;
		last_shootdowns = shootdowns;
		acc_lastReadCnt+=lastReadCnt;
		acc_shotdowns+=diff;
		
		
	}
	stop_workers = true;
	for (auto& t : threads)
		t.join();
	close(fd);
	return 0;
}
