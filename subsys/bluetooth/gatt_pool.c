/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <bluetooth/gatt_pool.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_gatt_pool, CONFIG_BT_GATT_POOL_LOG_LEVEL);


struct svc_el_pool {
	void *elements;
	atomic_t *locks;
};

#if CONFIG_BT_GATT_UUID16_POOL_SIZE != 0
static struct bt_uuid_16 uuid_16_tab[CONFIG_BT_GATT_UUID16_POOL_SIZE];
static ATOMIC_DEFINE(uuid_16_locks, ARRAY_SIZE(uuid_16_tab));
#define BT_UUID_16_TAB uuid_16_tab
#define BT_UUID_16_LOCKS uuid_16_locks
#else
#define BT_UUID_16_TAB NULL
#define BT_UUID_16_LOCKS NULL
#endif

#if CONFIG_BT_GATT_UUID32_POOL_SIZE != 0
static struct bt_uuid_32 uuid_32_tab[CONFIG_BT_GATT_UUID32_POOL_SIZE];
static ATOMIC_DEFINE(uuid_32_locks, ARRAY_SIZE(uuid_32_tab));
#define BT_UUID_32_TAB uuid_32_tab
#define BT_UUID_32_LOCKS uuid_32_locks
#else
#define BT_UUID_32_TAB NULL
#define BT_UUID_32_LOCKS NULL
#endif

#if CONFIG_BT_GATT_UUID128_POOL_SIZE != 0
static struct bt_uuid_128 uuid_128_tab[CONFIG_BT_GATT_UUID128_POOL_SIZE];
static ATOMIC_DEFINE(uuid_128_locks, ARRAY_SIZE(uuid_128_tab));
#define BT_UUID_128_TAB uuid_128_tab
#define BT_UUID_128_LOCKS uuid_128_locks
#else
#define BT_UUID_128_TAB NULL
#define BT_UUID_128_LOCKS NULL
#endif

#if CONFIG_BT_GATT_CHRC_POOL_SIZE != 0
static struct bt_gatt_chrc chrc_tab[CONFIG_BT_GATT_CHRC_POOL_SIZE];
static ATOMIC_DEFINE(chrc_locks, ARRAY_SIZE(chrc_tab));
#define BT_GATT_CHRC_TAB chrc_tab
#define BT_GATT_CHRC_LOCKS chrc_locks
#else
#define BT_GATT_CHRC_TAB NULL
#define BT_GATT_CHRC_LOCKS NULL
#endif

static struct svc_el_pool uuid_16_pool = {
	.elements = BT_UUID_16_TAB,
	.locks = BT_UUID_16_LOCKS,
};
static struct svc_el_pool uuid_32_pool = {
	.elements = BT_UUID_32_TAB,
	.locks = BT_UUID_32_LOCKS,
};
static struct svc_el_pool uuid_128_pool = {
	.elements = BT_UUID_128_TAB,
	.locks = BT_UUID_128_LOCKS,
};
static struct svc_el_pool chrc_pool = {
	.elements = BT_GATT_CHRC_TAB,
	.locks = BT_GATT_CHRC_LOCKS,
};

static struct bt_uuid const * const uuid_primary = BT_UUID_GATT_PRIMARY;
static struct bt_uuid const * const uuid_chrc = BT_UUID_GATT_CHRC;
static struct bt_uuid const * const uuid_ccc = BT_UUID_GATT_CCC;

#define EL_IN_POOL_VERIFY(pool, el)                                            \
	do {                                                                   \
		__ASSERT(pool != NULL, "Pool is uninitialized");               \
		__ASSERT(((uint32_t)el >= (uint32_t)pool) &&                         \
			     (((uint32_t)el) < (((uint32_t)pool) + sizeof(pool))),   \
			 "Element does not belong to the pool");               \
	} while ((0))

#define ADDR_2_INDEX(pool, el)                                                 \
	((((uint32_t)el) - ((uint32_t)pool)) / (sizeof(pool[0])))

static size_t free_element_find(struct svc_el_pool *el_pool, size_t el_cnt)
{
	__ASSERT((el_pool->elements != NULL) && (el_pool->locks != NULL),
		 "Pool uninitialized");

	for (size_t i = 0; i < el_cnt; i++) {
		if (!atomic_test_and_set_bit(el_pool->locks, i)) {
			return i;
		}
	}
	return el_cnt;
}

