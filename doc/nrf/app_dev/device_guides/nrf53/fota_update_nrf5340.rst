.. _ug_nrf53_developing_ble_fota:

FOTA updates with nRF5340 DK
############################

.. contents::
   :local:
   :depth: 2

.. include:: /includes/fota_upgrades_intro.rst
   :start-after: fota_upgrades_intro_start
   :end-before: fota_upgrades_intro_end

.. _ug_nrf53_developing_ble_fota_steps:

FOTA over Bluetooth Low Energy
******************************

.. note::
   |bluetooth_fota_note|

.. include:: /includes/fota_upgrades_over_ble_intro.rst
   :start-after: fota_upgrades_over_ble_intro_start
   :end-before: fota_upgrades_over_ble_intro_end

.. include:: /includes/fota_upgrades_over_ble_mandatory_mcuboot.rst
   :start-after: fota_upgrades_over_ble_mandatory_mcuboot_start
   :end-before: fota_upgrades_over_ble_mandatory_mcuboot_end

.. include:: /includes/fota_upgrades_over_ble_additional_information.rst
   :start-after: fota_upgrades_over_ble_additional_information_start
   :end-before: fota_upgrades_over_ble_additional_information_end

.. _ug_nrf53_developing_ble_fota_steps_testing:

Testing steps
=============

.. include:: /includes/fota_upgrades_ble_testing_nrfcdm.rst
   :start-after: fota_upgrades_outro_start
   :end-before: fota_upgrades_outro_end

FOTA update sample
******************

.. include:: /includes/fota_upgrades_update_sample.rst
   :start-after: fota_upgrades_update_start
   :end-before: fota_upgrades_update_end

FOTA in Bluetooth Mesh
**********************

.. include:: /includes/fota_upgrades_bt_mesh.txt

.. note::
   Point-to-point DFU over Bluetooth Low Energy is supported by default, out-of-the-box, for all samples and applications compatible with :zephyr:board:`thingy53`.
   See :ref:`thingy53_app_update` for more information about updating firmware image on :zephyr:board:`thingy53`.

FOTA in Matter
**************

See the `Matter Add-on`_ documentation for more information about FOTA in Matter.

FOTA over Thread
****************

.. include:: /includes/fota_upgrades_thread_zigbee.rst
   :start-after: fota_upgrades_thread_start
   :end-before: fota_upgrades_thread_end

FOTA over Zigbee
****************

.. include:: /includes/fota_upgrades_thread_zigbee.rst
   :start-after: fota_upgrades_zigbee_start
   :end-before: fota_upgrades_zigbee_end
