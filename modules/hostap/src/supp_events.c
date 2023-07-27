#include "supp_events.h"

#include "includes.h"
#include "common.h"

int send_wifi_mgmt_event(const char *ifname, enum net_event_wifi_cmd event, int status)
{
    const struct device *dev = device_get_binding(ifname);
    struct net_if *iface = net_if_lookup_by_dev(dev);

    if (!iface) {
        wpa_printf(MSG_ERROR, "Could not find iface for %s", ifname);
        return -ENODEV;
    }

    switch (event) {
    case NET_EVENT_WIFI_CMD_CONNECT_RESULT:
        wifi_mgmt_raise_connect_result_event(iface, status);
        break;
    case NET_EVENT_WIFI_CMD_DISCONNECT_RESULT:
        wifi_mgmt_raise_disconnect_result_event(iface, status);
        break;
    default:
        wpa_printf(MSG_ERROR, "Unsupported event %d", event);
        return -EINVAL;
    }

    return 0;
}
