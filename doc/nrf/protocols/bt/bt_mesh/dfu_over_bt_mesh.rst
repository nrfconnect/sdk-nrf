.. _dfu_over_bt_mesh:

DFU over Bluetooth Mesh
#######################

.. contents::
   :local:
   :depth: 2

The DFU specification is implemented in the Zephyr Bluetooth Mesh DFU subsystem as three separate models:

* :ref:`zephyr:bluetooth_mesh_dfu_srv`
* :ref:`zephyr:bluetooth_mesh_dfu_cli`
* :ref:`zephyr:bluetooth_mesh_dfd_srv`

For more information about the Zephyr Bluetooth Mesh DFU subsystem, see :ref:`zephyr:bluetooth_mesh_dfu`.

The Bluetooth Mesh subsystem in |NCS| provides a set of samples that can be used for evaluation of the Bluetooth Mesh DFU specification and subsystem:

* :ref:`ble_mesh_dfu_target` sample
* :ref:`ble_mesh_dfu_distributor` sample

To configure and control DFU on the Firmware Distribution Server, it is required to have either the Firmware Distribution Client model or the shell commands.

Either the nRF Mesh app or the shell commands can be used to configure and control the DFU process.

.. note::
   The Mesh DFU feature in the nRF Mesh app is currently supported only on iOS.

.. tabs::
   .. group-tab:: nRF Mesh app (iOS)

      The nRF Mesh app for iOS provides a built-in Firmware Distribution Client model to configure and control the DFU process on the distributor node.

      .. figure:: images/bt_mesh_dfuapp_00_proxy.png
         :alt: nRF Mesh app Proxy screen with Mesh DFU Distributor
         :align: right
         :figwidth: 450px
         :width: 120px

         **Proxy** screen with distributor node connection and proxy filter

      To connect to the distributor node:

      * Launch the nRF Mesh app for iOS and navigate to the **Proxy** screen.
      * If the *Proxy* section does not show 'Mesh DFU Distributor', disable *Automatic Connection*, click the *Proxy* name, and choose the distributor node from the list.


   .. group-tab:: Shell

      The Bluetooth Mesh DFU subsystem in Zephyr provides a set of shell commands that can be used to substitute the need for the client.
      For the complete list of commands, see the :ref:`zephyr:bluetooth_mesh_shell_dfd_server` section of the Bluetooth Mesh shell documentation.

      The commands can be executed in two ways:

      * Through the shell management subsystem of MCU manager (for example, using the nRF Connect Device Manager mobile application on Android or :ref:`Mcumgr command-line tool <dfu_tools_mcumgr_cli>`).
      * By accessing the :ref:`zephyr:shell_api` module over UART.

Provisioning and configuring the devices
****************************************

After programming the samples onto the boards, they need to be provisioned into the same Bluetooth Mesh network with an external provisioner device.
See the documentation for :ref:`provisioning the mesh DFU target device <ble_mesh_dfu_target_provisioning>` and :ref:`provisioning the mesh DFU distributor device <ble_mesh_dfu_distributor_provisioning>` for how this is done.

After the provisioning is completed, a Configuration Client needs to add a common application key to all devices.
The added application key must be bound:

* On the Distributor: to the Firmware Distribution Server, Firmware Update Client, BLOB Transfer Server and BLOB Transfer Client models instantiated on the primary element, and to the Firmware Update Server and BLOB Transfer Server models instantiated on the secondary element of the device (see :ref:`Configuring models on the Distributor node <ble_mesh_dfu_distributor_model_config>`).
* On Target nodes: to the Firmware Update Server and BLOB Transfer Server models instantiated on the primary element of the device (see :ref:`Configuring models on the Target node <ble_mesh_dfu_target_model_config>`).

The bound application key will be used in the firmware distribution procedure.


Uploading the firmware
**********************