static int uuid_16_get(struct bt_uuid **uuid, struct svc_el_pool *uuid_pool)
{
	size_t ind = free_element_find(uuid_pool,
				       CONFIG_BT_GATT_UUID16_POOL_SIZE);

	if (ind >= CONFIG_BT_GATT_UUID16_POOL_SIZE) {
		LOG_ERR("No more UUID16s in the pool!");
		return -ENOMEM;
	}

	*uuid = (struct bt_uuid *)
		&((struct bt_uuid_16 *) uuid_pool->elements)[ind];
	return 0;
}

static int uuid_32_get(struct bt_uuid **uuid, struct svc_el_pool *uuid_pool)
{
	size_t ind = free_element_find(uuid_pool,
				       CONFIG_BT_GATT_UUID32_POOL_SIZE);

	if (ind >= CONFIG_BT_GATT_UUID32_POOL_SIZE) {
		LOG_ERR("No more UUID32s in the pool!");
		return -ENOMEM;
	}

	*uuid = (struct bt_uuid *)
		&((struct bt_uuid_32 *) uuid_pool->elements)[ind];
	return 0;
}

static int uuid_128_get(struct bt_uuid **uuid, struct svc_el_pool *uuid_pool)
{
	size_t ind = free_element_find(uuid_pool,
				       CONFIG_BT_GATT_UUID128_POOL_SIZE);

	if (ind >= CONFIG_BT_GATT_UUID128_POOL_SIZE) {
		LOG_ERR("No more UUID128s in the pool!");
		return -ENOMEM;
	}

	*uuid = (struct bt_uuid *)
		&((struct bt_uuid_128 *) uuid_pool->elements)[ind];
	return 0;
}

static int chrc_get(struct bt_gatt_chrc **chrc)
{
	size_t ind = free_element_find(&chrc_pool,
				       CONFIG_BT_GATT_CHRC_POOL_SIZE);

	if (ind >= CONFIG_BT_GATT_CHRC_POOL_SIZE) {
		LOG_ERR("No more chrc descriptors in the pool!");
		return -ENOMEM;
	}

	*chrc = &((struct bt_gatt_chrc *) chrc_pool.elements)[ind];
	return 0;
}

static void chrc_release(struct bt_gatt_chrc const *chrc)
{
	EL_IN_POOL_VERIFY(BT_GATT_CHRC_TAB, chrc);
	atomic_clear_bit(chrc_pool.locks,
			 ADDR_2_INDEX(BT_GATT_CHRC_TAB, chrc));
}

static int uuid_register(struct bt_uuid **dest_uuid,
			 struct bt_uuid const *src_uuid)
{
	int ret = -EINVAL;

	__ASSERT(*dest_uuid == NULL, "Overriding attribute UUID!");

	switch (src_uuid->type) {
	case BT_UUID_TYPE_16:
		ret = uuid_16_get(dest_uuid, &uuid_16_pool);
		if (!ret) {
			memcpy(*dest_uuid, src_uuid, sizeof(struct bt_uuid_16));
		}
		break;

	case BT_UUID_TYPE_32:
		ret = uuid_32_get(dest_uuid, &uuid_32_pool);
		if (!ret) {
			memcpy(*dest_uuid, src_uuid, sizeof(struct bt_uuid_32));
		}
		break;

	case BT_UUID_TYPE_128:
		ret = uuid_128_get(dest_uuid, &uuid_128_pool);
		if (!ret) {
			memcpy(*dest_uuid, src_uuid,
			       sizeof(struct bt_uuid_128));
		}
		break;

	default:
		LOG_ERR("Unknown UUID type");
		break;
	}
	return ret;
}

static void uuid_unregister(struct bt_uuid const *uuid)
{
	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		EL_IN_POOL_VERIFY(BT_UUID_16_TAB, uuid);
#if CONFIG_BT_GATT_UUID16_POOL_SIZE != 0
		atomic_clear_bit(uuid_16_pool.locks,
				 ADDR_2_INDEX(BT_UUID_16_TAB, uuid));
#endif
		break;

	case BT_UUID_TYPE_32:
		EL_IN_POOL_VERIFY(BT_UUID_32_TAB, uuid);
#if CONFIG_BT_GATT_UUID32_POOL_SIZE != 0
		atomic_clear_bit(uuid_32_pool.locks,
				 ADDR_2_INDEX(BT_UUID_32_TAB, uuid));
#endif
		break;

	case BT_UUID_TYPE_128:
		EL_IN_POOL_VERIFY(BT_UUID_128_TAB, uuid);
#if CONFIG_BT_GATT_UUID128_POOL_SIZE != 0
		atomic_clear_bit(uuid_128_pool.locks,
				 ADDR_2_INDEX(BT_UUID_128_TAB, uuid));
#endif
		break;

	default:
		__ASSERT(false, "Unknown UUID type");
	}
}

