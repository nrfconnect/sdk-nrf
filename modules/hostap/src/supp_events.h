#ifndef __SUPP_EVENTS_H__
#define __SUPP_EVENTS_H__

#include <zephyr/net/wifi_mgmt.h>

int send_wifi_mgmt_event(const char *ifname, enum net_event_wifi_cmd event, int status);

#endif /* __SUPP_EVENTS_H__ */