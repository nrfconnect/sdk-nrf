/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <bluetooth/common/svc_common.h>

struct svc_el_pool {
	void *elements;
	u16_t cnt;
};

#if CONFIG_NRF_BT_UUID_16_POOL_SIZE != 0
static struct bt_uuid_16 uuid_16_tab[CONFIG_NRF_BT_UUID_16_POOL_SIZE];
#define BT_UUID_16_TAB uuid_16_tab
#else
#define BT_UUID_16_TAB NULL
#endif

#if CONFIG_NRF_BT_UUID_32_POOL_SIZE != 0
static struct bt_uuid_32 uuid_32_tab[CONFIG_NRF_BT_UUID_32_POOL_SIZE];
#define BT_UUID_32_TAB uuid_32_tab
#else
#define BT_UUID_32_TAB NULL
#endif

#if CONFIG_NRF_BT_UUID_128_POOL_SIZE != 0
static struct bt_uuid_128 uuid_128_tab[CONFIG_NRF_BT_UUID_128_POOL_SIZE];
#define BT_UUID_128_TAB uuid_128_tab
#else
#define BT_UUID_128_TAB NULL
#endif

#if CONFIG_NRF_BT_CHRC_POOL_SIZE != 0
static struct bt_gatt_chrc chrc_tab[CONFIG_NRF_BT_CHRC_POOL_SIZE];
#define BT_GATT_CHRC_TAB chrc_tab
#else
#define BT_GATT_CHRC_TAB NULL
#endif

#if CONFIG_NRF_BT_CCC_POOL_SIZE != 0
static struct _bt_gatt_ccc ccc_tab[CONFIG_NRF_BT_CCC_POOL_SIZE];
#define BT_GATT_CCC_TAB ccc_tab
#else
#define BT_GATT_CCC_TAB NULL
#endif

static struct svc_el_pool uuid_16_pool = { .elements = BT_UUID_16_TAB };
static struct svc_el_pool uuid_32_pool = { .elements = BT_UUID_32_TAB };
static struct svc_el_pool uuid_128_pool = { .elements = BT_UUID_128_TAB };
static struct svc_el_pool chrc_pool = { .elements = BT_GATT_CHRC_TAB };
static struct svc_el_pool ccc_pool = { .elements = BT_GATT_CCC_TAB };

static void uuid_16_get(struct bt_uuid **const uuid,
			struct svc_el_pool *const uuid_pool)
{
	__ASSERT(uuid_pool->cnt < CONFIG_NRF_BT_UUID_16_POOL_SIZE,
		 "No more UUID16s in the pool!");

	*uuid = (struct bt_uuid *)
		&((struct bt_uuid_16 *) uuid_pool->elements)[uuid_pool->cnt];
	uuid_pool->cnt++;
}

static void uuid_32_get(struct bt_uuid **const uuid,
			struct svc_el_pool *const uuid_pool)
{
	__ASSERT(uuid_pool->cnt < CONFIG_NRF_BT_UUID_32_POOL_SIZE,
		 "No more UUID32s in the pool!");

	*uuid = (struct bt_uuid *)
		&((struct bt_uuid_32 *) uuid_pool->elements)[uuid_pool->cnt];
	uuid_pool->cnt++;
}

static void uuid_128_get(struct bt_uuid **const uuid,
			 struct svc_el_pool *const uuid_pool)
{
	__ASSERT(uuid_pool->cnt < CONFIG_NRF_BT_UUID_128_POOL_SIZE,
		 "No more UUID128s in the pool!");

	*uuid = (struct bt_uuid *)
		&((struct bt_uuid_128 *) uuid_pool->elements)[uuid_pool->cnt];
	uuid_pool->cnt++;
}

static void chrc_get(struct bt_gatt_chrc **const chrc)
{
	__ASSERT(chrc_pool.cnt < CONFIG_NRF_BT_CHRC_POOL_SIZE,
		 "No more chrc descriptors in the pool!");

	*chrc = &((struct bt_gatt_chrc *) chrc_pool.elements)[chrc_pool.cnt];
	chrc_pool.cnt++;
}

