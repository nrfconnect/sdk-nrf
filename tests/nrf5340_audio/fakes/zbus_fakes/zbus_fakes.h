#ifndef _ZBUS_FAKES_H_
#define _ZBUS_FAKES_H_

#include <zephyr/fff.h>
#include <zephyr/zbus/zbus.h>

DECLARE_FAKE_VALUE_FUNC(int, zbus_chan_pub, const struct zbus_channel *, const void *, k_timeout_t);

#endif /* _ZBUS_FAKES_H_ */
