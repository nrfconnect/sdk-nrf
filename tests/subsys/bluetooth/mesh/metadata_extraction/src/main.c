/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Bluetooth Mesh DFU Target role sample
 */
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/mesh.h>
#include <bluetooth/mesh/models.h>


static struct bt_mesh_health_srv health_srv;

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_model models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(1, models, BT_MESH_MODEL_NONE),
};

#ifndef CONFIG_COMP_DATA_LAYOUT_SINGLE
static struct bt_mesh_onoff_cli onoff_cli;
static struct bt_mesh_lvl_cli lvl_cli;

static struct bt_mesh_model extra_models[] = {
	BT_MESH_MODEL_ONOFF_CLI(&onoff_cli),
	BT_MESH_MODEL_LVL_CLI(&lvl_cli),
};

static struct bt_mesh_elem elements_alt[] = {
	BT_MESH_ELEM(1, models, BT_MESH_MODEL_NONE),
	BT_MESH_ELEM(2, extra_models, BT_MESH_MODEL_NONE),
};
#endif

#ifdef CONFIG_COMP_DATA_LAYOUT_ARRAY
const struct bt_mesh_comp comp[2] = {
	{
		.cid = 1,
		.pid = 2,
		.vid = 3,
		.elem = elements,
		.elem_count = ARRAY_SIZE(elements),
	},
	{
		.cid = 4,
		.pid = 5,
		.vid = 6,
		.elem = elements_alt,
		.elem_count = ARRAY_SIZE(elements_alt),
	},
};
#else /* defined(COMP_DATA_LAYOUT_SINGLE) || defined(COMP_DATA_LAYOUT_MULTIPLE) */
static const struct bt_mesh_comp comp = {
	.cid = 1,
	.pid = 2,
	.vid = 3,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};
#ifdef CONFIG_COMP_DATA_LAYOUT_MULTIPLE
static const struct bt_mesh_comp comp_alt = {
	.cid = 4,
	.pid = 5,
	.vid = 6,
	.elem = elements_alt,
	.elem_count = ARRAY_SIZE(elements_alt),
};
#endif /* CONFIG_COMP_DATA_LAYOUT_MULTIPLE */
#endif /* defined(COMP_DATA_LAYOUT_SINGLE) || defined(COMP_DATA_LAYOUT_MULTIPLE) */

static const uint8_t dev_uuid[16];

static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
};

#ifndef CONFIG_COMP_DATA_LAYOUT_SINGLE
/* Since the value of this variable is never changed by the code, the compiler
 * might optimize it out, including any conditions that depend on it (and by
 * by extension, the entire alternate composition data structure). To prevent
 * this, the `volatile` qualifier is used.
 */
volatile bool use_alt;
#endif

static void bt_ready(int err)
{
	if (err) {
		return;
	}

#ifdef CONFIG_COMP_DATA_LAYOUT_ARRAY
	if (use_alt) {
		bt_mesh_init(&prov, &comp[1]);
	} else {
		bt_mesh_init(&prov, &comp[0]);
	}
#else /* defined(COMP_DATA_LAYOUT_SINGLE) || defined(COMP_DATA_LAYOUT_MULTIPLE) */
#ifdef CONFIG_COMP_DATA_LAYOUT_MULTIPLE
	if (use_alt) {
		bt_mesh_init(&prov, &comp_alt);
	} else
#endif /* CONFIG_COMP_DATA_LAYOUT_MULTIPLE */
	{
		bt_mesh_init(&prov, &comp);
	}
#endif /* defined(COMP_DATA_LAYOUT_SINGLE) || defined(COMP_DATA_LAYOUT_MULTIPLE) */
}

int main(void)
{
	bt_enable(bt_ready);

	return 0;
}
