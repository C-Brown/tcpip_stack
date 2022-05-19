#include <stdbool.h>
#include "../../tcp_public.h"
#include "isis_rtr.h"
#include "isis_policy.h"

extern void
isis_ipv4_rt_notif_cbk (
        void *rt_notif_data, size_t arg_size) ;

int
isis_config_import_policy(node_t *node, const char *access_lst_name) {

    isis_node_info_t *node_info;

    access_list_t *access_lst = acl_lookup_access_list(node, access_lst_name);
    
    if (!access_lst) {
        printf ("Error : Access List Do Not Exist\n");
        return -1;
    }

    node_info = ISIS_NODE_INFO(node);

    if (!isis_is_protocol_enable_on_node(node) ||
          isis_is_protocol_shutdown_in_progress(node)) {
        return -1;
    }

    if (node_info->import_policy == access_lst ) return 0;

    if (node_info->import_policy &&
        node_info->import_policy != access_lst) {

        printf ("Error : Other Import policy %s is already being used\n",
            node_info->import_policy->name);
        return -1;
    }

    node_info->import_policy = access_lst;
    access_list_reference(access_lst);
    isis_schedule_spf_job(node, ISIS_EVENT_ADMIN_CONFIG_CHANGED_BIT);
    return 0;
}

int
isis_config_export_policy(node_t *node, const char *access_lst_name) {

    isis_node_info_t *node_info;

    access_list_t *access_lst = acl_lookup_access_list(node, access_lst_name);
    
    if (!access_lst) {
        printf ("Error : Access List Do Not Exist\n");
        return -1;
    }

    node_info = ISIS_NODE_INFO(node);

    if ( !isis_is_protocol_enable_on_node(node) ||
          isis_is_protocol_shutdown_in_progress(node)) {
        return -1;
    }

    if (node_info->export_policy == access_lst ) return 0;

    if (node_info->export_policy &&
         node_info->export_policy != access_lst) {

        printf ("Error : Other Export policy %s is already being used\n",
            node_info->export_policy->name);
        return -1;
    }

    node_info->export_policy = access_lst;
    access_list_reference(access_lst);
    nfc_ipv4_rt_request_flash (node, isis_ipv4_rt_notif_cbk);
    return 0;
}

int
isis_unconfig_import_policy(node_t *node, const char *access_lst_name) {

    access_list_t *import_policy;
    isis_node_info_t *node_info;

    node_info = ISIS_NODE_INFO(node);

    if (!node_info) return 0;

    if (!node_info->import_policy) return 0;

    if (access_lst_name) {
        
        import_policy = acl_lookup_access_list(node, access_lst_name);
        if (!import_policy) {
            printf ("Error : Access List Do Not Exist\n");
            return -1;
        }
    }
    else {
        import_policy = node_info->import_policy;
    }

    if (!import_policy && !access_lst_name) return 0;

    access_list_dereference(node_info->import_policy);
    node_info->import_policy = NULL;
    isis_schedule_spf_job(node, ISIS_EVENT_ADMIN_CONFIG_CHANGED_BIT);
    return 0;
}

int
isis_unconfig_export_policy(node_t *node, const char *access_lst_name) {

    access_list_t *export_policy;
    isis_node_info_t *node_info;

    node_info = ISIS_NODE_INFO(node);

    if (!node_info)
        return 0;

    if (!node_info->import_policy)
        return 0;

    if (access_lst_name) {

        export_policy = acl_lookup_access_list(node, access_lst_name);
        if (!export_policy) {
            printf("Error : Access List Do Not Exist\n");
            return -1;
        }
    }
    else
    {
        export_policy = node_info->export_policy;
    }

    if (!export_policy && !access_lst_name)
        return 0;

    access_list_dereference(node_info->export_policy);
    node_info->export_policy = NULL;
    nfc_ipv4_rt_request_flash (node, isis_ipv4_rt_notif_cbk);
    return 0;
}

bool
isis_evaluate_export_policy (node_t *node, access_list_t *policy, l3_route_t *route) {

    if (!policy) return true;

    uint32_t ip_addr = tcp_ip_covert_ip_p_to_n(route->dest);
    uint16_t mask = (uint16_t)route->mask;

    if (access_list_evaluate (policy, 0, 0, ip_addr, mask, 0, 0) == ACL_PERMIT) {
        return true;
    }

    return false;
}

bool
isis_evaluate_import_policy (node_t *node, access_list_t *policy, uint32_t prefix) {

    if (!policy) return true;

    if (access_list_evaluate (policy, 0, 0, prefix, 0, 0, 0) == ACL_PERMIT) {
        return true;
    }

    return false;
}

void
isis_acl_change(node_t *node, access_list_t *access_list); 

void
isis_acl_change(node_t *node, access_list_t *access_list) {

    isis_node_info_t *node_info;

    if (!isis_is_protocol_enable_on_node(node) ||
          isis_is_protocol_shutdown_in_progress(node)) return;

    node_info = ISIS_NODE_INFO(node);

    if (node_info->import_policy == access_list) {
         isis_schedule_spf_job(node, ISIS_EVENT_ADMIN_CONFIG_CHANGED_BIT);
    }

    if (node_info->export_policy == access_list) {
         nfc_ipv4_rt_request_flash (node, isis_ipv4_rt_notif_cbk);
    }
}