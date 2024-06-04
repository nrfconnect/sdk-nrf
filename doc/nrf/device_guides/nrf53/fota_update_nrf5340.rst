.. _ug_nrf53_developing_ble_fota:

FOTA updates with nRF5340 DK
############################

.. contents::
   :local:
   :depth: 2

.. include:: /device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_intro_start
   :end-before: fota_upgrades_intro_end

FOTA over Bluetooth Low Energy
******************************

.. include:: /device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_over_ble_intro_start
   :end-before: fota_upgrades_over_ble_intro_end

.. include:: /device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_over_ble_mandatory_mcuboot_start
   :end-before: fota_upgrades_over_ble_mandatory_mcuboot_end

Bluetooth buffers configuration introduced by the :kconfig:option:`CONFIG_NCS_SAMPLE_MCUMGR_BT_OTA_DFU_SPEEDUP` Kconfig option is also automatically applied to the network core child image by the dedicated overlay file.

.. include:: /device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_over_ble_additional_information_start
   :end-before: fota_upgrades_over_ble_additional_information_end

Testing steps
=============

.. include:: /device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_outro_start
   :end-before: fota_upgrades_outro_end

FOTA update sample
******************

.. include:: /device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_update_start
   :end-before: fota_upgrades_update_end

FOTA in Bluetooth Mesh
**********************

.. include:: /device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_bt_mesh_start
   :end-before: fota_upgrades_bt_mesh_end

.. note::
   Point-to-point DFU over Bluetooth Low Energy is supported by default, out-of-the-box, for all samples and applications compatible with :ref:`zephyr:thingy53_nrf5340`.
   See :ref:`thingy53_app_update` for more information about updating firmware image on :ref:`zephyr:thingy53_nrf5340`.

FOTA in Matter
**************

.. include:: /device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_matter_start
   :end-before: fota_upgrades_matter_end

FOTA over Thread
****************

.. include:: /device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_thread_start
   :end-before: fota_upgrades_thread_end

FOTA over Zigbee
****************

.. include:: /device_guides/nrf52/fota_update.rst
   :start-after: fota_upgrades_zigbee_start
   :end-before: fota_upgrades_zigbee_end
