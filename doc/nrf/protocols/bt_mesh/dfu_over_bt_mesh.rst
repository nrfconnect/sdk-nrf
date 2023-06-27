.. _dfu_over_bt_mesh:

DFU over Bluetooth mesh
#######################

.. contents::
   :local:
   :depth: 2

The DFU specification is implemented in the Zephyr Bluetooth mesh DFU subsystem as three separate models:

* :ref:`zephyr:bluetooth_mesh_dfu_srv`
* :ref:`zephyr:bluetooth_mesh_dfu_cli`
* :ref:`zephyr:bluetooth_mesh_dfd_srv`

The specification that the Bluetooth mesh DFU subsystem is based on is not adopted yet, and therefore this feature should be used for experimental purposes only.
For more information about the Zephyr Bluetooth mesh DFU subsystem, see :ref:`zephyr:bluetooth_mesh_dfu`.

The Bluetooth mesh subsystem in |NCS| provides a set of samples that can be used for evaluation of the Bluetooth mesh DFU specification and subsystem:

* :ref:`ble_mesh_dfu_target` sample
* :ref:`ble_mesh_dfu_distributor` sample

To configure and control DFU on the Firmware Distribution Server, it is required to have the Firmware Distribution Client model.
The Bluetooth mesh DFU subsystem in Zephyr provides a set of shell commands that can be used to substitute the need for the client.
For the complete list of commands, see the :ref:`zephyr:bluetooth_mesh_shell_dfd_server` section of the Bluetooth mesh shell documentation.

The commands can be executed in two ways:

* Through the shell management subsystem of MCU manager (for example, using the nRF Connect Device Manager mobile application or :ref:`Mcumgr command-line tool <zephyr:mcumgr_cli>`).
* By accessing the :ref:`zephyr:shell_api` module over UART.

Provisioning and configuring the devices
****************************************

After programming the samples onto the boards, they need to be provisioned into the same Bluetooth mesh network with an external provisioner device.
See the documentation for :ref:`provisioning the mesh DFU target device <ble_mesh_dfu_target_provisioning>` and :ref:`provisioning the mesh DFU distributor device <ble_mesh_dfu_distributor_provisioning>` for how this is done.

After the provisioning is completed, a Configuration Client needs to add a common application key to all devices.
The added application key must be bound:

* On the Distributor: to the Firmware Distribution Server, Firmware Update Client, BLOB Transfer Server and BLOB Transfer Client models instantiated on the primary element, and to the Firmware Update Server and BLOB Transfer Server models instantiated on the secondary element of the device (see :ref:`Configuring models on the Distributor node <ble_mesh_dfu_distributor_model_config>`).
* On Target nodes: to the Firmware Update Server and BLOB Transfer Server models instantiated on the primary element of the device (see :ref:`Configuring models on the Target node <ble_mesh_dfu_target_model_config>`).

The bound application key will be used in the firmware distribution procedure.

Uploading the firmware
**********************

After configuring the models, a new image can be uploaded to the Distributor.
To upload the image, follow the instructions provided in the :ref:`ble_mesh_dfu_distributor_fw_image_upload` section of the :ref:`ble_mesh_dfu_distributor` sample documentation.

The uploaded image needs to be registered in the Bluetooth mesh DFU subsystem.
To achieve this, issue the ``mesh models dfu slot add`` shell command specifying size in bytes of the image that was uploaded to the Distributor.
Optionally, you can provide firmware ID, metadata and Unique Resource Identifier (URI) parameters that come with the image.

For example, to allocate a slot for the :ref:`ble_mesh_dfu_target` sample with image size of 241236 bytes, with firmware ID set to ``0200000000000000``, and metadata generated as described in :ref:`bluetooth_mesh_dfu_eval_md` section below, type the following command::

  mesh models dfu slot add 241236 0200000000000000 020000000100000094cf24017c26f3710100

When the slot is added, the shell will print the slot ID.
Take note of this ID as it will then be needed to start the DFU transfer::

  Adding slot (size: 241236)
  Slot added. ID: 0

.. note::
   To update any value in a slot, issue the ``mesh models dfu slot del`` command specifying the ID of the allocated slot, and then add the slot again.

Populating the Distributor's receivers list
*******************************************

