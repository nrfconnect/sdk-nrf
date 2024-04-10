.. _lwm2m_client_fota:

Evaluating LwM2M Advanced Firmware Update
#########################################

.. contents::
   :local:
   :depth: 2

The Advanced Firmware Update object (object id ``33629``) is an experimental object under development.
It allows to update multiple firmware packages from a device without deviating from the standard LwM2M Firmware Update object.
Advanced Firmware Update shares the first 13 resources with the `OMA LwM2M Firmware Update object v1.1`_ (object id ``5``).
However, Zephyr only implements version 1.0 of the Firmware Update object.

The main difference between these two versions is that the Advanced Firmware Update object is multi-instance.
For more information about the differences, see `LwM2M Firmware Update version 1.1 <LwM2M v1.2 specification_>`_.

.. note::
   This object is still under development in AVSystem Coiote platform.
   The following sections describe the object and how to test it manually without the Coiote user interface that is still under development.

Extended resources
******************

The following table describes the extensions in the object compared to the standard Firmware Update object:

+-------------+-------------------+-----------+----------------------------------------------------+
| Resource ID | Name              | Operation | Description                                        |
+=============+===================+===========+====================================================+
| 14          | Component name    | Read      | Name of the component handled by this instance of  |
|             |                   |           | the Advanced Firmware Update object.               |
+-------------+-------------------+-----------+----------------------------------------------------+
| 15          | Current version   | Read      | Version number of the package that is currently    |
|             |                   |           | installed and running for the component handled    |
|             |                   |           | by this instance of the Advanced Firmware Update   |
|             |                   |           | object.                                            |
+-------------+-------------------+-----------+----------------------------------------------------+
| 16          | Linked instance   | Read      | When multiple instances of the Advanced Firmware   |
|             |                   |           | Update object are in the ``downloaded`` state,     |
|             |                   |           | this resource lists all other instances that will  |
|             |                   |           | be updated in a batch if the Firmware Update       |
|             |                   |           | resource is executed on this instance with no      |
|             |                   |           | arguments. Each of the instances listed must be    |
|             |                   |           | in the ``downloaded`` state.                       |
+-------------+-------------------+-----------+----------------------------------------------------+
| 17          | Conflicting       | Read      | When the download or update fails and the Update   |
|             | instances         |           | Result resource value is set to ``12`` or ``13``,  |
|             |                   |           | this resource must be present and must contain     |
|             |                   |           | references to the Advanced Firmware Update object  |
|             |                   |           | instances that caused the conflict.                |
+-------------+-------------------+-----------+----------------------------------------------------+

Extended update execute resource
********************************

Following table lists the execute resource compared to the standard Firmware Update object:

+-------------+-----------+--------------------------------------------------------------------------+
| Resource ID | Operation | Description                                                              |
+=============+===========+==========================================================================+
| 3           | Execute   | Updates firmware by using the firmware package stored in the package, or |
|             |           | by using the firmware downloaded from the package URI. This resource is  |
|             |           | only executable when the value of the state resource is ``downloaded``.  |
+-------------+-----------+--------------------------------------------------------------------------+

If multiple instances of the Advanced Firmware Update object are in the ``downloaded`` state, the device might update multiple components in one go.
In this case, the linked instances resource must list all other components that update alongside the current one.

