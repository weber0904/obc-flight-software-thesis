#include <csp/csp_debug.h>
#include <csp/csp_hooks.h>

#include <stdint.h>
#include <sys/sysctl.h>

uint32_t csp_memfree_hook(void) {
    uint64_t memoryBytes = 0U;
    size_t length = sizeof(memoryBytes);
    int mib[2] = {CTL_HW, HW_MEMSIZE};

    if (sysctl(mib, 2, &memoryBytes, &length, NULL, 0) != 0) {
        return 0U;
    }

    if (memoryBytes > UINT32_MAX) {
        return UINT32_MAX;
    }
    return (uint32_t)memoryBytes;
}

unsigned int csp_ps_hook(csp_packet_t* packet) {
    (void)packet;
    return 0U;
}

void csp_reboot_hook(void) {
    csp_print("csp_reboot_hook is not supported in the Darwin hosted profile\n");
}

void csp_shutdown_hook(void) {
    csp_print("csp_shutdown_hook is not supported in the Darwin hosted profile\n");
}