Add Target nodes to the DFU transfer by issuing the ``mesh models dfd receivers-add`` shell command.
This shell command is specifying the element address of a Target node with the Firmware Update Server instance and the image index on the Target node that needs to be updated.
For example, for two Target nodes with addresses ``0x0004`` and ``0x0005`` respectively, and with image index 0, the command will look like this::

  mesh models dfd receivers-add 0x0004,0;0x0005,0

.. note::
   To remove all receivers from the list, issue the ``mesh models dfd receivers-delete-all`` command.

Initiating the distribution
***************************

To start the DFU transfer, issue the ``mesh models dfd start`` shell command.
This command requires two mandatory arguments: ``app_idx`` and ``slot_idx``:

* As ``app_idx``, use the application key index that is bound to the Firmware Distribution Server and other Firmware Update and BLOB Transfer models on the Distributor and Target nodes.
* As ``slot_idx``, use the ID of the slot allocated by the ``mesh models dfu slot add`` shell command on the previous step.

For example, to run the DFU transfer in unicast mode, with AppKey index 0 and slot ID 0, call::

  mesh models dfd start 0 0

By default, the Firmware Distribution Server will request the Firmware Update Servers to apply the image immediately after the DFU transfer.
To avoid applying the image immediately and only verify it, set the 4th argument to 0::

  mesh models dfd start 0 0 0 0

Firmware distribution
=====================

The transfer will take a couple of minutes, depending on the number of Target nodes and the network quality.
To check the transfer progress, call the ``mesh models dfd receivers-get`` shell command, for example::

  mesh models dfd receivers-get 0 2

The output may look like this::

  {
          "target_cnt": 1,
          "targets": {
                  "0": { "blob_addr": 0x0004, "phase": 2, "status": 0, "blob_status": 0, "progress": 50, "img_idx": 0 }
                  "1": { "blob_addr": 0x0005, "phase": 2, "status": 0, "blob_status": 0, "progress": 50, "img_idx": 0 }
          }
  }

To see the distribution status, phase and parameters of the DFU transfer, use the ``mesh models dfd get`` command.
When the DFU transfer successfully completes, the phase will be set to  :c:enum:`BT_MESH_DFD_PHASE_TRANSFER_SUCCESS`, for example::

  { "status": 0, "phase": 2, "group": 0x0000, "app_idx": 0, "ttl": 255, "timeout_base": 0, "xfer_mode": 1, "apply": 0, "slot_idx": 0 }

The :c:enum:`bt_mesh_dfd_phase` enumeration contains the complete list of distribution phases.

Suspending the distribution
===========================

The firmware distribution can be suspended using the ``mesh models dfd suspend`` shell command.
The distribution phase is switched to :c:enum:`BT_MESH_DFD_PHASE_TRANSFER_SUSPENDED` in this case.

To resume the DFU transfer, issue the ``mesh models dfu cli resume`` shell command.

Applying the firmware image
***************************

Depending on the update policy set at the start of the DFU transfer, the Firmware Distribution Server will do the following:

* If ``policy_apply`` is set to true or omitted when the DFU transfer starts, the Firmware Distribution Server will immediately apply the new firmware on the Target nodes upon the DFU transfer completion.
* If ``policy_apply`` is set to false, the image needs to be applied manually using the ``mesh models dfd apply`` command once the DFU transfer is completed.

When the Firmware Distribution Server starts applying the transferred image, the distribution phase is set to :c:enum:`BT_MESH_DFD_PHASE_APPLYING_UPDATE`.

After applying the new firmware, the Firmware Distribution Server will immediately request firmware ID of the currently running firmware on the Target nodes to confirm that the new firmware has been applied successfully.
Depending on the :c:enum:`bt_mesh_dfu_effect` value received from the Target nodes after the DFU transfer is started, the following cases are possible:

* If the image effect for a particular Target node is :c:enum:`BT_MESH_DFU_EFFECT_UNPROV`, the Firmware Distribution Server doesn't expect any reply from that Target node.
  If the Distributor doesn't receive any reply, it will repeat the request several times.
  If the Distributor eventually receives a reply, the DFU for this particular Target node is considered unsuccessful.
  Otherwise, the DFU is considered successful.
* In all other cases, the Distributor expects a reply from the Target node with the firmware ID equal to the firmware ID of the transferred image.
  If the Target node responds with a different firmware ID or doesn't respond at all after several requests, the DFU for this particular Target node is considered unsuccessful.
  Otherwise, the DFU is considered successful.