.. tabs::
   .. group-tab:: nRF Mesh app (iOS)

      .. note::
         When using the iOS nRF Mesh app for DFU, bind the application key to the Firmware Distribution Server model on the distributor node. The app binds the application key to the Firmware Update Server models on the Target nodes automatically.

      To upload firmware:

      * Navigate to the **Network** screen and click the :guilabel:`Update` button in the top right corner.

         .. figure:: images/bt_mesh_dfuapp_01_nw.png
            :alt: nRF Mesh app Network screen with Update button
            :figwidth: 450px
	    :width: 120px

            **Network** screen with mesh nodes and :guilabel:`Update` button

      * On the **Firmware Distribution** screen, review the distributor node information, the current state of the Firmware Distribution Server model, and the *Capabilities* section.

         .. figure:: images/bt_mesh_dfuapp_02_fdsconfig.png
            :alt: Firmware Distributor screen with model details
            :figwidth: 450px
	    :width: 120px

            **Firmware Distributor** screen showing details of the Firmware Distributor Server model

         .. figure:: images/bt_mesh_dfuapp_04_fdsconfig.png
            :alt: Firmware Distributor with firmware image slot empty and available
            :figwidth: 450px
	    :width: 120px

            **Firmware Distributor** with firmware image slot empty and available

      * Click :guilabel:`Next` in the top right corner of the Firmware Distribution screen to open the **Target Nodes** screen.
      * Click :guilabel:`Select File` and select the zip archive intended for distribution. Verify the firmware details shown by the app.

   .. group-tab:: Shell

      After configuring the models, a new image can be uploaded to the Distributor.
      To upload the image, follow the instructions provided in the :ref:`ble_mesh_dfu_distributor_fw_image_upload` section of the :ref:`ble_mesh_dfu_distributor` sample documentation.

      The uploaded image needs to be registered in the Bluetooth Mesh DFU subsystem.
      To achieve this, issue the ``mesh models dfu slot add`` shell command specifying size in bytes of the image that was uploaded to the Distributor.
      Optionally, you can provide firmware ID, metadata and Unique Resource Identifier (URI) parameters that come with the image.

      For example, to allocate a slot for the :ref:`ble_mesh_dfu_target` sample with image size of 241236 bytes, with firmware ID set to ``0200000000000000``, and metadata generated as described in :ref:`bluetooth_mesh_dfu_eval_md` section below, type the following command::

        mesh models dfu slot add 241236 0200000000000000 020000000000000094cf24017c26f3710100

      When the slot is added, the shell will print the slot ID.
      Take note of this ID as it will then be needed to start the DFU transfer::

        Adding slot (size: 241236)
        Slot added. ID: 0

      .. note::
         To update any value in a slot, issue the ``mesh models dfu slot del`` command specifying the ID of the allocated slot, and then add the slot again.

Populating the Distributor's receivers list
*******************************************

.. tabs::
   .. group-tab:: nRF Mesh app (iOS)

      To add Target nodes to the receivers list:

      * Scroll down to the *AVAILABLE TARGET NODES* section.
      * Select the nodes to update by clicking on them, or click :guilabel:`Select All` to select all nodes.
      * Click :guilabel:`Next` in the top right corner of the screen to go to the **Firmware Distribution** screen.

         .. figure:: images/bt_mesh_dfuapp_05_tgtnodes.png
            :alt: Target Nodes screen with firmware selection and node list
            :figwidth: 450px
	    :width: 120px

            **Target Nodes** screen with firmware details and available target nodes to select

   .. group-tab:: Shell

      Add Target nodes to the DFU transfer by issuing the ``mesh models dfd receivers-add`` shell command.
      This shell command is specifying the element address of a Target node with the Firmware Update Server instance and the image index on the Target node that needs to be updated.
      For example, for two Target nodes with addresses ``0x0004`` and ``0x0005`` respectively, and with image index 0, the command will look like this::

        mesh models dfd receivers-add 0x0004,0;0x0005,0

      .. note::
         To remove all receivers from the list, issue the ``mesh models dfd receivers-delete-all`` command.


Initiating the distribution
***************************

