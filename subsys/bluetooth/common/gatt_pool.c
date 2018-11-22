/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/common/gatt_pool.h>

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

#if CONFIG_BT_GATT_CCC_POOL_SIZE != 0
static struct _bt_gatt_ccc ccc_tab[CONFIG_BT_GATT_CCC_POOL_SIZE];
static ATOMIC_DEFINE(ccc_locks, ARRAY_SIZE(ccc_tab));
#define BT_GATT_CCC_TAB ccc_tab
#define BT_GATT_CCC_LOCKS ccc_locks
#else
#define BT_GATT_CCC_TAB NULL
#define BT_GATT_CCC_LOCKS NULL
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
static struct svc_el_pool ccc_pool = {
	.elements = BT_GATT_CCC_TAB,
	.locks = BT_GATT_CCC_LOCKS,
};

#define EL_IN_POOL_VERIFY(pool, el)                                            \
	do {                                                                   \
		__ASSERT(pool != NULL, "Pool is uninitialized");               \
		__ASSERT(((u32_t)el >= (u32_t)pool) &&                         \
			     (((u32_t)el) < (((u32_t)pool) + sizeof(pool))),   \
			 "Element does not belong to the pool");               \
	} while ((0))

#define ADDR_2_INDEX(pool, el)                                                 \
	((((u32_t)el) - ((u32_t)pool)) / (sizeof(pool[0])))

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

static void uuid_16_get(struct bt_uuid **uuid, struct svc_el_pool *uuid_pool)
{
	size_t ind = free_element_find(uuid_pool,
				       CONFIG_BT_GATT_UUID16_POOL_SIZE);

	__ASSERT(ind < CONFIG_BT_GATT_UUID16_POOL_SIZE,
		 "No more UUID16s in the pool!");

	*uuid = (struct bt_uuid *)
		&((struct bt_uuid_16 *) uuid_pool->elements)[ind];
}

static void uuid_32_get(struct bt_uuid **uuid, struct svc_el_pool *uuid_pool)
{
	size_t ind = free_element_find(uuid_pool,
				       CONFIG_BT_GATT_UUID32_POOL_SIZE);

	__ASSERT(ind < CONFIG_BT_GATT_UUID32_POOL_SIZE,
		 "No more UUID32s in the pool!");

	*uuid = (struct bt_uuid *)
		&((struct bt_uuid_32 *) uuid_pool->elements)[ind];
}

static void uuid_128_get(struct bt_uuid **uuid, struct svc_el_pool *uuid_pool)
{
	size_t ind = free_element_find(uuid_pool,
				       CONFIG_BT_GATT_UUID128_POOL_SIZE);

	__ASSERT(ind < CONFIG_BT_GATT_UUID128_POOL_SIZE,
		 "No more UUID128s in the pool!");

	*uuid = (struct bt_uuid *)
		&((struct bt_uuid_128 *) uuid_pool->elements)[ind];
}

static void chrc_get(struct bt_gatt_chrc **chrc)
{
	size_t ind = free_element_find(&chrc_pool,
				       CONFIG_BT_GATT_CHRC_POOL_SIZE);

	__ASSERT(ind < CONFIG_BT_GATT_CHRC_POOL_SIZE,
		 "No more chrc descriptors in the pool!");

	*chrc = &((struct bt_gatt_chrc *) chrc_pool.elements)[ind];
}

static void chrc_release(struct bt_gatt_chrc const *chrc)
{
	EL_IN_POOL_VERIFY(BT_GATT_CHRC_TAB, chrc);
	atomic_clear_bit(chrc_pool.locks,
			 ADDR_2_INDEX(BT_GATT_CHRC_TAB, chrc));
}

static void ccc_get(struct _bt_gatt_ccc **ccc)
{
	size_t ind = free_element_find(&ccc_pool, CONFIG_BT_GATT_CCC_POOL_SIZE);

	__ASSERT(ind < CONFIG_BT_GATT_CCC_POOL_SIZE,
		 "No more chrc descriptors in the pool!");

	*ccc = &((struct _bt_gatt_ccc *) ccc_pool.elements)[ind];
}

static void ccc_release(struct _bt_gatt_ccc const *ccc)
{
	EL_IN_POOL_VERIFY(BT_GATT_CCC_TAB, ccc);
	atomic_clear_bit(ccc_pool.locks,
			 ADDR_2_INDEX(BT_GATT_CCC_TAB, ccc));
}

static void uuid_register(struct bt_uuid **dest_uuid,
			  struct bt_uuid const *src_uuid)
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

void bt_gatt_pool_svc_get(struct bt_gatt_service *svc,
			  struct bt_uuid const *svc_uuid)
{
	struct bt_gatt_attr *attr = &svc->attrs[svc->attr_count];
	struct bt_uuid      *uuid_gatt_primary = BT_UUID_GATT_PRIMARY;

