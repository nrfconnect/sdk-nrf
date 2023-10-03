/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "dfu_dist.h"

static struct bt_mesh_blob_io_flash *blob_flash_stream;

static void slot_info_print(const struct bt_mesh_dfu_slot *slot, const uint8_t *idx)
{
	char fwid[2 * CONFIG_BT_MESH_DFU_FWID_MAXLEN + 1];
	char metadata[2 * CONFIG_BT_MESH_DFU_METADATA_MAXLEN + 1];
	size_t len;

	len = bin2hex(slot->fwid, slot->fwid_len, fwid, sizeof(fwid));
	fwid[len] = '\0';
	len = bin2hex(slot->metadata, slot->metadata_len, metadata,
		      sizeof(metadata));
	metadata[len] = '\0';

	if (idx != NULL) {
		printk("Slot %u:\n", *idx);
	} else {
		printk("Slot:\n");
	}
	printk("\tSize:     %u bytes\n", slot->size);
	printk("\tFWID:     %s\n", fwid);
	printk("\tMetadata: %s\n", metadata);
}

static int dfd_srv_recv(struct bt_mesh_dfd_srv *srv,
			const struct bt_mesh_dfu_slot *slot,
			const struct bt_mesh_blob_io **io)
{
	printk("Uploading new firmware image to the distributor.\n");
	slot_info_print(slot, NULL);

	*io = &blob_flash_stream->io;

	return 0;
}

static void dfd_srv_del(struct bt_mesh_dfd_srv *srv,
			const struct bt_mesh_dfu_slot *slot)
{
	printk("Deleting the firmware image from the distributor.\n");
	slot_info_print(slot, NULL);
}

static int dfd_srv_send(struct bt_mesh_dfd_srv *srv,
			const struct bt_mesh_dfu_slot *slot,
			const struct bt_mesh_blob_io **io)
{
	printk("Starting the firmware distribution.\n");
	slot_info_print(slot, NULL);

	*io = &blob_flash_stream->io;

	return 0;
}

static const char *dfd_phase_to_str(enum bt_mesh_dfd_phase phase)
{
	switch (phase) {
	case BT_MESH_DFD_PHASE_IDLE:
		return "Idle";
	case BT_MESH_DFD_PHASE_TRANSFER_ACTIVE:
		return "Transfer Active";
	case BT_MESH_DFD_PHASE_TRANSFER_SUCCESS:
		return "Transfer Success";
	case BT_MESH_DFD_PHASE_APPLYING_UPDATE:
		return "Applying Update";
	case BT_MESH_DFD_PHASE_COMPLETED:
		return "Completed";
	case BT_MESH_DFD_PHASE_FAILED:
		return "Failed";
	case BT_MESH_DFD_PHASE_CANCELING_UPDATE:
		return "Canceling Update";
	case BT_MESH_DFD_PHASE_TRANSFER_SUSPENDED:
		return "Transfer Suspended";
	default:
		return "Unknown Phase";
	}
}

static void dfd_srv_phase(struct bt_mesh_dfd_srv *srv, enum bt_mesh_dfd_phase phase)
{
	printk("Distribution phase changed to %s\n", dfd_phase_to_str(phase));
}

static struct bt_mesh_dfd_srv_cb dfd_srv_cb = {
	.recv = dfd_srv_recv,
	.del = dfd_srv_del,
	.send = dfd_srv_send,
	.phase = dfd_srv_phase,
};

struct bt_mesh_dfd_srv dfd_srv = BT_MESH_DFD_SRV_INIT(&dfd_srv_cb);

void dfu_distributor_init(struct bt_mesh_blob_io_flash *flash_stream)
{
	blob_flash_stream = flash_stream;
}