.. tabs::
   .. group-tab:: nRF Mesh app (iOS)

      To set distribution parameters on the **Firmware Distribution** screen:

      * Set transfer TTL, Timeout, Transfer Mode, and Update Policy (you may leave default values as is).
      * To transfer firmware to several Target nodes simultaneously, under the 'Multicast Distribution' section choose the desired group address.

         .. figure:: images/bt_mesh_dfuapp_06_fds.png
            :alt: Firmware Distribution parameters (TTL, timeout, transfer mode, policy, multicast)
            :figwidth: 450px
	    :width: 120px

            **Firmware Distribution** screen with transfer parameters and multicast options

   .. group-tab:: Shell
      To start the DFU transfer, issue the ``mesh models dfd start`` shell command.
      This command requires two mandatory arguments: ``app_idx`` and ``slot_idx``:

      * As ``app_idx``, use the application key index that is bound to the Firmware Distribution Server and other Firmware Update and BLOB Transfer models on the Distributor and Target nodes.
      * As ``slot_idx``, use the ID of the slot allocated by the ``mesh models dfu slot add`` shell command on the previous step.

      For example, to run the DFU transfer in unicast mode, with AppKey index 0 and slot ID 0, call::

        mesh models dfd start 0 0

      By default, the Firmware Distribution Server will request the Firmware Update Servers to apply the image immediately after the DFU transfer.
      To avoid applying the image immediately and only verify it, set the 4th argument to 0::

        mesh models dfd start 0 0 0 0

      .. note::
         After a successful firmware distribution, the Firmware Distribution Server has to be set to idle state by issuing the ``mesh models dfd cancel`` shell command, before a new firmware distribution can be initiated.

Firmware distribution
=====================

.. tabs::
   .. group-tab:: nRF Mesh app (iOS)

      To start the distribution:

      * After setting the Firmware Distribution parameters and multicast options, click :guilabel:`Next` in the top right corner of the screen. The app binds the necessary keys and subscribes the target nodes to the group address.

         .. figure:: images/bt_mesh_dfuapp_07_autoconf.png
            :alt: App configures distributor and target nodes
            :figwidth: 450px
	    :width: 120px

            App configures distributor and target nodes

      * When configuration is complete, click :guilabel:`Next` again. The app uploads the firmware image to the distributor node over SMP (Firmware Upload; typically a couple of minutes), then instructs the distributor to start the Firmware Distribution process.

         .. figure:: images/bt_mesh_dfuapp_08_fu.png
            :alt: Firmware Update screen during image upload to distributor
            :figwidth: 450px
	    :width: 120px

            Firmware Update screen while uploading image to the distributor

         The mobile device may be disconnected; the distributor node continues the Firmware Distribution process in the background.

         .. figure:: images/bt_mesh_dfuapp_09_fu.png
            :alt: Firmware Update screen during distribution to targets
            :figwidth: 450px
	    :width: 120px

            Firmware Update screen during distribution to target nodes

      To exit the Firmware Update screen without canceling the distribution, press the :guilabel:`Close` button in the top right corner of the screen (the distribution process continues in the background). Pressing the :guilabel:`X` button in the top left corner cancels the distribution process.

   .. group-tab:: Shell

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
      When the DFU transfer successfully completes, the phase will be set to :c:enum:`BT_MESH_DFD_PHASE_TRANSFER_SUCCESS`, for example::

        { "status": 0, "phase": 2, "group": 0x0000, "app_idx": 0, "ttl": 255, "timeout_base": 0, "xfer_mode": 1, "apply": 0, "slot_idx": 0 }

      The :c:enum:`bt_mesh_dfd_phase` enumeration contains the complete list of distribution phases.

Suspending the distribution
===========================