The server might override this behavior by including the argument ``0`` in the Execute operation.
If the argument is present but has no value, the LwM2M Client must attempt to update only the component handled by the current instance.
If the argument is present with a value containing a list of the Advanced Firmware Update object instances specified as a core link format (for example, when the argument might read ``0='</33629/1>``), the LwM2M Client must attempt to update the components handled by the current instance and the instances listed in the argument and must not attempt to update any other components.
If the client is not able to satisfy such a request, the update process will fail with the Update Result resource set to ``13``.

Library support
***************

Advanced Firmware update is implemented in the :ref:`lib_lwm2m_client_utils` library and is designed to support modem and application binary updates.

Configuration option
====================

Enable the following Kconfig options to use advanced FOTA:

.. code-block:: console

   CONFIG_LWM2M_CLIENT_UTILS=y
   CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT=y
   CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_UPDATE_OBJ_SUPPORT=y
   CONFIG_LWM2M_FIRMWARE_UPDATE_OBJ_SUPPORT=n
   CONFIG_FOTA_CLIENT_AUTOSCHEDULE_UPDATE=n
   CONFIG_LWM2M_RW_OMA_TLV_SUPPORT=y

The :file:`overlay-adv-firmware.conf` configuration file includes these options.

Initializing
============

The Advanced Firmware Update functionality is implemented in :file:`lwm2m_firmware.c` and :file:`lwm2m_adv_firmware.c`.
The Advanced Firmware Update object shares the same API with the standard LwM2M Firmware Update object as implemented in the :ref:`lib_lwm2m_client_utils` library.
The image is automatically confirmed to be valid on the boot and that marks the FOTA process as completed.
The modem FOTA process is automatically validated during update; it does not reboot the device.

LwM2M Client utilities library setup
====================================

By default, the Advanced Firmware Update object supports two instances that are configured in the following way:

+-------------+-------------+---------------------------------------------------------------------+
| Instance ID | Owner       | DFU types                                                           |
+=============+=============+=====================================================================+
| 0           | Application | DFU_TARGET_IMAGE_TYPE_MCUBOOT                                       |
+-------------+-------------+---------------------------------------------------------------------+
| 1           | Modem       | DFU_TARGET_IMAGE_TYPE_MODEM_DELTA, DFU_TARGET_IMAGE_TYPE_FULL_MODEM |
+-------------+-------------+---------------------------------------------------------------------+

Advanced Firmware Update helper script
**************************************

For working with the `AVSystem Coiote server <Coiote Device Management_>`_, a helper script has been provided.
To automate the firmware update, you can use the script :file:`fota.py` that is available in the :file:`samples/cellular/lwm2m_client/scripts/` folder.
The :file:`fota.py` file supports ``update`` and ``upload`` commands for firmware updates.

The commands can be used in the following way:

.. parsed-literal::

   ./scripts/fota.py -id *device_id* -to *time_out* update *instance_id* *binary_type* *binary_src*
   ./scripts/fota.py upload *instance_id* *binary_name*

FOTA upload command
===================

The FOTA ``upload`` command allocates a resource ID and uploads the binary to Coiote Device Management server.
Currently, the script defines resource ID based on the given instance ID: ``lwm2m_client_fota_instance_<instance_id>``.

.. code-block:: console

   ./scripts/fota.py upload 0 app_update.bin

The following example output shows the allocation of a resource id ``lwm2m_client_fota_instance_0`` on the server side:

.. code-block:: console

   [INFO] fota.py - Upload app_update.bin with instance 0 to Coiote
   [INFO] fota.py - Binary app_update.bin, Size 364747 (bytes)
   [INFO] coiote.py - Creating fota resource for binary app_update.bin with id lwm2m_client_fota_instance_0
   [INFO] fota.py - Allocated Resource id lwm2m_client_fota_instance_0 for instance 0

.. tabs::

   .. group-tab:: nRF91x1 DK

      .. code-block:: console

         ./scripts/fota.py upload 1 mfw_nrf91x1_update_from_2.x.x_to_2.x.x-FOTA-TEST.bin

      where 2.x.x is the latest modem release version.

   .. group-tab:: nRF9160 DK

      .. code-block:: console

         ./scripts/fota.py upload 1 mfw_nrf9160_update_from_1.x.x_to_1.x.x-FOTA-TEST.bin

      where 1.x.x is the latest modem release version.

The following is an example output that shows the allocation of a resource id ``lwm2m_client_fota_instance_0`` on the server side for the nRF9160 DK:

.. code-block:: console

   [INFO] fota.py - Upload mfw_nrf9160_update_from_1.3.5_to_1.3.5-FOTA-TEST.bin with instance 1 to Coiote
   [INFO] fota.py - Binary mfw_nrf9160_update_from_1.3.5_to_1.3.5-FOTA-TEST.bin, Size 14312 (bytes)
   [INFO] coiote.py - Creating fota resource for binary mfw_nrf9160_update_from_1.3.5_to_1.3.5-FOTA-TEST.bin with id lwm2m_client_fota_instance_1
   [INFO] fota.py - Allocated Resource id lwm2m_client_fota_instance_1 for instance 1

FOTA update command
===================

For running the full FOTA process, the firmware can be given as a resource ID that already exists on the server side, or as a binary file that will be uploaded automatically.

To use a binary name, run the script with the following parameters:

.. parsed-literal::

   ./scripts/fota.py -id *device_id* -to *timeout* update *instance_id* file *binary_name*

When binary type is ``file``, script tries to discover given binary name from the :file:`samples/cellular/lwm2m_client/` file, or the :file:`samples/cellular/lwm2m_client/build/zephyr`.
The ``update`` command uploads and generates binary resource IDs automatically when a binary file is used.
The update default task timeout is 800 seconds.

Following is an example of updating a modem instance by giving a binary file:

.. tabs::

   .. group-tab:: nRF91x1 DK

      .. code-block:: console

         ./scripts/fota.py -id urn:imei:359746166785274 update 1 file mfw_nrf91x1_update_from_2.x.x_to_2.x.x-FOTA-TEST.bin

      where 2.x.x is the latest modem release version.

   .. group-tab:: nRF9160 DK

      .. code-block:: console

         ./scripts/fota.py -id urn:imei:351358811331351 update 1 file mfw_nrf9160_update_from_1.x.x_to_1.x.x-FOTA-TEST.bin

      where 1.x.x is the latest modem release version.

To use existing resource IDs, run the script with the following parameters:

.. parsed-literal::

   ./scripts/fota.py -id *device_id* update *instance_id* resource *resource_id*

Following is an example of uploading a binary and updating a modem by referring to the uploaded resource ID:

.. tabs::

      .. group-tab:: nRF91x1 DK

         .. code-block:: console

            ./scripts/fota.py upload 1 mfw_nrf91x1_update_from_2.x.x-FOTA-TEST_to_2.x.x.bin
            ./scripts/fota.py -id urn:imei:359746166785274 update 1 resource lwm2m_client_fota_instance_1

         where 2.x.x is the latest modem release version.

      .. group-tab:: nRF9160 DK

         .. code-block:: console

            ./scripts/fota.py upload 1 mfw_nrf9160_update_from_1.x.x-FOTA-TEST_to_1.x.x.bin
            ./scripts/fota.py -id urn:imei:351358811331351 update 1 resource lwm2m_client_fota_instance_1

         where 1.x.x is the latest modem release version.

Testing advanced FOTA
**********************

Complete the following steps to test the advanced FOTA firmware update with the lwM2M client sample and the :file:`/scripts/fota.py` file.


   #. Download the latest released modem zip file from `nRF9160 DK Downloads`_ or `nRF9161 DK Downloads`_.
   #. Update the modem firmware using the nRF Programmer app of `nRF Connect for Desktop`_.
   #. Copy the binaries with the following naming format from the zip file to the folder :file:`/nrf/samples/cellular/lwm2m_client`:

   .. tabs::

      .. group-tab:: nRF91x1 DK

         * :file:`mfw_nrf91x1_update_from_2.x.x_to_2.x.x-FOTA-TEST.bin`
         * :file:`mfw_nrf91x1_update_from_2.x.x-FOTA-TEST_to_2.x.x.bin`

         where 2.x.x is the latest modem release version.

      .. group-tab:: nRF9160 DK

         * :file:`mfw_nrf9160_update_from_1.x.x_to_1.x.x-FOTA-TEST.bin`
         * :file:`mfw_nrf9160_update_from_1.x.x-FOTA-TEST_to_1.x.x.bin`

         where 1.x.x is the latest modem release version.

   #. To set up the script, you must set the username and password that you used in AVSystem's Coiote Device Management server as the environment variables.

      .. code-block:: console

         # Setup phase
         export COIOTE_PASSWD='my-password'
         export COIOTE_USER='my-username'

   #. To use the :ref:`lwm2m_client` sample after updating the firmware, you must build the sample with the following overlays:

         * :file:`overlay-avsystems.conf`
         * :file:`overlay-lwm2m-1.1.conf`
         * :file:`overlay-fota_helper.conf`
         * :file:`overlay-adv-firmware.conf`

   #. Register your device with the Coiote Device management server.
   #. Flash the compiled sample using the erase flash option.
   #. Wait for the device registration to be complete.
   #. Open the :file:`src/prj.conf` file.
   #. Change :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` to ``1.0.1`` and rebuild the sample.
   #. Update the application and modem firmware by using the :file:`/scripts/fota.py` script:

      .. tabs::

         .. group-tab:: nRF91x1 DK

            .. code-block:: console

               ./scripts/fota.py -id urn:imei:359746166785274 update 0 file app_update.bin update 1 file mfw_nrf91x1_update_from_2.x.x_to_2.x.x-FOTA-TEST.bin

         .. group-tab:: nRF9160 DK

            .. code-block:: console

               ./scripts/fota.py -id urn:imei:351358811331351 update 0 file app_update.bin update 1 file mfw_nrf9160_update_from_1.x.x_to_1.x.x-FOTA-TEST.bin

      Following is an example output of the command for the nRF9160 DK:

      .. code-block:: console

         [INFO] fota.py - Client identity: urn:imei:351358811331351
         [INFO] fota.py - Binary app_update.bin, Size 364747 (bytes)
         [INFO] coiote.py - Creating fota resource for binary app_update.bin with id lwm2m_client_fota_instance_0
         [INFO] fota.py - Init setup for instance 0 firmware Update resource:lwm2m_client_fota_instance_0
         [INFO] fota.py - Binary mfw_nrf9160_update_from_1.3.5_to_1.3.5-FOTA-TEST.bin, Size 14312 (bytes)
         [INFO] coiote.py - Creating fota resource for binary mfw_nrf9160_update_from_1.3.5_to_1.3.5-FOTA-TEST.bin with id lwm2m_client_fota_instance_1
         [INFO] fota.py - Init setup for instance 1 firmware Update resource:lwm2m_client_fota_instance_1
         [INFO] fota.py - Start Firmware Update
         [INFO] fota.py - Delete Observation Advanced Firmware Update
         [WARNING] coiote.py - Coiote: Path Advanced Firmware Update was not observed
         [INFO] fota.py - Write Fota Download url to Advanced Firmware Update.0.Package URI
         [INFO] fota.py - Write Fota Download url to Advanced Firmware Update.1.Package URI
         [INFO] coiote.py - Device is Queuemode Coiote have to wait next Registration Update
         [INFO] fota.py - Download Url Write done
         [INFO] fota.py - Enable Observation Advanced Firmware Update
         [INFO] fota.py - Downloading instance: 0
         [INFO] fota.py - Downloading instance: 1
         [INFO] fota.py - Download ready instance: 0
         [INFO] fota.py - Download ready instance: 1
         [INFO] coiote.py - Device is Queuemode Coiote have to wait next Registration Update
         [INFO] fota.py - Update started instance: 0
         [INFO] fota.py - Update started instance: 1
         [INFO] fota.py - Firmware Update Successfully instance: 0
         [INFO] fota.py - From:1.0.0-0 to 1.0.1-0
         [INFO] fota.py - Firmware Update Successfully instance: 1
         [INFO] fota.py - From:mfw_nrf9160_1.3.5 to mfw_nrf9160_1.3.5-FOTA-TEST
         [INFO] fota.py - Firmware update process finished
         [INFO] fota.py - Delete Observation Advanced Firmware Update

   #. Update the modem firmware back to the original released version:

      .. tabs::

         .. group-tab:: nRF91x1 DK

            .. code-block:: console

               ./scripts/fota.py -id urn:imei:359746166785274 update 1 mfw_nrf91x1_update_from_2.x.x-FOTA-TEST_to_2.x.x.bin

         .. group-tab:: nRF9160 DK

            .. code-block:: console

               ./scripts/fota.py -id urn:imei:351358811331351 update 1 file mfw_nrf9160_update_from_1.x.x-FOTA-TEST_to_1.x.x.bin

      Following is an example output of the command for the nRF9160 DK:

      .. code-block:: console

         [INFO] fota.py - Client identity: urn:imei:351358811331351
         [INFO] fota.py - Binary mfw_nrf9160_update_from_1.3.5-FOTA-TEST_to_1.3.5.bin, Size 14312 (bytes)
         [INFO] coiote.py - Creating fota resource for binary mfw_nrf9160_update_from_1.3.5-FOTA-TEST_to_1.3.5.bin with id lwm2m_client_fota_instance_1
         [INFO] fota.py - Init setup for instance 1 firmware Update resource:lwm2m_client_fota_instance_1
         [INFO] fota.py - Start Firmware Update
         [INFO] fota.py - Delete Observation Advanced Firmware Update
         [WARNING] coiote.py - Coiote: Path Advanced Firmware Update was not observed
         [INFO] fota.py - Write Fota Download url to Advanced Firmware Update.1.Package URI
         [INFO] coiote.py - Device is Queuemode Coiote have to wait next Registration Update
         [INFO] fota.py - Download Url Write done
         [INFO] fota.py - Enable Observation Advanced Firmware Update
         [INFO] fota.py - Downloading instance: 1
         [INFO] fota.py - Download ready instance: 1
         [INFO] coiote.py - Device is Queuemode Coiote have to wait next Registration Update
         [INFO] fota.py - Update started instance: 1
         [INFO] fota.py - Firmware Update Successfully instance: 1
         [INFO] fota.py - From:mfw_nrf9160_1.3.5-FOTA-TEST to mfw_nrf9160_1.3.5
         [INFO] fota.py - Firmware update process finished
         [INFO] fota.py - Delete Observation Advanced Firmware Update
