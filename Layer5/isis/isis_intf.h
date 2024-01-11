#ifndef __ISIS_INTF__
#define __ISIS_INTF__

bool isis_node_intf_is_enable(interface_t *intf);

#define GET_ISIS_INTF_INFO(interface_ptr) ((isis_intf_info_t *)(interface_ptr->intf_nw_props.isis_intf_info))

typedef struct isis_intf_info_ {


} isis_intf_info_t;

#endif