static void ccc_get(struct _bt_gatt_ccc **const ccc)
{
	__ASSERT(ccc_pool.cnt < CONFIG_NRF_BT_CCC_POOL_SIZE,
		 "No more chrc descriptors in the pool!");

	*ccc = &((struct _bt_gatt_ccc *) ccc_pool.elements)[ccc_pool.cnt];
	ccc_pool.cnt++;
}

static int uuid_register(struct bt_uuid **const dest_uuid,
			 struct bt_uuid const *const src_uuid)
{
	__ASSERT(*dest_uuid == NULL, "Overriding attribute UUID!");

	switch (src_uuid->type) {
	case BT_UUID_TYPE_16:
		uuid_16_get(dest_uuid, &uuid_16_pool);
		memcpy(*dest_uuid, src_uuid, sizeof(struct bt_uuid_16));
		break;

	case BT_UUID_TYPE_32:
		uuid_32_get(dest_uuid, &uuid_32_pool);
		memcpy(*dest_uuid, src_uuid, sizeof(struct bt_uuid_32));
		break;

	case BT_UUID_TYPE_128:
		uuid_128_get(dest_uuid, &uuid_128_pool);
		memcpy(*dest_uuid, src_uuid, sizeof(struct bt_uuid_128));
		break;

	default:
		__ASSERT(false, "Unknown UUID type");
	}

	return 0;
}

int primary_svc_register(struct bt_gatt_service *const svc,
			 struct bt_uuid const *const svc_uuid)
{
	struct bt_gatt_attr *attr = &svc->attrs[svc->attr_count];
	struct bt_uuid      *uuid_gatt_primary = BT_UUID_GATT_PRIMARY;

	uuid_register((struct bt_uuid **) &attr->uuid, uuid_gatt_primary);
	uuid_register((struct bt_uuid **) &attr->user_data, svc_uuid);
	attr->perm = BT_GATT_PERM_READ;
	attr->read = bt_gatt_attr_read_service;

	svc->attr_count++;
	return 0;
}

int chrc_register(struct bt_gatt_service *const svc,
		  struct bt_gatt_chrc const *const chrc)
{
	struct bt_gatt_attr *attr = &svc->attrs[svc->attr_count];
	struct bt_uuid      *uuid_gatt_chrc = BT_UUID_GATT_CHRC;
	struct bt_gatt_chrc *dest_chrc;

	uuid_register((struct bt_uuid **) &attr->uuid, uuid_gatt_chrc);
	attr->perm = BT_GATT_PERM_READ;
	attr->read = bt_gatt_attr_read_chrc;

	/* Register user data for characteristic. */
	chrc_get((struct bt_gatt_chrc **) &attr->user_data);
	dest_chrc = (struct bt_gatt_chrc *) attr->user_data;
	dest_chrc->properties = chrc->properties;
	uuid_register((struct bt_uuid **) &dest_chrc->uuid, chrc->uuid);

	svc->attr_count++;
	return 0;
}

int descriptor_register(struct bt_gatt_service *const svc,
			struct bt_gatt_attr const *const descriptor)
{
	struct bt_gatt_attr *attr = &svc->attrs[svc->attr_count];

	memcpy(attr, descriptor, sizeof(struct bt_gatt_attr));
	attr->uuid = NULL;
	uuid_register((struct bt_uuid **) &attr->uuid, descriptor->uuid);

	svc->attr_count++;
	return 0;
}

int ccc_register(struct bt_gatt_service *const svc,
		 struct _bt_gatt_ccc const *const ccc)
{
	struct bt_gatt_attr *attr = &svc->attrs[svc->attr_count];
	struct bt_uuid      *uuid_gatt_ccc = BT_UUID_GATT_CCC;

	uuid_register((struct bt_uuid **) &attr->uuid, uuid_gatt_ccc);
	attr->perm = BT_GATT_PERM_READ | BT_GATT_PERM_WRITE;
	attr->read = bt_gatt_attr_read_ccc;
	attr->write = bt_gatt_attr_write_ccc;
	ccc_get((struct _bt_gatt_ccc **) &attr->user_data);
	memcpy(attr->user_data, ccc, sizeof(struct _bt_gatt_ccc));

	svc->attr_count++;
	return 0;
}

