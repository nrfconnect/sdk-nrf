.. _nrf-nfc-system-off-sample:

NFC: System OFF
###############

.. contents::
   :local:
   :depth: 2

The NFC System OFF sample shows how to make the NFC Tag device wake up from the System OFF mode when it detects the NFC field.
The sample uses the :ref:`lib_nfc_ndef`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample also requires a smartphone or tablet with the NFC feature.

Overview
********

The sample starts with the initialization of the NFC Tag device.
The device then attempts to detect an external NFC field.
If the field is not detected within three seconds, the device goes into the System OFF mode.

The device wakes up from the System OFF mode when an NFC field is detected.
The system is started and the NFC Tag device is reinitialized.
The tag can be read afterwards.

When the NFC field is not within range anymore, the device goes back to the System OFF mode.

You do not necessarily need a fully working NFC Tag for the wake-up.
You can use the NFCT peripheral registers directly.
In this case, the reader cannot read anything, but it can wake up the system.

When using the registers to wake up the device, replace :c:func:`start_nfc()` with the following function:

.. code-block:: c

  void field_sens_start(void)
  {
  	int key = irq_lock();
  	/* First check if NFCT is not already sensing. */
  	if ((NRF_NFCT->NFCTAGSTATE & NFCT_NFCTAGSTATE_NFCTAGSTATE_Msk)
  			== NFCT_NFCTAGSTATE_NFCTAGSTATE_Disabled) {
  		NRF_NFCT->TASKS_SENSE = 1;
  	}
  	irq_unlock(key);
  }

See the `System OFF mode`_ page in the nRF52840 Product Specification for more information.

User interface
**************

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      LED 1:
         Lit when an NFC field is present within range.

      LED 2:
         Lit when the system is on.

   .. group-tab:: nRF54 DKs

      LED 0:
         Lit when an NFC field is present within range.

      LED 1:
         Lit when the system is on.

Building and running
********************

.. |sample path| replace:: :file:`samples/nfc/system_off`

.. include:: /includes/build_and_run_ns.txt

.. include:: /includes/nRF54H20_erase_UICR.txt

.. note::
   |nfc_nfct_driver_note|

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

.. tabs::

   .. group-tab:: nRF52 and nRF53 DKs

      1. Observe that **LED 2** on the Tag device turns off three seconds after the programming has completed.
         This indicates that the system is in the System OFF mode.
      #. Make sure that the NFC feature is activated on the smartphone or tablet.
         Check the device documentation on how to enable the NFC feature.
      #. With the smartphone or tablet, touch the NFC antenna of the NFC Tag device.
      #. Observe that **LED 2** on the Tag device is lit, followed by **LED 1** shortly after that.
         Also a "Hello World!" notification appears on the smartphone or tablet.
         The notification text is obtained from NFC.
      #. Move the smartphone or tablet away from the NFC antenna.
         **LED 1** turns off.
      #. Observe that **LED 2** on the Tag device turns off after three seconds.
         This indicates that system is in the System OFF mode again.

   .. group-tab:: nRF54 DKs

      1. Observe that **LED 1** on the Tag device turns off three seconds after the programming has completed.
         This indicates that the system is in the System OFF mode.
      #. Make sure that the NFC feature is activated on the smartphone or tablet.
         Check the device documentation on how to enable the NFC feature.
      #. With the smartphone or tablet, touch the NFC antenna of the NFC Tag device.
      #. Observe that **LED 1** on the Tag device is lit, followed by **LED 0** shortly after that.
         Also a "Hello World!" notification appears on the smartphone or tablet.
         The notification text is obtained from NFC.
      #. Move the smartphone or tablet away from the NFC antenna.
         **LED 0** turns off.
      #. Observe that **LED 1** on the Tag device turns off after three seconds.
         This indicates that system is in the System OFF mode again.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nfc_ndef_msg`
* :ref:`nfc_text`

In addition, it uses the Type 2 Tag library from `sdk-nrfxlib`_:

* :ref:`nrfxlib:nfc_api_type2`

The sample uses the following Zephyr libraries:

* ``include/zephyr.h``
* ``include/device.h``
* ``include/power/power.h``
* :ref:`GPIO Interface <zephyr:api_peripherals>`

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