The DFU ends after the Distributor stops polling the Target nodes.
If the DFU completes successfully for at least one Target node, the firmware distribution is considered as successful.
In this case, the distribution phase is set to :c:enum:`BT_MESH_DFD_PHASE_COMPLETED`.
If the DFU doesn't complete successfully, the distribution phase is set to :c:enum:`BT_MESH_DFD_PHASE_FAILED`.

Cancelling the distribution
***************************

To cancel the firmware distribution, use the ``mesh models dfd cancel`` shell command.
The Firmware Distribution Server will start the cancelling procedure by sending a cancel message to all Targets and will switch phase to :c:enum:`BT_MESH_DFD_PHASE_CANCELING_UPDATE`.
Once the cancelling procedure is completed, the phase is set to :c:enum:`BT_MESH_DFD_PHASE_IDLE`.

.. note::
   It is possible to cancel the firmware distribution on a specific Target node at any time by sending Firmware Update Cancel message.
   To do this, use the ``mesh models dfu cli cancel`` shell command specifying unicast address of the Target node.

Recovering from failed distribution
***********************************

If the firmware distribution fails for any reason, the list of Target nodes should be cleared and the distribution phase should be set to :c:enum:`BT_MESH_DFD_PHASE_IDLE` before making a new attempt.
To do this, run the following shell commands::

  mesh models dfd receivers-delete-all
  mesh models dfd cancel

To bring a stalled Target node to idle state, use the ``mesh models dfu cli cancel`` shell command.

.. note::
   This does not affect the allocated image slots.

.. _bluetooth_mesh_dfu_eval_md:

Composing the firmware metadata
*******************************

The Bluetooth mesh DFU subsystem provides a set of shell commands that can be used to compose a firmware metadata.
The format of metadata is defined in the :c:struct:`bt_mesh_dfu_metadata` structure.
For the complete list of commands, see :ref:`zephyr:bluetooth_mesh_shell_dfu_metadata`.

To start composing metadata, issue the ``mesh models dfu metadata comp-add`` shell command that encodes a Composition Data header.
For example, for a Target node with product ID 0x0059, zero company and version IDs, 10 entries in the replay list, and with Relay, Proxy and Friend features enabled, the command will be the following::

  mesh models dfu metadata comp-add 0x59 0 0 10 7

Now you need to encode elements the are present on a new image.
For each element to encode, issue the ``mesh models dfu metadata comp-elem-add`` shell command specifying the location of the element, number of Bluetooth SIG and vendor models and their IDs.
For example, for :ref:`ble_mesh_dfu_target` sample, which has only one element containing Configuration and Health Server models as well as DFU and BLOB Transfer Server models, the command will be the following::

  mesh models dfu metadata comp-elem-add 1 4 0 0x0000 0x0002 0xBF42 0xBF44

.. note::
   In case of any mistakes being done during the enconding of the Composition Data, use the ``mesh models dfu metadata comp-clear`` command to clear the cached value, then start composing the metadata from the beginning.

When all elements are added, generate a hash of the Composition Data using the ``mesh models dfu metadata comp-hash-get`` shell command.
For example, using the inputs from the commands above, the output of this command should be the following::

  Composition data to be hashed:
          CID: 0x0059
          PID: 0x0000
          VID: 0x0000
          CPRL: 10
          Features: 0x7
          Elem: 1
                  NumS: 4
                  NumV: 0
                  SIG Model ID: 0x0000
                  SIG Model ID: 0x0002
                  SIG Model ID: 0xbf42
                  SIG Model ID: 0xbf44
  Composition data hash: 0x71f3267c

The generated hash will then be encoded into the metadata.
Use the ``mesh models dfu metadata encode`` shell command to encode the metadata.
For example, using the Composition Data hash generated above, the command to encode the metadata for firmware version ``2.0.0+0``, with a size of 241236 bytes and targeted to application core, will be the following::

  mesh models dfu metadata encode 2 0 0 0 241236 1 0x71f3267c 1

The output of the command will be the following::

  Metadata to be encoded:
          Version: 2.0.0+0
          Size: 241236
          Core Type: 0x1
          Composition data hash: 0x71f3267c
          Elements: 1
          User data length: 0
  Encoded metadata: 020000000100000094cf24017c26f3710100
