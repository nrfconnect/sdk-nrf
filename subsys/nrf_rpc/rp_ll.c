/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 *
 */
#include <zephyr.h>
#include <errno.h>
#include <drivers/ipm.h>

#include <openamp/open_amp.h>
#include <metal/sys.h>
#include <metal/device.h>
#include <metal/alloc.h>

#include <nrf_errno.h>

#include "rp_ll.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(rp_ll_api);

static struct k_work_q my_work_q;
static struct k_work work_item;

/* Indicates that handshake was done by this endpoint */
#define EP_FLAG_HANSHAKE_DONE 1

/* Shared memory configuration */
#define SHM_NODE            DT_CHOSEN(zephyr_ipc_shm)
#define SHM_START_ADDR      (DT_REG_ADDR(SHM_NODE) + 0x400)
#define SHM_SIZE            (DT_REG_SIZE(SHM_NODE) - 0x400)
#define SHM_DEVICE_NAME     "sram0.shm"

#define VRING_COUNT         2
#define VRING_TX_ADDRESS    (SHM_START_ADDR + SHM_SIZE - 0x400)
#define VRING_RX_ADDRESS    (VRING_TX_ADDRESS - 0x400)
#define VRING_ALIGNMENT     4
#define VRING_SIZE          16

#define VDEV_STATUS_ADDR    DT_REG_ADDR(SHM_NODE)

BUILD_ASSERT(VRING_TX_ADDRESS >= SHM_START_ADDR);
BUILD_ASSERT(VRING_RX_ADDRESS >= SHM_START_ADDR);

/* Handlers for TX and RX channels */
/* TX handler */
static const struct device *ipm_tx_handle;

/* RX handler */
static const struct device *ipm_rx_handle;

static struct rpmsg_virtio_device rvdev;
struct rpmsg_device *rdev;

static metal_phys_addr_t shm_physmap[] = { SHM_START_ADDR };
static struct metal_device shm_device = {
	.name = SHM_DEVICE_NAME,
	.bus = NULL,
	.num_regions = 1,
	.regions = {
		{
			.virt = (void *)SHM_START_ADDR,
			.physmap = shm_physmap,
			.size = SHM_SIZE,
			.page_shift = 0xffffffff,
			.page_mask = 0xffffffff,
			.mem_flags = 0,
			.ops = { NULL },
		},
	},
	.node = { NULL },
	.irq_num = 0,
	.irq_info = NULL
};

static struct virtqueue *vq[2];

static struct virtio_vring_info rvrings[2];
static struct virtio_device vdev;
static struct rpmsg_virtio_shm_pool shpool;

/* Thread properties */
static K_THREAD_STACK_DEFINE(rx_thread_stack,
	CONFIG_NRF_RPC_TR_PRMSG_RX_STACK_SIZE);

static unsigned char virtio_get_status(struct virtio_device *vdev)
{
	return IS_ENABLED(CONFIG_RPMSG_MASTER) ?
		VIRTIO_CONFIG_STATUS_DRIVER_OK : sys_read8(VDEV_STATUS_ADDR);
}

static uint32_t virtio_get_features(struct virtio_device *vdev)
{
	return 0;
}

void virtio_set_status(struct virtio_device *vdev, unsigned char status)
{
	sys_write8(status, VDEV_STATUS_ADDR);
}

static void virtio_set_features(struct virtio_device *vdev, uint32_t features)
{
	/* No need for implementation */
}

static void virtio_notify(struct virtqueue *vq)
{
	int status;

	status = ipm_send(ipm_tx_handle, 0, 0, NULL, 0);
	if (status != 0) {
		LOG_ERR("Failed to notify: %d", status);
	}
}

const struct virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.get_features = virtio_get_features,
	.set_status = virtio_set_status,
	.set_features = virtio_set_features,
	.notify = virtio_notify,
};

/* Callback launch right after some data has arrived. */
static void ipm_callback(const struct device *ipmdev, void *user_data, uint32_t id,
			 volatile void *data)
{
	k_work_submit_to_queue(&my_work_q, &work_item);
}

/* Callback launch right after virtqueue_notification from rx_thread. */
static int endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
		       uint32_t src, void *priv)
{
	struct rp_ll_endpoint *my_ep = metal_container_of(ept,
		struct rp_ll_endpoint, rpmsg_ep);

	if (len == 0) {
		if (!(my_ep->flags & EP_FLAG_HANSHAKE_DONE)) {
			rpmsg_send(ept, (uint8_t *)"", 0);
			my_ep->flags |= EP_FLAG_HANSHAKE_DONE;
			LOG_INF("Handshake done");
			my_ep->callback(my_ep, RP_LL_EVENT_CONNECTED, NULL, 0);
		}
		return RPMSG_SUCCESS;
	}

	my_ep->callback(my_ep, RP_LL_EVENT_DATA, data, len);

	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ep)
{
	rpmsg_destroy_ept(ep);
}

void work_callback(struct k_work *item)
{
	ARG_UNUSED(item);
	virtqueue_notification(IS_ENABLED(CONFIG_RPMSG_MASTER) ? vq[0] : vq[1]);
}

