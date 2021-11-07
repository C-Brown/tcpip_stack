#include "../../tcp_public.h"
#include "isis_rtr.h"
#include "isis_adjacency.h"
#include "isis_intf.h"
#include "isis_const.h"
#include "isis_pkt.h"

/* Timer APIs */

static void
isis_timer_expire_delete_adjacency_cb(void *arg, uint32_t arg_size) {

    if (!arg) return;
    
    isis_adjacency_t *adjacency = (isis_adjacency_t *)arg;

    interface_t *intf = adjacency->intf;
    isis_intf_info_t *intf_info = ISIS_INTF_INFO(intf);
    intf_info->adjacency = NULL;

    timer_de_register_app_event(adjacency->delete_timer);
    adjacency->delete_timer = NULL;

    assert(!adjacency->expiry_timer);

    free(adjacency);
}

static void
isis_adjacency_start_delete_timer(
        isis_adjacency_t *adjacency) {

    if (adjacency->delete_timer)
        return;

    adjacency->delete_timer = timer_register_app_event(
        node_get_timer_instance(adjacency->intf->att_node),
        isis_timer_expire_delete_adjacency_cb,
        (void *)adjacency,
        sizeof(isis_adjacency_t),
        ISIS_ADJ_DEFAULT_DELETE_TIME,
        0);
}

static void
isis_adjacency_stop_delete_timer(
        isis_adjacency_t *adjacency) {

    if (!adjacency->delete_timer) return;

    timer_de_register_app_event(adjacency->delete_timer);
    adjacency->delete_timer = NULL;
}


static void
isis_timer_expire_down_adjacency_cb(void *arg, uint32_t arg_size) {

    if (!arg) return;

    isis_adjacency_t *adjacency = (isis_adjacency_t *)arg;

    timer_de_register_app_event(adjacency->expiry_timer);
    adjacency->expiry_timer = NULL;

     isis_change_adjacency_state(adjacency, ISIS_ADJ_STATE_DOWN);
}

static void
isis_adjacency_start_expiry_timer(
        isis_adjacency_t *adjacency) {

    if (adjacency->expiry_timer) return;

    adjacency->expiry_timer = timer_register_app_event(
                                node_get_timer_instance(adjacency->intf->att_node),
                                isis_timer_expire_down_adjacency_cb,
                                (void *)adjacency, sizeof(isis_adjacency_t),
                                adjacency->hold_time * 1000,
                                0);
}


static void
isis_adjacency_stop_expiry_timer(
       isis_adjacency_t *adjacency) {

        if (!adjacency->expiry_timer) return;

        timer_de_register_app_event(adjacency->expiry_timer);
        adjacency->expiry_timer = NULL;
}


 static void
 isis_adjacency_refresh_expiry_timer(
        isis_adjacency_t *adjacency) {

        assert(adjacency->expiry_timer);

        timer_reschedule(adjacency->expiry_timer,
                    adjacency->hold_time * 1000);
}

static void
isis_adjacency_set_uptime(isis_adjacency_t *adjacency) {

    assert(adjacency->adj_state == ISIS_ADJ_STATE_UP);
    adjacency->uptime = time(NULL);
}

void
isis_change_adjacency_state(
            isis_adjacency_t *adjacency,
            isis_adj_state_t new_adj_state) {

    isis_adj_state_t old_adj_state = adjacency->adj_state;

    switch(old_adj_state) {

        case ISIS_ADJ_STATE_DOWN:

            switch(new_adj_state) {

                case ISIS_ADJ_STATE_DOWN:
                    break;
                case ISIS_ADJ_STATE_INIT:
                    adjacency->adj_state = new_adj_state;
                    isis_adjacency_stop_delete_timer(adjacency);
                    isis_adjacency_start_expiry_timer(adjacency);
                    break;
                case ISIS_ADJ_STATE_UP:
                break;
                default: ;
            }


        case ISIS_ADJ_STATE_INIT:

            switch(new_adj_state) {

                case ISIS_ADJ_STATE_DOWN:
                    adjacency->adj_state = new_adj_state;
                    isis_adjacency_stop_expiry_timer(adjacency);
                    isis_adjacency_start_delete_timer(adjacency);
                    break;
                case ISIS_ADJ_STATE_INIT:
                    break;
                case ISIS_ADJ_STATE_UP:
                    adjacency->adj_state = new_adj_state;
                    isis_adjacency_refresh_expiry_timer(adjacency);
                    isis_adjacency_set_uptime(adjacency);
                    ISIS_NODE_INFO(adjacency->intf->att_node)->adj_up_count++;
                    isis_create_fresh_lsp_pkt(adjacency->intf->att_node);
                    break;
                default: ;
            }


        case ISIS_ADJ_STATE_UP:

                switch(new_adj_state) {

                case ISIS_ADJ_STATE_DOWN:
                     adjacency->adj_state = new_adj_state;
                    isis_adjacency_stop_expiry_timer(adjacency);
                    isis_adjacency_start_delete_timer(adjacency);
                    ISIS_NODE_INFO(adjacency->intf->att_node)->adj_up_count--;
                    isis_create_fresh_lsp_pkt(adjacency->intf->att_node);
                    break;
                case ISIS_ADJ_STATE_INIT:
                    break;
                case ISIS_ADJ_STATE_UP:
                    isis_adjacency_refresh_expiry_timer(adjacency);
                    break;
                default: ;
            }

        default: ;
    }
}

