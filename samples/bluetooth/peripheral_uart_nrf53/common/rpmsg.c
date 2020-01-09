/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This library enables user to use open-amp library to enable communication
 * between master and remote core of the processor.
 * To use:
 * 1. Register RX callback by calling ipc_register_rx_callback()
 * 2. Initialize ipc, by calling ipc_init()
 *
 * When those steps are completed, application is ready to exchange data
 * between two cores.
 */
#include <zephyr.h>
#include <errno.h>
#include <drivers/ipm.h>

#include <openamp/open_amp.h>
#include <metal/sys.h>
#include <metal/device.h>
#include <metal/alloc.h>

#include "rpmsg.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(rpmsg);

/**@brief RPC structure
 *
 * Contains all used stuff to ensure stable and realiable communication
 */
struct rpc_ctx {
	/* Pointer to RX callback */
	rx_callback rx_callback;

	/* RX data semaphore */
	struct k_sem data_rx_sem;

	/* Endpoint semaphore */
	struct k_sem ept_sem;
};

/* Stack size of stack area used by each thread */
#define STACKSIZE 1024

/* Scheduling priority used by each thread */
#define PRIORITY 7

/* Shared memory configuration */
#define SHM_START_ADDR      (DT_IPC_SHM_BASE_ADDRESS + 0x400)
#define SHM_SIZE            0x7c00
#define SHM_DEVICE_NAME     "sram0.shm"

#define VRING_COUNT         2
#define VRING_TX_ADDRESS    (SHM_START_ADDR + SHM_SIZE - 0x400)
#define VRING_RX_ADDRESS    (VRING_TX_ADDRESS - 0x400)
#define VRING_ALIGNMENT     4
#define VRING_SIZE          16

#define VDEV_STATUS_ADDR    DT_IPC_SHM_BASE_ADDRESS

/* Handlers for TX and RX channels */
/* TX handler */
static struct device *ipm_tx_handle;

/* RX handler */
static struct device *ipm_rx_handle;

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
static struct rpmsg_endpoint ep;

/* Thread properties */
static K_THREAD_STACK_DEFINE(rx_thread_stack, STACKSIZE);
static struct k_thread rx_thread_data;

/* Main context structure */
static struct rpc_ctx rpc;

static unsigned char virtio_get_status(struct virtio_device *vdev)
{
	return IS_ENABLED(CONFIG_RPMSG_MASTER) ?
		VIRTIO_CONFIG_STATUS_DRIVER_OK : sys_read8(VDEV_STATUS_ADDR);
}

static u32_t virtio_get_features(struct virtio_device *vdev)
{
	return BIT(VIRTIO_RPMSG_F_NS);
}

void virtio_set_status(struct virtio_device *vdev, unsigned char status)
{
	sys_write8(status, VDEV_STATUS_ADDR);
}

#if defined(CONFIG_RPMSG_MASTER)
static void virtio_set_features(struct virtio_device *vdev, u32_t features)
{
	/* No need for implementation */
}
#endif

static void virtio_notify(struct virtqueue *vq)
{
	int status;

	status = ipm_send(ipm_tx_handle, 0, 0, NULL, 0);
	if (status != 0) {
		LOG_WRN("Failed to notify: %d", status);
	}
}

const struct virtio_dispatch dispatch = {
	.get_status = virtio_get_status,
	.get_features = virtio_get_features,
	.set_status = virtio_set_status,
#if defined(CONFIG_RPMSG_MASTER)
	.set_features = virtio_set_features,
#endif
	.notify = virtio_notify,
};

/* Callback launch right after some data has arrived. */
static void ipm_callback(void *context, u32_t id, volatile void *data)
{
	k_sem_give(&rpc.data_rx_sem);
}

/* Callback launch right after virtqueue_notification from rx_thread. */
static int endpoint_cb(struct rpmsg_endpoint *ept, void *data, size_t len,
		       u32_t src, void *priv)
{
	/* We have just received data package. */
	if (rpc.rx_callback) {
		rpc.rx_callback(data, len);
	}

	return RPMSG_SUCCESS;
}

static void rpmsg_service_unbind(struct rpmsg_endpoint *ep)
{
	rpmsg_destroy_ept(ep);
}

