
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

//#include "bench_common.h"
//#include "linux/exmap.h"

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

// sched affinity
#include <sched.h>
#include <pthread.h>

#define page_length 12
void something (){
sleep(1);
std::cout <<  "running on cpu " << sched_getcpu() <<" \n" ;
}

int main(){
	// amount of CPUs

	int numCPU;
	//hardware_concurrency might not work and return 0 for amount of max concurrent threads
	if (!(numCPU = std::thread::hardware_concurrency())){
	numCPU = sysconf(_SC_NPROCESSORS_ONLN);
	}
	

	std::thread t1;
	//std::thread t2;

	
	//declare array of bitmasks 
	/*
	cpu_set_t *my_set= new cpu_set_t [numCPU];
	for (int i ; i<2;i++){
	//initialize with zero
	CPU_ZERO (&my_set[i]);
	//each set i, runs only on cpu i 
	CPU_SET(i,&my_set[i]);
	
	if (pthread_setaffinity_np(threads[i].native_handle(),sizeof(cpu_set_t), &my_set[i])){
	
	}
	std::cerr << "ERROR schedmask"<<"\n";
	}
	*/
	std::cout <<numCPU<<"numcpu \n";
	
	cpu_set_t my_set1;
	
	CPU_ZERO (&my_set1);
	
	CPU_SET(0,&my_set1);
	
	printf("CPU_COUNT() of set:    %d\n", CPU_COUNT( &my_set1));
	for (int i=0 ;i<numCPU;i++){
	if (CPU_ISSET(i,&my_set1)) {
	std::cout <<"CPU "<<i<<" is in cpuset \n";
	}
	}
	
	/*if (pthread_setaffinity_np(t1.native_handle(), sizeof(cpu_set_t), &my_set1)){
	std::cerr << "ERROR schedmask"<<"\n";
	}
	*/
	//std::cout<< my_set1 <<" \n";
	
	//int thread_count = 2;
	//for (int thread_id = 0; thread_id < thread_count; thread_id++){
	//threads[thread_id] =std::thread{something};
	//sleep(1);
	//}
	t1 =std::thread(something);
	if (pthread_setaffinity_np(t1.native_handle(), sizeof(cpu_set_t), &my_set1)){
	std::cerr << "ERROR schedmask"<<"\n";
	}
	
	
		/*threads.emplace_back([&, thread_id]() { 
			std::cout <<" I am "<< thread_id << "running on cpu " << sched_getcpu() <<" " ;
			});
		
		sleep(1);
		for (auto& t : threads)
		t.join();
		
		for (int i ; i<5;i++){
		std::cout <<" I am "<< i << "running on cpu " << sched_getcpu() <<" " ;
		}
		*/
		//for (auto& t : threads)
		//t.join();
	t1.join();

return 0;
}