isis_adj_state_t 
isis_get_next_adj_state_on_receiving_next_hello(
    isis_adjacency_t *adjacency) {

    switch(adjacency->adj_state){
        case ISIS_ADJ_STATE_DOWN:
            return ISIS_ADJ_STATE_INIT;
        case ISIS_ADJ_STATE_INIT:
            return ISIS_ADJ_STATE_UP;
        case ISIS_ADJ_STATE_UP:
            return ISIS_ADJ_STATE_UP;
        default : ; 
    }   
}

void
 isis_delete_adjacency(isis_adjacency_t * adjacency) {

     interface_t *intf = adjacency->intf;

    isis_intf_info_t *intf_info = ISIS_INTF_INFO(intf);

    assert(intf_info);

    intf_info->adjacency = NULL;

    isis_adjacency_stop_expiry_timer(adjacency);
    isis_adjacency_stop_delete_timer(adjacency);

    if (adjacency->adj_state == ISIS_ADJ_STATE_UP) {
        ISIS_NODE_INFO(adjacency->intf->att_node)->adj_up_count--;
    }
    free(adjacency);
 }


void
isis_update_interface_adjacency_from_hello(
        interface_t *iif,
        byte *hello_tlv_buffer,
        size_t tlv_buff_size) {

  /* Algorithm : 

    1. If isis_adjacency_t do not exist on iif, create a new one in DOWN state
    2. Iterate over hello_tlv_buffer and copy all 6 TLVs values from hello to Adjacency members 
    3. Track if there is change in any attribute of existing Adjacency in step 2 (bool nbr_attr_changed )
    4. Keep track if Adj is newly created (bool new_adj )
 */ 

    bool new_adj = false;
    bool nbr_attr_changed = false;
    uint32_t ip_addr_int;
    bool force_bring_down_adj = false;
    bool regen_lsp = false;
    isis_intf_info_t *isis_intf_info = ISIS_INTF_INFO(iif);

    isis_adjacency_t *adjacency = isis_intf_info->adjacency;

    if (!adjacency) {

        adjacency = calloc(1, sizeof(isis_adjacency_t));
        adjacency->intf = iif;
        new_adj = true;
        adjacency->adj_state = ISIS_ADJ_STATE_DOWN; 
        isis_intf_info->adjacency = adjacency;
        isis_adjacency_start_delete_timer(adjacency);
    }
    
    byte tlv_type, tlv_len, *tlv_value = NULL;

    ITERATE_TLV_BEGIN(hello_tlv_buffer, tlv_type, tlv_len, tlv_value, tlv_buff_size){

        switch(tlv_type) {

            case ISIS_TLV_HOSTNAME:
                if (memcmp(adjacency->nbr_name, tlv_value, tlv_len)) {
                    nbr_attr_changed = true;
                    regen_lsp = true;
                    memcpy(adjacency->nbr_name, tlv_value, tlv_len);
                }
            break;
            case ISIS_TLV_RTR_ID:
                if (adjacency->nbr_rtr_id != *(uint32_t *)(tlv_value)) {
                    nbr_attr_changed = true;
                    adjacency->nbr_rtr_id = *(uint32_t *)(tlv_value);
                }
            break;    
            case ISIS_TLV_IF_IP:
                memcpy((byte *)&ip_addr_int, tlv_value, sizeof(ip_addr_int));
                if (adjacency->nbr_intf_ip != ip_addr_int ) {
                    nbr_attr_changed = true;
                    adjacency->nbr_intf_ip = ip_addr_int;
                    force_bring_down_adj = true;
                    regen_lsp = true;
                }
            break;
            case ISIS_TLV_IF_INDEX:
            if (adjacency->remote_if_index != *(uint32_t *)tlv_value) {
                memcpy((byte *)&adjacency->remote_if_index, tlv_value, tlv_len);
                regen_lsp = true;
            }
            break;
            case ISIS_TLV_HOLD_TIME:
                adjacency->hold_time = *((uint32_t *)tlv_value);
            break;
            case ISIS_TLV_METRIC_VAL:
                if (adjacency->cost != *((uint32_t *)tlv_value)) {
                    adjacency->cost = *((uint32_t *)tlv_value);
                    nbr_attr_changed = true;
                    regen_lsp = true;
                }
            break;
            case ISIS_TLV_IF_MAC:
                if (memcmp(adjacency->nbr_mac.mac, (byte *)tlv_value, tlv_len)) {
                    memcpy(adjacency->nbr_mac.mac, tlv_value, tlv_len);
                     force_bring_down_adj = true;
                }
            break;
            default: ;
        }
    }  ITERATE_TLV_END(hello_tlv_buffer, tlv_type, tlv_len, tlv_value, tlv_buff_size);

    if (!new_adj) {

        isis_adj_state_t next_state;

        if (force_bring_down_adj) {
            next_state = ISIS_ADJ_STATE_DOWN;
        }
        else {
            next_state = isis_get_next_adj_state_on_receiving_next_hello(adjacency);
        }
        isis_change_adjacency_state(adjacency, next_state);
    }

    if (!new_adj && regen_lsp) {
        isis_create_fresh_lsp_pkt(adjacency->intf->att_node);
    }
    
    ISIS_INTF_INCREMENT_STATS(iif, good_hello_pkt_recvd);
 }

