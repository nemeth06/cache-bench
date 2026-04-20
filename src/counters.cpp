#include "counters.hpp"


#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>

// glibc does not expose perf_event_open directly -> use the raw syscall.
static int perf_event_open(struct perf_event_attr* attr,
                            pid_t pid, int cpu, int group_fd,
                            unsigned long flags)
{
    return static_cast<int>(
        syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags));
}

// Open a single hardware or hw-cache counter.  Returns -1 on failure.
static int open_counter(std::uint32_t type, std::uint64_t config)
{
    struct perf_event_attr attr{};
    attr.type           = type;
    attr.size           = sizeof(attr);
    attr.config         = config;
    attr.disabled       = 1;   // disabled by default, we enable manually
    attr.exclude_kernel = 1;   // count only user-space events
    attr.exclude_hv     = 1;
    // pid=0: this process, cpu=-1: any CPU, group_fd=-1: no group
    return perf_event_open(&attr, 0, -1, -1, 0);
}

PerfCounters::PerfCounters()
{
    // L1 cache misses
    fd_l1_misses_ = open_counter(PERF_TYPE_HARDWARE,
                                  PERF_COUNT_HW_CACHE_MISSES);

    // Attempt generic LLC misses first (works natively on Intel)
    fd_llc_misses_ = open_counter(
        PERF_TYPE_HW_CACHE,
        static_cast<std::uint64_t>(PERF_COUNT_HW_CACHE_LL)
            | (static_cast<std::uint64_t>(PERF_COUNT_HW_CACHE_OP_READ)    << 8)
            | (static_cast<std::uint64_t>(PERF_COUNT_HW_CACHE_RESULT_MISS) << 16));

    // Fallback for AMD Zen+ to measure DRAM Accesses
    if (fd_llc_misses_ == -1) {
        // Event 0x43: Demand Data Cache Fills by Data Source
        // Umask 0x48: 0x08 (Local DRAM) | 0x40 (Remote DRAM)
        constexpr std::uint64_t AMD_DRAM_FILLS_RAW = 0x4843;
        fd_llc_misses_ = open_counter(PERF_TYPE_RAW, AMD_DRAM_FILLS_RAW);
    }

    fd_branch_mispr_ = open_counter(PERF_TYPE_HARDWARE,
                                     PERF_COUNT_HW_BRANCH_MISSES);
    fd_cycles_       = open_counter(PERF_TYPE_HARDWARE,
                                     PERF_COUNT_HW_CPU_CYCLES);
    fd_instructions_ = open_counter(PERF_TYPE_HARDWARE,
                                     PERF_COUNT_HW_INSTRUCTIONS);

    available_ = (fd_l1_misses_    != -1 && fd_llc_misses_   != -1 &&
                  fd_branch_mispr_ != -1 && fd_cycles_        != -1 &&
                  fd_instructions_ != -1);

    if (!available_) {
        // Close any descriptors that did open before we hit a failure.
        const std::array<int, 5> fds{
            fd_l1_misses_, fd_llc_misses_, fd_branch_mispr_,
            fd_cycles_,    fd_instructions_
        };
        for (int fd : fds) {
            if (fd != -1) ::close(fd);
        }
        fd_l1_misses_ = fd_llc_misses_ = fd_branch_mispr_ =
        fd_cycles_    = fd_instructions_ = -1;
    }
}

PerfCounters::~PerfCounters()
{
    if (!available_) return;
    for (int fd : {fd_l1_misses_, fd_llc_misses_, fd_branch_mispr_,
                   fd_cycles_,    fd_instructions_})
    {
        ::close(fd);
    }
}

void PerfCounters::start()
{
    if (!available_) return;
    for (int fd : {fd_l1_misses_, fd_llc_misses_, fd_branch_mispr_,
                   fd_cycles_,    fd_instructions_})
    {
        ::ioctl(fd, PERF_EVENT_IOC_RESET,  0);
        ::ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }
}

void PerfCounters::stop()
{
    if (!available_) return;
    for (int fd : {fd_l1_misses_, fd_llc_misses_, fd_branch_mispr_,
                   fd_cycles_,    fd_instructions_})
    {
        ::ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    }
}

Counts PerfCounters::read() const
{
    Counts c{};
    if (!available_) return c;

    auto rd = [](int fd) -> long long {
        long long val = 0;
        return (::read(fd, &val, sizeof(val)) == sizeof(val)) ? val : -1LL;
    };

    c.l1_misses    = rd(fd_l1_misses_);
    c.llc_misses   = rd(fd_llc_misses_);
    c.branch_mispr = rd(fd_branch_mispr_);
    c.cycles       = rd(fd_cycles_);
    c.instructions = rd(fd_instructions_);
    return c;
}