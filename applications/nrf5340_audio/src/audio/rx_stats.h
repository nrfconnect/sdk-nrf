#ifndef _RX_STATS_H_
#define _RX_STATS_H_

#include "audio_defines.h"

#include <zephyr/bluetooth/audio/bap.h>

int rx_stats_stream_recv(struct bt_bap_stream const *const stream, struct audio_metadata meta);

int rx_stats_stream_clear(struct bt_bap_stream const *const stream);

int rx_stats_stream_start(struct bt_bap_stream const *const stream);

#endif /* _RX_STATS_H_ */
