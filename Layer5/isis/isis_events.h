#ifndef __ISIS_EVENTS__
#define __ISIS_EVENTS__

#include <assert.h>

typedef enum isis_events_ {

    isis_event_none,
    #define ISIS_EVENT_NONE_BIT                 (isis_event_none)
    isis_event_adj_state_goes_up,
    #define ISIS_EVENT_ADJ_STATE_GOES_UP_BIT    (1 << isis_event_adj_state_goes_up)
    isis_event_adj_state_goes_down,
    #define ISIS_EVENT_ADJ_STATE_GOES_DOWN_BIT  (1 << isis_event_adj_state_goes_down)
    isis_event_protocol_enable,
    #define ISIS_EVENT_PROTOCOL_ENABLE_BIT      (1 << isis_event_protocol_enable)
    isis_event_protocol_disable,
    #define ISIS_EVENT_PROTOCOL_DISABLE_BIT     (1 << isis_event_protocol_disable)
    isis_event_protocol_disable_on_intf,
    #define ISIS_EVENT_PROTOCOL_DISABLE_ON_INTF_BIT     (1 << isis_event_protocol_disable_on_intf)
    isis_event_protocol_enable_on_intf,
    #define ISIS_EVENT_PROTOCOL_ENABLE_ON_INTF_BIT      (1 << isis_event_protocol_enable_on_intf)
    isis_event_nbr_ip_changed,
    #define ISIS_EVENT_NBR_IP_CHANGED_BIT               (1 << isis_event_nbr_ip_changed)
    isis_event_nbr_metric_changed,
    #define ISIS_EVENT_NBR_METRIC_CHANGED_BIT           (1 << isis_event_nbr_metric_changed)
    isis_event_nbr_rtr_id_changed,
    #define ISIS_EVENT_NBR_RTR_ID_CHANGED_BIT           (1 << isis_event_nbr_rtr_id_changed)
    /*lspdb update events begin*/
    isis_event_self_duplicate_lsp,
    #define ISIS_EVENT_SELF_DUPLICATE_LSP_BIT           (1 << isis_event_self_duplicate_lsp)
    isis_event_self_fresh_lsp,
    #define ISIS_EVENT_SELF_FRESH_LSP_BIT               (1 << isis_event_self_fresh_lsp)
    isis_event_self_new_lsp,
    #define ISIS_EVENT_SELF_NEW_LSP_BIT                 (1 << isis_event_self_new_lsp)
    isis_event_self_old_lsp,
    #define ISIS_EVENT_SELF_OLD_LSP_BIT                 (1 << isis_event_self_old_lsp)
    isis_event_non_local_duplicate_lsp,
    #define ISIS_EVENT_NON_LOCAL_DUPLICATE_LSP_BIT      (1 << isis_event_non_local_duplicate_lsp)
    isis_event_non_local_fresh_lsp,
    #define ISIS_EVENT_NON_LOCAL_FRESH_LSP_BIT          (1 << isis_event_non_local_fresh_lsp)
    isis_event_non_local_new_lsp,
    #define ISIS_EVENT_NON_LOCAL_NEW_LSP_BIT            (1 << isis_event_non_local_new_lsp)
    isis_event_non_local_old_lsp,
    #define ISIS_EVENT_NON_LOCAL_OLD_LSP_BIT            (1 << isis_event_non_local_old_lsp)
    /*lspdb update events end*/
    isis_event_on_demand_flood,
    #define ISIS_EVENT_ON_DEMAND_FLOOD_BIT              (1 << isis_event_on_demand_flood)
    isis_event_periodic_lsp_generation,
    #define ISIS_EVENT_PERIODIC_LSP_GENERATION_BIT      (1 << isis_event_periodic_lsp_generation)
    isis_event_reconciliation_triggered,
    #define ISIS_EVENT_RECONCILIATION_TRIGGERED_BIT     (1 << isis_event_reconciliation_triggered)
    isis_event_reconciliation_restarted,
    #define ISIS_EVENT_RECONCILIATION_RESTARTED_BIT     (1 << isis_event_reconciliation_restarted)
    isis_event_reconciliation_exit,
    #define ISIS_EVENT_RECONCILIATION_EXIT_BIT          (1 << isis_event_reconciliation_exit)
    isis_event_admin_action_db_clear,
    #define ISIS_EVENT_ADMIN_ACTION_DB_CLEAR_BIT        (1 << isis_event_admin_action_db_clear)
    /* SPF related events */
    isis_event_spf_job_scheduled,
    #define ISIS_EVENT_SPF_JOB_SCHEDULED_BIT            (1 << isis_event_spf_job_scheduled)
    isis_event_spf_runs,
    #define ISIS_EVENT_SPF_RUNS_BIT                     (1 << isis_event_spf_runs)
    isis_event_max = isis_event_spf_runs + 1            /* Do not cross more than 63 */
    #define ISIS_EVENT_MAX                              (1 << isis_event_max)
} isis_event_type_t;

