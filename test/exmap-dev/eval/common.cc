#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <string>
#include <set>
#include <boost/algorithm/string.hpp>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <sys/stat.h>



#include "common.h"

int open_file(const char *fn, size_t *file_size) {
    assert(fn != NULL);
    int fd= open(fn, O_RDONLY | O_DIRECT);
    if (fd == -1) return -1;

    struct stat s;
    int ret = fstat(fd, &s);
    if (ret == -1) return -1;
    uint64_t fileSize = static_cast<uint64_t>(s.st_size);
    if (fileSize == 0) { // /dev/sda
        ioctl(fd, BLKGETSIZE64, &fileSize);
    }
    *file_size = fileSize;

    return fd;
}

unsigned long long
readTLBShootdownCount(void) {
    std::ifstream irq_stats("/proc/interrupts");
    assert (!!irq_stats);

    for (std::string line; std::getline(irq_stats, line); ) {
        if (line.find("TLB") != std::string::npos) {
            std::vector<std::string> strs;
            boost::split(strs, line, boost::is_any_of("\t "));
            unsigned long long count = 0;
            for (size_t i = 0; i < strs.size(); i++) {
                std::set<std::string> bad_strs = {"", "TLB", "TLB:", "shootdowns"};
                if (bad_strs.find(strs[i]) != bad_strs.end())
                    continue;
                std::stringstream ss(strs[i]);
                unsigned long long c;
                ss >> c;
                count += c;
            }
            return count;
        }
    }
    return 0;
}