int rp_ll_send(struct rp_ll_endpoint *endpoint, const uint8_t *buf,
	size_t buf_len)
{
	int ret;

	ret = rpmsg_send(&endpoint->rpmsg_ep, buf, buf_len);
	if (ret > 0) {
		ret = 0;
	}
	return ret;
}

int rp_ll_init(void)
{
	int err;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;
	struct metal_io_region *shm_io;
	struct metal_device *device;
	struct rpmsg_virtio_shm_pool *pshpool = NULL;

	/* Libmetal setup. */
	err = metal_init(&metal_params);
	if (err) {
		LOG_ERR("metal_init: failed - error code %d", err);
		return err;
	}

	err = metal_register_generic_device(&shm_device);
	if (err) {
		LOG_ERR("Couldn't register shared memory device: %d", err);
		return err;
	}

	err = metal_device_open("generic", SHM_DEVICE_NAME, &device);
	if (err) {
		LOG_ERR("metal_device_open failed: %d", err);
		return err;
	}

	shm_io = metal_device_io_region(device, 0);
	if (!shm_io) {
		LOG_ERR("metal_device_io_region failed to get region");
		return -NRF_ENODEV;
	}

	/* IPM setup. */
	ipm_tx_handle = device_get_binding(IS_ENABLED(CONFIG_RPMSG_MASTER) ?
					   "IPM_1" : "IPM_0");
	if (!ipm_tx_handle) {
		LOG_ERR("Could not get TX IPM device handle");
		return -NRF_ENODEV;
	}

	ipm_rx_handle = device_get_binding(IS_ENABLED(CONFIG_RPMSG_MASTER) ?
					   "IPM_0" : "IPM_1");
	if (!ipm_rx_handle) {
		LOG_ERR("Could not get RX IPM device handle");
		return -NRF_ENODEV;
	}

	ipm_register_callback(ipm_rx_handle, ipm_callback, NULL);

	/* Virtqueue setup. */
	vq[0] = virtqueue_allocate(VRING_SIZE);
	if (!vq[0]) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[0]");
		return -NRF_ENOMEM;
	}

	vq[1] = virtqueue_allocate(VRING_SIZE);
	if (!vq[1]) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[1]");
		return -NRF_ENOMEM;
	}

	rvrings[0].io = shm_io;
	rvrings[0].info.vaddr = (void *)VRING_TX_ADDRESS;
	rvrings[0].info.num_descs = VRING_SIZE;
	rvrings[0].info.align = VRING_ALIGNMENT;
	rvrings[0].vq = vq[0];

	rvrings[1].io = shm_io;
	rvrings[1].info.vaddr = (void *)VRING_RX_ADDRESS;
	rvrings[1].info.num_descs = VRING_SIZE;
	rvrings[1].info.align = VRING_ALIGNMENT;
	rvrings[1].vq = vq[1];

	vdev.role = IS_ENABLED(CONFIG_RPMSG_MASTER) ?
		    RPMSG_MASTER : RPMSG_REMOTE;

	vdev.vrings_num = VRING_COUNT;
	vdev.func = &dispatch;
	vdev.vrings_info = &rvrings[0];

	if (IS_ENABLED(CONFIG_RPMSG_MASTER)) {
		/* This step is only required if you are VirtIO device master.
		 * Initialize the shared buffers pool.
		 */
		rpmsg_virtio_init_shm_pool(&shpool, (void *)SHM_START_ADDR,
					   SHM_SIZE);
		pshpool = &shpool;

	}

	/* Setup rvdev. */
	err = rpmsg_init_vdev(&rvdev, &vdev, NULL, shm_io, pshpool);
	if (err != 0) {
		LOG_ERR("RPMSH vdev initialization failed %d", err);
	}

	/* Get RPMsg device from RPMsg VirtIO device. */
	rdev = rpmsg_virtio_get_rpmsg_device(&rvdev);

	k_work_q_start(&my_work_q, rx_thread_stack,
		K_THREAD_STACK_SIZEOF(rx_thread_stack),
		CONFIG_NRF_RPC_TR_PRMSG_RX_PRIORITY);
	k_work_init(&work_item, work_callback);

	LOG_DBG("initializing %s: SUCCESS", __func__);

	return 0;

}

int rp_ll_endpoint_init(struct rp_ll_endpoint *endpoint,
	int endpoint_number, rp_ll_event_handler callback, void *user_data)
{
	int err;

	endpoint->callback = callback;

	err = rpmsg_create_ept(&endpoint->rpmsg_ep, rdev, "", endpoint_number,
		endpoint_number, endpoint_cb, rpmsg_service_unbind);

	if (err != 0) {
		LOG_ERR("RPMSH endpoint cretate failed %d", err);
		return err;
	}

	rpmsg_send(&endpoint->rpmsg_ep, (uint8_t *)"", 0);

	return 0;
}

void rp_ll_endpoint_uninit(struct rp_ll_endpoint *endpoint)
{
	rpmsg_destroy_ept(&endpoint->rpmsg_ep);
}

void rp_ll_uninit(void)
{
	rpmsg_deinit_vdev(&rvdev);
	metal_finish();
	LOG_INF("IPC uninitialized");
}
