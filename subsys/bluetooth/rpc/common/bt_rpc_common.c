

#include <zephyr/types.h>
#include <zephyr.h>
#include <device.h>

#include "nrf_rpc_cbor.h"

#include "bt_rpc_common.h"


NRF_RPC_GROUP_DEFINE(bt_rpc_grp, "bt_rpc", NULL, NULL, NULL);


#ifdef CONFIG_BT_RPC_INITIALIZE_NRF_RPC


static void err_handler(const struct nrf_rpc_err_report *report)
{
	printk("nRF RPC error %d ocurred. See nRF RPC logs for more details.",
	       report->code);
	k_oops();
}


static int serialization_init(struct device *dev)
{
	ARG_UNUSED(dev);

	int err;

	printk("Init begin\n"); // TODO: Log instead of printk

	err = nrf_rpc_init(err_handler);
	if (err) {
		return -EINVAL;
	}

	printk("Init done\n");

	return 0;
}


SYS_INIT(serialization_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);


#endif /* CONFIG_BT_RPC_INITIALIZE_NRF_RPC */