/** @brief Free a single attribute.
 *
 *  This function frees all the elements dynamically allocated for the single
 *  attribute.
 *
 *  @param attr The attribute to be released to the pool.
 *
 *  @note
 *  If the given attribute was not created using bt_gatt_pool functions,
 *  the result of this function is undefined.
 */
static void bt_gatt_pool_attr_free(struct bt_gatt_attr const *attr)
{
	if (!attr) {
		LOG_ERR("Attribute handle to free is NULL");
		return;
	}

	if (!bt_uuid_cmp(attr->uuid, uuid_primary)) {
		uuid_unregister(attr->user_data);
	} else if (!bt_uuid_cmp(attr->uuid, uuid_chrc)) {
		chrc_release(attr->user_data);
	} else if (!bt_uuid_cmp(attr->uuid, uuid_ccc)) {
		/* Nothing to release */
	} else {
		/* Either the characteristic value UUID or a descriptor
		 * created using bt_gatt_pool_desc_alloc.
		 */
		uuid_unregister(attr->uuid);
	}
}

int bt_gatt_pool_svc_alloc(struct bt_gatt_pool *gp,
			   struct bt_uuid const *svc_uuid)
{
	int ret;
	struct bt_gatt_attr *attr;
	struct bt_uuid *uuid = NULL;

	if (!gp || !gp->svc.attrs || !svc_uuid) {
		LOG_ERR("Invalid attribute");
		return -EINVAL;
	}
	if (gp->svc.attr_count >= gp->attr_array_size) {
		LOG_ERR("No space left on given svc");
		return -ENOSPC;
	}

	ret = uuid_register(&uuid, svc_uuid);
	if (ret) {
		return ret;
	}

	attr = &gp->svc.attrs[gp->svc.attr_count++];
	*attr = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(uuid_primary,
			BT_GATT_PERM_READ, bt_gatt_attr_read_service, NULL,
			uuid);

	return 0;
}

int bt_gatt_pool_chrc_alloc(struct bt_gatt_pool *gp, uint8_t props,
			    struct bt_gatt_attr const *attr)
{
	int ret;
	struct bt_gatt_attr *chrc_decl, *chrc_value;
	struct bt_gatt_chrc *chrc;
	struct bt_uuid *uuid = NULL;

	if (!gp || !gp->svc.attrs || !attr) {
		LOG_ERR("Invalid attribute");
		return -EINVAL;
	}
	if ((gp->svc.attr_count + 2) > gp->attr_array_size) {
		LOG_ERR("No space left on given svc");
		return -ENOSPC;
	}

	ret = chrc_get(&chrc);
	if (ret) {
		return ret;
	}

	ret = uuid_register(&uuid, attr->uuid);
	if (ret) {
		chrc_release(chrc);
		return ret;
	}

	/* Characteristic declaration attribute value */
	*chrc = (struct bt_gatt_chrc) { .uuid = uuid,
				       .value_handle = 0U,
				       .properties = props };

	/* Characteristic declaration attribute. */
	chrc_decl = &gp->svc.attrs[gp->svc.attr_count++];
	*chrc_decl = (struct bt_gatt_attr)BT_GATT_ATTRIBUTE(uuid_chrc,
			BT_GATT_PERM_READ, bt_gatt_attr_read_chrc, NULL,
			chrc);

	/* Characteristic value attribute. */
	chrc_value = &gp->svc.attrs[gp->svc.attr_count++];
	*chrc_value = *attr;
	chrc_value->uuid = uuid;

	return 0;
}

int bt_gatt_pool_desc_alloc(struct bt_gatt_pool *gp,
			    struct bt_gatt_attr const *descriptor)
{
	int ret;
	struct bt_gatt_attr *attr;
	struct bt_uuid *uuid = NULL;

