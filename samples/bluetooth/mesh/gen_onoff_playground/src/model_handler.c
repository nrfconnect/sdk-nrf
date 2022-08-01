#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"

/* TODO: Add a status handler for the OnOff client. */

struct button {
	/** Current light status of the corresponding server. */
	bool status;
	/** Generic OnOff client instance for this switch. */
	struct bt_mesh_onoff_cli client;
};


static void status_handler(struct bt_mesh_onoff_cli *cli, struct bt_mesh_msg_ctx *ctx,

               const struct bt_mesh_onoff_status *status);


static void status_handler(struct bt_mesh_onoff_cli *cli,
			   struct bt_mesh_msg_ctx *ctx,
			   const struct bt_mesh_onoff_status *status)
{

    printk("Status handler: This is the onoff value %d \n", status->present_on_off);
	dk_set_led(3,status->present_on_off);

}
			   

static struct button buttons[] = {
#if DT_NODE_EXISTS(DT_ALIAS(sw0))
	{ .client = BT_MESH_ONOFF_CLI_INIT(&status_handler) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw1))
	{ .client = BT_MESH_ONOFF_CLI_INIT(&status_handler) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw2))
	{ .client = BT_MESH_ONOFF_CLI_INIT(&status_handler) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(sw3))
	{ .client = BT_MESH_ONOFF_CLI_INIT(&status_handler) },
#endif

};

/* TODO: Add a set handler for the OnOff server. */

static void led_set(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_onoff_set *set,
		    struct bt_mesh_onoff_status *rsp);

/* TODO: Add a get handler for the OnOff server. */

static void led_get(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_onoff_status *rsp);


static const struct bt_mesh_onoff_srv_handlers onoff_handlers = {
	.set = led_set,
	.get = led_get,
};

static void led_get(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_onoff_status *rsp)
{
}

static void led_set(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_onoff_set *set,
		    struct bt_mesh_onoff_status *rsp)
{
	
	rsp->present_on_off=set->on_off;

	printk("Set handler: This is the onoff value %d \n", rsp->present_on_off);
	dk_set_led(1,rsp->present_on_off);


}



static struct bt_mesh_onoff_srv onoff_srv = BT_MESH_ONOFF_SRV_INIT(&onoff_handlers);


/** TODO: When button 1 is pressed, send a Generic OnOff Set
	 * message (with the configured publish parameters) using the
	 * OnOff client. The set message Onoff parameter shall toggle
	 * between true/false on consecutive button presses.
	*/


static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if (!bt_mesh_is_provisioned()) {
		return;
	}

	if (pressed & changed & BIT(0)) {
		struct bt_mesh_onoff_set set = {

			.on_off = !buttons[0].status,

		};

		int err = bt_mesh_onoff_cli_set(&buttons[0].client, NULL, &set, NULL);

		if (!err) {
			buttons[0].status = set.on_off;

		} else {
			printk("OnOff set failed.\n");
		}
	}
	}

	

static void attention_on(struct bt_mesh_model *mod)
{
	printk("Attention On\n");
}

static void attention_off(struct bt_mesh_model *mod)
{
	printk("Attention Off\n");
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(1,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_CFG_SRV,
			     BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
			     BT_MESH_MODEL_ONOFF_CLI(&buttons[0].client)),
		     BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(2,
		     BT_MESH_MODEL_LIST(
			     BT_MESH_MODEL_ONOFF_SRV(&onoff_srv)),
		     BT_MESH_MODEL_NONE),

};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	static struct button_handler button_handler = {
		.cb = button_handler_cb,
	};

	dk_button_handler_add(&button_handler);

	return &comp;
}