.. tabs::
   .. group-tab:: nRF Mesh app (iOS)

      To suspend the distribution:

      * Navigate to the **Network** screen, click the :guilabel:`Mesh DFU Distributor` node, navigate to the element list, click :guilabel:`Element 1`, then :guilabel:`Firmware Distribution Server`.
      * On the **Firmware Distribution Server** screen, scroll to the *Control* section and click :guilabel:`Get` to obtain the current state.
      * Click the :guilabel:`Suspend` button.

         .. figure:: images/bt_mesh_dfuapp_10_fds.png
            :alt: Firmware Distribution Server screen with Control (Get, Suspend, Cancel, Apply)
            :figwidth: 450px
	    :width: 120px

            **Firmware Distribution Server** screen with status, phase, and control actions

   .. group-tab:: Shell

      To suspend the firmware distribution, use the ``mesh models dfd suspend`` shell command.
      The distribution phase is switched to :c:enum:`BT_MESH_DFD_PHASE_TRANSFER_SUSPENDED` in this case.

      To resume the DFU transfer after suspending, issue the ``mesh models dfu cli resume`` shell command.

Applying the firmware image
***************************

Depending on the update policy set at the start of the DFU transfer, the Firmware Distribution Server will do the following:

* If ``policy_apply`` is set to true or omitted when the DFU transfer starts, the Firmware Distribution Server will immediately apply the new firmware on the Target nodes upon the DFU transfer completion.
* If ``policy_apply`` is set to false, the image needs to be applied manually.

.. tabs::
   .. group-tab:: nRF Mesh app (iOS)

      When the update policy was set to Verify Only, the target nodes wait for an apply message after the distribution completes. To apply the firmware:

      * Navigate to the **Network** screen, click the :guilabel:`Mesh DFU Distributor` node, navigate to the element list, click :guilabel:`Element 1`, then :guilabel:`Firmware Distribution Server`.
      * On the **Firmware Distribution Server** screen, scroll to the *Control* section and click :guilabel:`Get` to obtain the current state.
      * Click the :guilabel:`Apply` button.

         .. figure:: images/bt_mesh_dfuapp_10_fds.png
            :alt: Firmware Distribution Server screen with Control (Get, Suspend, Cancel, Apply)
            :figwidth: 450px
	    :width: 120px

            **Firmware Distribution Server** screen with status, phase, and control actions

   .. group-tab:: Shell

      If the update policy was set to Verify Only, to apply the image manually after the DFU transfer is completed, use the ``mesh models dfd apply`` command.

When the Firmware Distribution Server starts applying the transferred image, the distribution phase is set to :c:enum:`BT_MESH_DFD_PHASE_APPLYING_UPDATE`.

After applying the new firmware, the Firmware Distribution Server will immediately request firmware ID of the currently running firmware on the Target nodes to confirm that the new firmware has been applied successfully.
Depending on the :c:enum:`bt_mesh_dfu_effect` value received from the Target nodes after the DFU transfer is started, the following cases are possible:

* If the image effect for a particular Target node is :c:enum:`BT_MESH_DFU_EFFECT_UNPROV`, the Firmware Distribution Server does not expect any reply from that Target node.
  If the Distributor does not receive any reply, it will repeat the request several times.
  If the Distributor eventually receives a reply, the DFU for this particular Target node is considered unsuccessful.
  Otherwise, the DFU is considered successful.
* In all other cases, the Distributor expects a reply from the Target node with the firmware ID equal to the firmware ID of the transferred image.
  If the Target node responds with a different firmware ID or does not respond at all after several requests, the DFU for this particular Target node is considered unsuccessful.
  Otherwise, the DFU is considered successful.

The DFU ends after the Distributor stops polling the Target nodes.
If the DFU completes successfully for at least one Target node, the firmware distribution is considered as successful.
In this case, the distribution phase is set to :c:enum:`BT_MESH_DFD_PHASE_COMPLETED`.
If the DFU does not complete successfully, the distribution phase is set to :c:enum:`BT_MESH_DFD_PHASE_FAILED`.

.. _bluetooth_mesh_dfu_cancelling_distribution:

Cancelling the distribution
***************************