	__ASSERT_NO_MSG(svc->attrs != NULL);

	memset(attr, 0, sizeof(*attr));

	uuid_register((struct bt_uuid **) &attr->uuid, uuid_gatt_primary);
	uuid_register((struct bt_uuid **) &attr->user_data, svc_uuid);
	attr->perm = BT_GATT_PERM_READ;
	attr->read = bt_gatt_attr_read_service;

	svc->attr_count++;
}

void bt_gatt_pool_svc_put(struct bt_gatt_attr const *attr)
{
	__ASSERT_NO_MSG(attr != NULL);

	uuid_unregister(attr->uuid);
	uuid_unregister(attr->user_data);
}

void bt_gatt_pool_chrc_get(struct bt_gatt_service *svc,
			   struct bt_gatt_chrc const *chrc)
{
	struct bt_gatt_attr *attr = &svc->attrs[svc->attr_count];
	struct bt_uuid      *uuid_gatt_chrc = BT_UUID_GATT_CHRC;
	struct bt_gatt_chrc *dest_chrc;

	__ASSERT_NO_MSG(svc->attrs != NULL);

	memset(attr, 0, sizeof(*attr));

	uuid_register((struct bt_uuid **) &attr->uuid, uuid_gatt_chrc);
	attr->perm = BT_GATT_PERM_READ;
	attr->read = bt_gatt_attr_read_chrc;

	/* Register user data for characteristic. */
	chrc_get((struct bt_gatt_chrc **) &attr->user_data);
	dest_chrc = (struct bt_gatt_chrc *) attr->user_data;
	memset(dest_chrc, 0, sizeof(*dest_chrc));
	dest_chrc->properties = chrc->properties;
	uuid_register((struct bt_uuid **) &dest_chrc->uuid, chrc->uuid);

	svc->attr_count++;
}

void bt_gatt_pool_chrc_put(struct bt_gatt_attr const *attr)
{
	__ASSERT_NO_MSG(attr != NULL);

	uuid_unregister(((struct bt_gatt_chrc *) attr->user_data)->uuid);
	chrc_release(attr->user_data);
	uuid_unregister(attr->uuid);
}

void bt_gatt_pool_desc_get(struct bt_gatt_service *svc,
			   struct bt_gatt_attr const *descriptor)
{
	struct bt_gatt_attr *attr = &svc->attrs[svc->attr_count];

	__ASSERT_NO_MSG(svc->attrs != NULL);

	memset(attr, 0, sizeof(*attr));

	memcpy(attr, descriptor, sizeof(*attr));
	attr->uuid = NULL;
	uuid_register((struct bt_uuid **) &attr->uuid, descriptor->uuid);

	svc->attr_count++;
}

void bt_gatt_pool_desc_put(struct bt_gatt_attr const *attr)
{
	__ASSERT_NO_MSG(attr != NULL);

	uuid_unregister(attr->uuid);
}

void bt_gatt_pool_ccc_get(struct bt_gatt_service *svc,
			  struct _bt_gatt_ccc const *ccc)
{
	struct bt_gatt_attr *attr = &svc->attrs[svc->attr_count];
	struct bt_uuid      *uuid_gatt_ccc = BT_UUID_GATT_CCC;

	__ASSERT_NO_MSG(svc->attrs != NULL);

	memset(attr, 0, sizeof(*attr));

	uuid_register((struct bt_uuid **) &attr->uuid, uuid_gatt_ccc);
	attr->perm = BT_GATT_PERM_READ | BT_GATT_PERM_WRITE;
	attr->read = bt_gatt_attr_read_ccc;
	attr->write = bt_gatt_attr_write_ccc;
	ccc_get((struct _bt_gatt_ccc **) &attr->user_data);
	memcpy(attr->user_data, ccc, sizeof(struct _bt_gatt_ccc));

	svc->attr_count++;
}

void bt_gatt_pool_ccc_put(struct bt_gatt_attr const *attr)
{
	__ASSERT_NO_MSG(attr != NULL);

	uuid_unregister(attr->uuid);
	ccc_release(attr->user_data);
}

#if CONFIG_BT_GATT_POOL_STATS != 0
static size_t mask_print(atomic_t *mask, size_t mask_size)
{
	size_t used_el_cnt = 0;

	for (size_t i = 0; i < mask_size; i++) {
		u32_t state_part = mask[mask_size - i - 1];

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

#if CONFIG_BT_GATT_CCC_POOL_SIZE != 0
	printk("CCC Pool. Locked elements mask:\n");

	used_el_cnt = mask_print(BT_GATT_CCC_LOCKS,
				 ARRAY_SIZE(BT_GATT_CCC_LOCKS));

	printk("\nPool element usage: %d out of %d\n\n", used_el_cnt,
	       CONFIG_BT_GATT_CCC_POOL_SIZE);
#endif
}
#endif