void
isis_show_adjacency( isis_adjacency_t *adjacency,
                                    uint8_t tab_spaces) {

    char *ip_addr_str;

    PRINT_TABS(tab_spaces);
    ip_addr_str = tcp_ip_covert_ip_n_to_p (adjacency->nbr_rtr_id, 0);
    printf("Nbr : %s(%s)\n", adjacency->nbr_name, ip_addr_str);

    PRINT_TABS(tab_spaces);
    ip_addr_str = tcp_ip_covert_ip_n_to_p( adjacency->nbr_intf_ip, 0);
    printf("Nbr intf ip : %s  ifindex : %u\n",
        ip_addr_str,
        adjacency->remote_if_index);
        
    PRINT_TABS(tab_spaces);
    printf("Nbr Mac Addr : %02x:%02x:%02x:%02x:%02x:%02x\n", 
            adjacency->nbr_mac.mac[0], 
            adjacency->nbr_mac.mac[1], 
            adjacency->nbr_mac.mac[2], 
            adjacency->nbr_mac.mac[3], 
            adjacency->nbr_mac.mac[4], 
            adjacency->nbr_mac.mac[5]);

    PRINT_TABS(tab_spaces);
    printf("State : %s   HT : %u sec   Cost : %u\n",
        isis_adj_state_str(adjacency->adj_state),
        adjacency->hold_time,
        adjacency->cost);

    PRINT_TABS(tab_spaces);

    if (adjacency->expiry_timer) {
        printf("Expiry Timer Remaining : %u msec\n",
                wt_get_remaining_time(adjacency->expiry_timer));
    }
    else {
        printf("Expiry Timer : Nil\n");
    }

    PRINT_TABS(tab_spaces);

    if (adjacency->delete_timer) {
         printf("Delete Timer Remaining : %u msec\n",
            wt_get_remaining_time(adjacency->delete_timer));
    }
    else {
        printf("Delete Timer : Nil\n");
    }

    if (adjacency->adj_state == ISIS_ADJ_STATE_UP) {

        PRINT_TABS(tab_spaces);
        printf("Up Time : %s\n", hrs_min_sec_format(
                (unsigned int)difftime(time(NULL), adjacency->uptime)));
    }
}

/* TLV 22 handling fns */
uint8_t
isis_nbr_tlv_encode_size(isis_adjacency_t *adjacency,
                                          uint8_t *total_subtlv_len) {

    uint8_t ptlv_len = 0;
    uint8_t subtlv_len = 0;

    ptlv_len += TLV_OVERHEAD_SIZE ;
    ptlv_len += 4; /* nbr loopback address */
    ptlv_len += 4; /* metric */
    ptlv_len += 1; /* total subtlv _len */

    subtlv_len += TLV_OVERHEAD_SIZE + 4 + 4;
    subtlv_len += TLV_OVERHEAD_SIZE + 4;
    subtlv_len += TLV_OVERHEAD_SIZE + 4;

    ptlv_len += subtlv_len;

    *total_subtlv_len = subtlv_len;
    return ptlv_len;
}


