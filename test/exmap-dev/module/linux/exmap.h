#pragma once
#include <asm/ioctl.h>
#include <linux/types.h>

#define STATIC_ASSERT(COND,MSG) typedef char static_assertion_##MSG[(!!(COND))*2-1]

// Maximum Range of exmap_page.len
#define EXMAP_PAGE_LEN_BITS 12
#define EXMAP_PAGE_MAX_PAGES (1 << EXMAP_PAGE_LEN_BITS)

#define EXMAP_USER_INTERFACE_PAGES 512

#define getsoftinterupt _IOR('a', 'a', int32_t)
#define gethardinterupt _IOR('b', 'a', int32_t)

struct exmap_iov {
	union {
		uint64_t value;
		struct {
			uint64_t page   : 64 - EXMAP_PAGE_LEN_BITS;
			uint64_t len    : EXMAP_PAGE_LEN_BITS;
		};
		struct {
			int32_t   res;
			int16_t   pages;
		};
	};
};
STATIC_ASSERT(sizeof(struct exmap_iov) == 8, exmap_iov);

struct exmap_user_interface {
	union {
		struct exmap_iov iov[EXMAP_USER_INTERFACE_PAGES];
	};
};
STATIC_ASSERT(sizeof(struct exmap_user_interface) == 4096, exmap_user_interface);

struct exmap_ioctl_setup {
	int    fd;
	int    max_interfaces;
	size_t buffer_size;
	uint64_t flags;
};

#define EXMAP_IOCTL_SETUP _IOC(_IOC_WRITE, 'k', 1, sizeof(struct exmap_ioctl_setup))

struct exmap_action_params {
	uint16_t interface;
	uint16_t iov_len;
	uint16_t opcode; // exmap_opcode
	uint64_t flags;  // exmap_flags
};

#define EXMAP_IOCTL_ACTION _IOC(_IOC_WRITE, 'k', 2, sizeof(struct exmap_action_params))



enum exmap_opcode {
	EXMAP_OP_READ   = 0,
	EXMAP_OP_ALLOC  = 1,
	EXMAP_OP_FREE   = 2,
	EXMAP_OP_WRITE  = 3,
};

enum exmap_flags {
	// When allocating memory, we only look at the first element, and
	// if that is currently mapped, we skip that exmap_iov
	EXMAP_ALLOC_PROBE  = 1, // Not implemented yet(!); If the first page of a vector is mapped, return immediately
};
typedef enum exmap_flags exmap_flags;



#define EXMAP_OFF_EXMAP       0x0000
#define EXMAP_OFF_INTERFACE_BASE 0xe000000000000000UL
#define EXMAP_OFF_INTERFACE_MAX  0xf000000000000000UL
#define EXMAP_OFF_INTERFACE(n) (EXMAP_OFF_INTERFACE_BASE | (n << 12LL))
