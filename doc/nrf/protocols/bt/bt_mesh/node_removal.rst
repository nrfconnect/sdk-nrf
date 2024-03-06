.. _ug_bt_mesh_node_removal:

Removing a node from a mesh network
###################################

.. contents::
   :local:
   :depth: 2

In certain circumstances, a need may arise to remove a node from a Bluetooth® Mesh network.
This can be done by unprovisioning the node using the Config Node Reset message.

Though the unprovisioning of the node is a straightforward procedure, some additional steps need to be taken to maintain the security of the network.
The procedure consists of two steps:

1. Removing the node.
2. Replacing the network key(s), used by the removed node, in the rest of the nodes in the network.

Removing the node
*****************

To remove the node from the network and reset its configuration, send the Config Node Reset message from a node running the Configuration Client model.
The node running the Configuration Client model is referred to as the Mesh Network Manager.

To remove the node using the |NCS| API, complete the following steps:

1. Call the :c:func:`bt_mesh_cfg_cli_node_reset` function to send a Config Node Reset message to the node to be reset.
#. Find the node in the Configuration Database (CDB) using the :c:func:`bt_mesh_cdb_node_get` function.
#. Remove the node from the CDB using the :c:func:`bt_mesh_cdb_node_del` function.

When using the nRF Mesh mobile app as a provisioner, do the following:

1. On the Network screen, tap the mesh device node to be reset.
#. Scroll down and find the :guilabel:`Reset Node` section.
#. Press the :guilabel:`RESET` button.

Updating the network key
************************

Even if the Config Node Reset message has been sent to the node, and the Config Node Reset Status message was received, this doesn't guarantee that the node has removed all configuration data.
To ensure that the removed node is not able to decrypt network messages after being reset, update the network key(s) that were known to the removed node.

This procedure can be used in any other cases when a network key needs to be updated.
For example, in cases where there is a suspicion of network credentials being compromised.

The same procedure can be used to update an application key.
To update an application key, the associated network key update must also be started before starting the updating for the application key.

.. note::
   Updating keys is currently not supported by the nRF Mesh mobile app.

The procedure of updating a network (or application) key is described below.

Uploading a new key
===================

Upload a new network key to the nodes by sending the Config NetKey Update message to every node, including the node with the Configuration Client model.
If an application key update is needed, upload a new application key by sending the Config AppKey Update message to every node after uploading the new network key.

The snippet below demonstrates how to do this using the |NCS| API:

.. code-block:: c

    static uint8_t send_net_key_update(struct bt_mesh_cdb_node *node, void *user_data)
    {
        struct bt_mesh_cdb_subnet *subnet = user_data;
        uint8_t status = 0;
        int err;

        err = bt_mesh_cfg_cli_net_key_update(node->net_idx, node->addr, subnet->net_idx,
                                             subnet->keys[1].net_key, &status);
        if (err || status) {
            printk("Unable to update NetKey on %#x (err %d, status %u)\n", node->addr, err,
                   status);
        } else {
            printk("Updated NetKey with index %u on %#x\n", subnet->net_idx, node->addr);
        }

        return BT_MESH_CDB_ITER_CONTINUE;
    }

    static int update_net_key(uint16_t net_idx, uint8_t *new_net_key)
    {
        struct bt_mesh_cdb_subnet *subnet;

        subnet = bt_mesh_cdb_subnet_get(net_idx);
        if (!subnet) {
            printk("Unable to get subnet\n");
            return -EINVAL;
        }

        /* Store the new network key in CDB. */
        memcpy(subnet->keys[1].net_key, new_net_key, 16);
        bt_mesh_cdb_subnet_store(subnet);

        /* Send the new network key to each node. */
        bt_mesh_cdb_node_foreach(send_net_key_update, subnet);
        return 0;
    }

    ...

    uint8_t new_net_key[16] = { ... };
    update_net_key(BT_MESH_NET_PRIMARY, new_net_key);

Switching to the new key
========================

