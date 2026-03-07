
#include "zbus_fakes.h"

DEFINE_FAKE_VALUE_FUNC(int, zbus_chan_pub, const struct zbus_channel *, const void *, k_timeout_t);
