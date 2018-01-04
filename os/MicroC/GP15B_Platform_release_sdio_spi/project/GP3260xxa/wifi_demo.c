#include <host_apis.h>
extern int ssv6xxx_dev_init(ssv6xxx_hw_mode hmode);

int wifi_demo(int argc, char *argv[])
{
    ssv6xxx_dev_init(SSV6XXX_HWM_STA);
}