byte *
isis_encode_nbr_tlv(isis_adjacency_t *adjacency, 
                                  byte *buff,
                                  uint16_t *tlv_len) {

    uint8_t subtlv_len = 0;
    byte *start_buff = buff;

    *tlv_len = isis_nbr_tlv_encode_size(adjacency, &subtlv_len);

    /* now start encoding the data into buffer */

    *start_buff = ISIS_IS_REACH_TLV;
    start_buff += 1;

    *start_buff = *tlv_len - TLV_OVERHEAD_SIZE;
    start_buff += 1;

    /*loopback address */
    memcpy(start_buff, (byte *)&adjacency->nbr_rtr_id, sizeof(adjacency->nbr_rtr_id));
    start_buff += sizeof(adjacency->nbr_rtr_id);

    /*Metric */
    uint32_t metric = adjacency->cost;
    memcpy(start_buff, (byte *)&metric, sizeof (metric));
    start_buff += sizeof(uint32_t);

    /* Total Subtlv len */
    memcpy(start_buff, (byte *)&subtlv_len, sizeof(uint8_t));
    start_buff += sizeof(uint8_t);

    /* Now we will encode subtlv 4, i.e.
    local ifindex and remote if index */

    uint32_t if_indexes[2];

    if_indexes[0] = IF_INDEX(adjacency->intf);
    if_indexes[1] = adjacency->remote_if_index;

    start_buff = tlv_buffer_insert_tlv(start_buff,
                        ISIS_TLV_IF_INDEX, 8, 
                        (byte *)if_indexes);

    /* Encode Subtlv 6 i.e. local ip address of interface */

    uint32_t ip_addr = tcp_ip_covert_ip_p_to_n(IF_IP(adjacency->intf));

    start_buff = tlv_buffer_insert_tlv(start_buff,
                            ISIS_TLV_LOCAL_IP, 4,
                            (byte *)&ip_addr);
                            
    /* Encode subtlv 8 i.e. remote IP Address */

    start_buff = tlv_buffer_insert_tlv(start_buff,
                            ISIS_TLV_REMOTE_IP, 4,
                            (byte *)&adjacency->nbr_intf_ip);
                            
    return start_buff;
}

byte *
isis_encode_all_nbr_tlvs(node_t *node, byte *buff) {

    interface_t *intf;
    uint16_t bytes_encoded;
    isis_adjacency_t *adjacency;

    isis_node_info_t *node_info = ISIS_NODE_INFO(node);

    if (!isis_is_protocol_enable_on_node(node)) return buff;

    ITERATE_NODE_INTERFACES_BEGIN(node, intf) {

        if (!isis_node_intf_is_enable(intf)) continue;
        adjacency = ISIS_INTF_INFO(intf)->adjacency;
        if (!adjacency) continue;
        if (adjacency->adj_state != ISIS_ADJ_STATE_UP) continue;
        buff = isis_encode_nbr_tlv(adjacency, buff, &bytes_encoded);

   } ITERATE_NODE_INTERFACES_END(node, intf);

    return buff;
}


uint16_t
isis_size_to_encode_all_nbr_tlv(node_t *node) {

    interface_t *intf;
    uint16_t bytes_needed;
    uint8_t subtlv_bytes_needed;
    isis_adjacency_t *adjacency;

    isis_node_info_t *node_info = ISIS_NODE_INFO(node);

    bytes_needed = 0;
    subtlv_bytes_needed = 0;

    if (!isis_is_protocol_enable_on_node(node)) return 0;

    ITERATE_NODE_INTERFACES_BEGIN(node, intf) {

        if (!isis_node_intf_is_enable(intf)) continue;
        adjacency = ISIS_INTF_INFO(intf)->adjacency;
        if (adjacency == NULL) continue;
        if (adjacency->adj_state != ISIS_ADJ_STATE_UP) continue;
        bytes_needed += isis_nbr_tlv_encode_size(adjacency, &subtlv_bytes_needed);

   } ITERATE_NODE_INTERFACES_END(node, intf);

    return bytes_needed;
}

void
isis_show_all_adjacencies (node_t *node) {

     uint32_t rc = 0;
     interface_t *intf;
     isis_adjacency_t *adjacency;

    ITERATE_NODE_INTERFACES_BEGIN (node, intf) {

        if ( !isis_node_intf_is_enable(intf)) continue;
       
            adjacency = ISIS_INTF_INFO(intf)->adjacency;
            if (!adjacency) continue;

            printf( "%-16s   %-16s   %-6s   %s\n", 
            intf->if_name, adjacency->nbr_name,
            isis_adj_state_str(adjacency->adj_state),
            hrs_min_sec_format(
                (unsigned int)difftime(time(NULL), adjacency->uptime)));

    } ITERATE_NODE_INTERFACES_END (node, intf);
 }