.. tabs::
   .. group-tab:: nRF Mesh app (iOS)

      To cancel the distribution:

      * Navigate to the **Network** screen, click the :guilabel:`Mesh DFU Distributor` node, navigate to the element list, click :guilabel:`Element 1`, then :guilabel:`Firmware Distribution Server`.
      * On the **Firmware Distribution Server** screen, scroll to the *Control* section and click :guilabel:`Get` to obtain the current state.
      * Click the :guilabel:`Cancel` button.

      Alternatively, open the Firmware Distribution flow by clicking the :guilabel:`Update` button on the **Network** screen, then press the :guilabel:`X` button in the top left corner of the **Firmware Update** screen.

   .. group-tab:: Shell

      To cancel the firmware distribution, use the ``mesh models dfd cancel`` shell command.
      The Firmware Distribution Server will start the cancelling procedure by sending a cancel message to all Targets and will switch phase to :c:enum:`BT_MESH_DFD_PHASE_CANCELING_UPDATE`.
      Once the cancelling procedure is completed, the phase is set to :c:enum:`BT_MESH_DFD_PHASE_IDLE`.

      .. note::
         It is possible to cancel the firmware distribution on a specific Target node at any time by sending a Firmware Update Cancel message.
         To do this, use the ``mesh models dfu cli cancel`` shell command specifying the unicast address of the Target node.

Recovering from failed distribution
***********************************

.. tabs::
   .. group-tab:: nRF Mesh app (iOS)

      To recover from a failed distribution, clear the distribution phase and optionally clear the firmware slots so that a new distribution can be started.

      To clear the distribution phase and reset the Firmware Distribution Server state, follow the steps in :ref:`bluetooth_mesh_dfu_cancelling_distribution`.

      To clear the list of firmware slots (for example, to remove slot data from the failed distribution before retrying):

      * Navigate to the **Network** screen, click the :guilabel:`Mesh DFU Distributor` node, navigate to the element list, click :guilabel:`Element 1`, click :guilabel:`Firmware Distribution Server`, and scroll to the *Firmware Distribution Slots* section.
      * Click :guilabel:`Get` to obtain the current list of firmware slots.
      * To remove a slot: swipe left on the slot and click :guilabel:`Delete`. To remove all slots: click :guilabel:`Delete All`.

      Then start the distribution process again from the beginning.

   .. group-tab:: Shell

      If the firmware distribution fails for any reason, the list of Target nodes should be cleared and the distribution phase should be set to :c:enum:`BT_MESH_DFD_PHASE_IDLE` before making a new attempt.
      To do this, run the following shell commands::

        mesh models dfd receivers-delete-all
        mesh models dfd cancel

      To bring a stalled Target node to idle state, use the ``mesh models dfu cli cancel`` shell command.

      .. note::
         This does not affect the allocated image slots.

.. _bluetooth_mesh_dfu_eval_md:

Generating the firmware metadata
********************************

.. note::
   When using the nRF Mesh app for DFU, the app obtains all required information from the firmware archive generated as part of the build process.


There are two ways to generate the required DFU metadata:

  * Automated generation using the DFU metadata extraction script integrated in the |NCS| build system.
  * Manual generation by using shell commands.

Using the DFU metadata extraction script is the most efficient way of generating the required DFU metadata.

Automated metadata generation
=============================

By enabling the :kconfig:option:`SB_CONFIG_DFU_ZIP_BLUETOOTH_MESH_METADATA` option in sysbuild, the metadata will be automatically parsed from the ``.elf`` and ``.config`` files.
The parsed data is stored in the :file:`ble_mesh_metadata.json` file.
The file is placed in the :file:`dfu_application.zip` archive in the build folder of the application.
Additionally, the metadata string and, optionally, the firmware ID required by the ``mesh models dfu slot add`` command are printed in the command-line window when the application is built::

  Bluetooth Mesh Composition metadata generated:
    Encoded metadata: 020000000000000094cf24017c26f3710100
    Firmware ID: 59000200000000000000
    Full metadata written to: APPLICATION_FOLDER\build\zephyr\dfu_application.zip

You can generate the Firmware ID using one of the following options:

* To use a user-supplied hex-string as the Firmware ID, enable the :kconfig:option:`SB_CONFIG_DFU_ZIP_BLUETOOTH_MESH_METADATA_FWID_CUSTOM` option.
  This is the default behavior.

  * If this option is selected, the :kconfig:option:`SB_CONFIG_DFU_ZIP_BLUETOOTH_MESH_METADATA_FWID_CUSTOM_HEX` option must be set to a valid Firmware ID.
    At minimum, it should include a Company ID in little-endian order.
    The rest of the string is vendor-specific version information.
  * When using the :ref:`zephyr:bluetooth_mesh_dfd_srv` for distribution, the number of bytes in the Firmware ID of images used during distribution must not exceed the value of the :kconfig:option:`CONFIG_BT_MESH_DFU_FWID_MAXLEN` Kconfig option set on the distributor node.

* To use a Firmware ID consisting of the values of the :kconfig:option:`CONFIG_BT_COMPANY_ID` and the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig options, enable the :kconfig:option:`SB_CONFIG_DFU_ZIP_BLUETOOTH_MESH_METADATA_FWID_MCUBOOT_VERSION` option.

.. note::
   The Firmware ID (FWID) specified in the JSON file must match the ``fwid`` used when initializing the :ref:`zephyr:bluetooth_mesh_dfu_srv`.
   This ensures that after the firmware update is applied, the node will respond with the correct Current Firmware ID in the Firmware Update Information Status message.

.. note::
   It is required that the Composition Data is declared with the ``const`` qualifier.
   If the application contains more than one Composition Data structure (for example, when the structure to be used is picked at runtime), the script will not print any encoded metadata.
   In this case, use the JSON file to find the encoded metadata matching the Composition Data to be used by the device after the update.
   Additionally, the script is hardcoded to produce a metadata string where the firmware is targeted for the application core.

A separate west command can be utilized to print the metadata to the console, given that it is already generated by the build system.
This gives the user easy access to this information, without having to enter the ``.json`` file in the build folder or to rebuild the application::

  west build -t ble_mesh_dfu_metadata

For this particular example, the following output is generated:

.. toggle::

  .. code-block:: console

      {
        "sign_version": {
          "major": 2,
          "minor": 0,
          "revision": 0,
          "build_number": 0
        },
        "binary_size": 241236,
        "composition_data": {
          "cid": 89,
          "pid": 0,
          "vid": 0,
          "crpl": 10,
          "features": 7,
          "elements": [
            {
              "location": 1,
              "sig_models": [
                0,
                2,
                48962,
                48964
              ],
              "vendor_models": []
            }
          ]
        },
        "composition_hash": 1911760508,
        "encoded_metadata": "020000000000000094cf24017c26f3710100",
        "firmware_id": "59000200000000000000"
      }

Manual metadata generation
==========================

The Bluetooth Mesh DFU subsystem provides a set of shell commands that can be used to compose a firmware metadata.
The format of metadata is defined in the :c:struct:`bt_mesh_dfu_metadata` structure.
For the complete list of commands, see :ref:`zephyr:bluetooth_mesh_shell_dfu_metadata`.

To start composing metadata, issue the ``mesh models dfu metadata comp-add`` shell command that encodes a Composition Data header.
For example, for a Target node with product ID 0x0059, zero company and version IDs, 10 entries in the replay list, and with Relay, Proxy and Friend features enabled, the command will be the following::

  mesh models dfu metadata comp-add 0x59 0 0 10 7

Now you need to encode elements that are present on a new image.
For each element to encode, issue the ``mesh models dfu metadata comp-elem-add`` shell command specifying the location of the element, number of Bluetooth SIG and vendor models and their IDs.
For example, for :ref:`ble_mesh_dfu_target` sample, which has only one element containing Configuration and Health Server models as well as DFU and BLOB Transfer Server models, the command will be the following::

  mesh models dfu metadata comp-elem-add 1 4 0 0x0000 0x0002 0xBF42 0xBF44

.. note::
   In case of any mistakes during the encoding of the Composition Data, use the ``mesh models dfu metadata comp-clear`` command to clear the cached value, then start composing the metadata from the beginning.

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
  Encoded metadata: 020000000000000094cf24017c26f3710100