const char *
isis_event_str(isis_event_type_t isis_event_type);

static inline unsigned long
isis_event_to_event_bit(isis_event_type_t event_type) {
 
    switch(event_type) {
        case isis_event_none:
        return ISIS_EVENT_NONE_BIT;
        case isis_event_adj_state_goes_up:
        return ISIS_EVENT_ADJ_STATE_GOES_UP_BIT;
        case isis_event_adj_state_goes_down:
        return ISIS_EVENT_ADJ_STATE_GOES_DOWN_BIT;
        case isis_event_protocol_enable:
        return ISIS_EVENT_PROTOCOL_ENABLE_BIT;
        case isis_event_protocol_disable:
        return ISIS_EVENT_PROTOCOL_DISABLE_BIT;
        case isis_event_protocol_disable_on_intf:
        return ISIS_EVENT_PROTOCOL_DISABLE_ON_INTF_BIT;
        case isis_event_protocol_enable_on_intf:
        return ISIS_EVENT_PROTOCOL_ENABLE_ON_INTF_BIT;
        case isis_event_nbr_ip_changed:
        return ISIS_EVENT_NBR_IP_CHANGED_BIT;
        case isis_event_nbr_metric_changed:
        return ISIS_EVENT_NBR_METRIC_CHANGED_BIT;
        case isis_event_nbr_rtr_id_changed:
        return ISIS_EVENT_NBR_RTR_ID_CHANGED_BIT;
    /*lspdb update events begin*/
        case isis_event_self_duplicate_lsp:
        return ISIS_EVENT_SELF_DUPLICATE_LSP_BIT;
        case isis_event_self_fresh_lsp:
        return ISIS_EVENT_SELF_FRESH_LSP_BIT;
        case isis_event_self_new_lsp:
        return ISIS_EVENT_SELF_NEW_LSP_BIT;
        case isis_event_self_old_lsp:
        return ISIS_EVENT_SELF_OLD_LSP_BIT;
        case isis_event_non_local_duplicate_lsp:
        return ISIS_EVENT_NON_LOCAL_DUPLICATE_LSP_BIT;
        case isis_event_non_local_fresh_lsp:
        return ISIS_EVENT_NON_LOCAL_FRESH_LSP_BIT;
        case isis_event_non_local_new_lsp:
        return ISIS_EVENT_NON_LOCAL_NEW_LSP_BIT;
        case isis_event_non_local_old_lsp:
        return ISIS_EVENT_NON_LOCAL_OLD_LSP_BIT;
    /*lspdb update events end*/
        case isis_event_on_demand_flood:
        return ISIS_EVENT_ON_DEMAND_FLOOD_BIT;
        case isis_event_periodic_lsp_generation:
        return ISIS_EVENT_PERIODIC_LSP_GENERATION_BIT;
        case isis_event_reconciliation_triggered:
        return ISIS_EVENT_RECONCILIATION_TRIGGERED_BIT;
        case isis_event_reconciliation_restarted:
        return ISIS_EVENT_RECONCILIATION_RESTARTED_BIT;
        case isis_event_reconciliation_exit:
        return ISIS_EVENT_RECONCILIATION_EXIT_BIT;
        case isis_event_admin_action_db_clear:
        return ISIS_EVENT_ADMIN_ACTION_DB_CLEAR_BIT;
    /* SPF related events */
        case isis_event_spf_job_scheduled:
        return ISIS_EVENT_SPF_JOB_SCHEDULED_BIT;
        case isis_event_spf_runs:
        return ISIS_EVENT_SPF_RUNS_BIT;
        case isis_event_max: /* Do not cross more than this */
        return ISIS_EVENT_MAX;
        default:
            assert(0);
    }
}

#endif 