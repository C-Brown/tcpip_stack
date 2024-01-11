#include "../../tcp_public.h"
#include "isis_intf.h"

bool isis_node_intf_is_enable(interface_t *intf) {
    isis_intf_info_t *isis_intf_info = GET_ISIS_INTF_INFO(intf);

    if (!isis_intf_info) {
        return false;
    } else {
        return true;
    }
}