	if (!gp || !gp->svc.attrs || !descriptor) {
		LOG_ERR("Invalid attribute");
		return -EINVAL;
	}
	if (!bt_uuid_cmp(descriptor->uuid, uuid_primary) ||
	    !bt_uuid_cmp(descriptor->uuid, uuid_chrc)    ||
	    !bt_uuid_cmp(descriptor->uuid, uuid_ccc)) {
		LOG_ERR("Wrong function used for special attribute allocation");
		return -EINVAL;
	}
	if (gp->svc.attr_count >= gp->attr_array_size) {
		LOG_ERR("No space left on given svc");
		return -ENOSPC;
	}

	ret = uuid_register(&uuid, descriptor->uuid);
	if (ret) {
		return ret;
	}

	attr = &gp->svc.attrs[gp->svc.attr_count++];
	*attr = *descriptor;
	attr->uuid = uuid;

	return 0;
}

int bt_gatt_pool_ccc_alloc(struct bt_gatt_pool *gp,
			   struct _bt_gatt_ccc *ccc,
			   uint8_t perm)
{
	struct bt_gatt_attr *attr;

	if (!gp || !gp->svc.attrs || !ccc || !perm) {
		LOG_ERR("Invalid attribute");
		return -EINVAL;
	}
	if (gp->svc.attr_count >= gp->attr_array_size) {
		LOG_ERR("No space left on given svc");
		return -ENOSPC;
	}

	attr = &gp->svc.attrs[gp->svc.attr_count++];
	*attr = (struct bt_gatt_attr)BT_GATT_CCC_MANAGED(ccc, perm);
	attr->uuid = uuid_ccc;

	return 0;
}


void bt_gatt_pool_free(struct bt_gatt_pool *gp)
{
	if (!gp) {
		LOG_ERR("Gatt pool is NULL");
		return;
	}
	if (!gp->svc.attrs) {
		LOG_ERR("Gatt pool attributes is NULL");
		return;
	}

	for (size_t n = 0; n < gp->svc.attr_count; ++n) {
		bt_gatt_pool_attr_free(&gp->svc.attrs[n]);
	}
	gp->svc.attr_count = 0;
}


#if CONFIG_BT_GATT_POOL_STATS != 0
static size_t mask_print(atomic_t *mask, size_t mask_size)
{
	size_t used_el_cnt = 0;

	for (size_t i = 0; i < mask_size; i++) {
		uint32_t state_part = mask[mask_size - i - 1];

		printk("%08X", state_part);
		used_el_cnt += popcount(state_part);
	}
	return used_el_cnt;
}

void bt_gatt_pool_stats_print(void)
{
	size_t used_el_cnt;

#if CONFIG_BT_GATT_UUID16_POOL_SIZE != 0
	printk("UUID 16 Pool. Locked elements mask:\n");

	used_el_cnt = mask_print(BT_UUID_16_LOCKS,
				 ARRAY_SIZE(BT_UUID_16_LOCKS));

	printk("\nPool element usage: %d out of %d\n\n", used_el_cnt,
	       CONFIG_BT_GATT_UUID16_POOL_SIZE);
#endif

#if CONFIG_BT_GATT_UUID32_POOL_SIZE != 0
	printk("UUID 32 Pool. Locked elements mask:\n");

	used_el_cnt = mask_print(BT_UUID_32_LOCKS,
				 ARRAY_SIZE(BT_UUID_32_LOCKS));

	printk("\nPool element usage: %d out of %d\n\n", used_el_cnt,
	       CONFIG_BT_GATT_UUID32_POOL_SIZE);
#endif

#if CONFIG_BT_GATT_UUID128_POOL_SIZE != 0
	printk("UUID 128 Pool. Locked elements mask:\n");

	used_el_cnt = mask_print(BT_UUID_128_LOCKS,
				 ARRAY_SIZE(BT_UUID_128_LOCKS));

	printk("\nPool element usage: %d out of %d\n\n", used_el_cnt,
	       CONFIG_BT_GATT_UUID128_POOL_SIZE);
#endif

#if CONFIG_BT_GATT_CHRC_POOL_SIZE != 0
	printk("Characteristic Pool. Locked elements mask:\n");

	used_el_cnt = mask_print(BT_GATT_CHRC_LOCKS,
				 ARRAY_SIZE(BT_GATT_CHRC_LOCKS));

	printk("\nPool element usage: %d out of %d\n\n", used_el_cnt,
	       CONFIG_BT_GATT_CHRC_POOL_SIZE);
#endif
}
#endif /* CONFIG_BT_GATT_POOL_STATS */