static void ns_bind_cb(struct rpmsg_device *rdev,
		       const char *name, u32_t dest)
{
	(void)rpmsg_create_ept(&ep, rdev, name, RPMSG_ADDR_ANY,
			       dest, endpoint_cb, rpmsg_service_unbind);

	k_sem_give(&rpc.ept_sem);
}

/**@brief Hadles rx notifications.
 *
 * rx_thread is responsible for notification the rpmsg library to launch
 * endpoint_cb
 *
 * @param[in] p1 Not used.
 * @param[in] p2 Not used.
 * @param[in] p3 Not used.
 */
static void rx_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (1) {
		int status = k_sem_take(&rpc.data_rx_sem, K_FOREVER);

		if (status == 0) {
			virtqueue_notification(IS_ENABLED(CONFIG_RPMSG_MASTER) ?
					       vq[0] : vq[1]);
		}
	}
}

int ipc_transmit(const u8_t *buff, size_t buff_len)
{
	int ret = 0;

	ret = rpmsg_send(&ep, buff, buff_len);

	if (ret > 0) {
		ret = 0;
	}

	return ret;
}

int ipc_init(void)
{
	int err;
	struct metal_init_params metal_params = METAL_INIT_DEFAULTS;

	static struct virtio_vring_info rvrings[2];
	static struct virtio_device vdev;
	static struct metal_io_region *shm_io;
	static struct metal_device *device;
	static struct rpmsg_virtio_shm_pool shpool;

	/* Init semaphores. */
	k_sem_init(&rpc.data_rx_sem, 0, 1);
	k_sem_init(&rpc.ept_sem, 0, 1);

	k_thread_create(&rx_thread_data, rx_thread_stack,
			K_THREAD_STACK_SIZEOF(rx_thread_stack),
			rx_thread, NULL, NULL, NULL,
			K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);

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
		return -ENODEV;
	}

	/* IPM setup. */
	ipm_tx_handle = device_get_binding(IS_ENABLED(CONFIG_RPMSG_MASTER) ?
					   "IPM_1" : "IPM_0");
	if (!ipm_tx_handle) {
		LOG_ERR("Could not get TX IPM device handle");
		return -ENODEV;
	}

	ipm_rx_handle = device_get_binding(IS_ENABLED(CONFIG_RPMSG_MASTER) ?
					   "IPM_0" : "IPM_1");
	if (!ipm_rx_handle) {
		LOG_ERR("Could not get RX IPM device handle");
		return -ENODEV;
	}

	ipm_register_callback(ipm_rx_handle, ipm_callback, NULL);

	/* Virtqueue setup. */
	vq[0] = virtqueue_allocate(VRING_SIZE);
	if (!vq[0]) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[0]");
		return -ENOMEM;
	}

	vq[1] = virtqueue_allocate(VRING_SIZE);
	if (!vq[1]) {
		LOG_ERR("virtqueue_allocate failed to alloc vq[1]");
		return -ENOMEM;
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

		err = rpmsg_init_vdev(&rvdev, &vdev, ns_bind_cb, shm_io,
				      &shpool);
		if (err) {
			LOG_ERR("RPMSG vdev initialization failed %d", err);
			return err;
		}

		/* Wait til nameservice ep is setup */
		k_sem_take(&rpc.ept_sem, K_FOREVER);

		LOG_INF("Endpoint set successfully");

	} else {

		/* Setup rvdev. */
		err = rpmsg_init_vdev(&rvdev, &vdev, NULL, shm_io, NULL);
		if (err != 0) {
			LOG_ERR("RPMSH vdev initialization failed %d", err);
		}

		/* Get RPMsg device from RPMsg VirtIO device. */
		rdev = rpmsg_virtio_get_rpmsg_device(&rvdev);

		err = rpmsg_create_ept(&ep, rdev, "bt_ser", RPMSG_ADDR_ANY,
				       RPMSG_ADDR_ANY,
				       endpoint_cb, rpmsg_service_unbind);
		if (err != 0) {
			LOG_ERR("RPMSH endpoint cretate failed %d", err);
		}
	}

	return 0;
}

void ipc_deinit(void)
{
	rpmsg_deinit_vdev(&rvdev);
	metal_finish();
	LOG_INF("IPC deinitialised");
}

void ipc_register_rx_callback(rx_callback cb)
{
	rpc.rx_callback = cb;
}