After uploading the new key, configure the nodes to use it when sending messages.
To do that, change the Key Refresh Phase to 2 by sending the Config Key Refresh Phase Set message with Transition field set to 0x2.
Send this message to at least one node.

The rest of the nodes will switch the Key Refresh Phase through Secure Network Beacon.
But in this case, it may take a while for all nodes to eventually switch the phase due to the following:

* The time between two consecutive Secure Network Beacons is approximately 10 seconds.
* Some nodes can have a backoff interval of up to 600 seconds when sending Secure Network Beacons.

You can speed up this process by sending the Config Key Refresh Phase Set message to all nodes that need to be updated.

After switching the Key Refresh Phase to 2, a node will decrypt messages using both old and new keys, but encrypt only using the new key.

The snippet below demonstrates how to set the Key Refresh Phase on all nodes using the |NCS| API:

.. code-block:: c

    static uint8_t send_key_refresh_phase_set(struct bt_mesh_cdb_node *node, void *user_data)
    {
        struct bt_mesh_cdb_subnet *subnet = user_data;
        uint8_t phase;
        uint8_t status;
        int err;

        err = bt_mesh_cfg_cli_krp_set(node->net_idx, node->addr, subnet->net_idx, subnet->kr_phase,
                                      &status, &phase);
        if (err || status) {
            printk("Unable to set Key Refresh Phase on %#x (err %d, status %u)\n",
                   node->addr, err, status);
        } else {
            printk("Set Key Refresh Phase to %u on %#x\n", phase, node->addr);
        }

        return BT_MESH_CDB_ITER_CONTINUE;
    }

    static int update_key_refresh_phase(uint8_t net_idx, uint8_t phase)
    {
        struct bt_mesh_cdb_subnet *subnet;

        subnet = bt_mesh_cdb_subnet_get(net_idx);
        if (!subnet) {
            printk("Unable to get subnet\n");
            return -EINVAL;
        }

        subnet->kr_phase = phase;
        bt_mesh_cdb_subnet_store(subnet);

        bt_mesh_cdb_node_foreach(send_key_refresh_phase_set, subnet);
        return 0;
    }

    ...

    update_key_refresh_phase(BT_MESH_NET_PRIMARY, BT_MESH_KR_PHASE_2);

If the Key Refresh Phase is to be changed through Secure Network Beacons, wait until all nodes have changed the Key Refresh Phase to 2.
This can be done by sending the Config Key Refresh Phase Get message to a specific node.
To retrieve the Key Refresh Phase from a node using the |NCS| API, use the :c:func:`bt_mesh_cfg_cli_krp_get` function.

Revoking the old key
====================

When all nodes are in the Key Refresh Phase 2, the old key needs to be removed.
To do that, switch the Key Refresh Phase to 3 by sending the Config Key Refresh Phase Set message with the Transition field set to 0x3.

The same logic as for the phase 2 applies here.
Either send this message to one of the nodes (not necessarily the node with the Configuration Client model) and wait while other nodes receive Secure Network Beacon, or send the message to each node.

The snippet below demonstrates how to send the message to each node using the |NCS| API:

.. code-block:: c

    static int swap_net_keys_in_cdb(uint8_t net_idx)
    {
        struct bt_mesh_cdb_subnet *subnet;

        subnet = bt_mesh_cdb_subnet_get(net_idx);
        if (!subnet) {
            printk("Unable to get subnet\n");
            return -EINVAL;
        }

        memcpy(subnet->keys[0].net_key, subnet->keys[1].net_key, 16);
        memset(subnet->keys[1].net_key, 0, 16);
        bt_mesh_cdb_subnet_store(subnet);
    }

    ...

    update_key_refresh_phase(BT_MESH_NET_PRIMARY, BT_MESH_KR_PHASE_3);

    /* Replace the old key with the new one in CDB. */
    swap_net_keys_in_cdb(BT_MESH_NET_PRIMARY);

Once all nodes have switched the Key Refresh Phase to 3, the procedure completes.
