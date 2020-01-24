.. _nrf-nfc-system-off-sample:

NFC System OFF
##############

The NFC System OFF sample shows how to make the NFC Tag device wake up from the System OFF mode when it detects the NFC field.
The sample uses the :ref:`nfc`.

Overview
********

The sample starts with the initialization of the NFC Tag device.
The device then attempts to detect an external NFC field.
If the field is not detected within 3 seconds, the device goes into the System OFF mode.

The wake-up from the System OFF mode is done when an NFC field is detected.
The system is started and the NFC Tag device is reinitialized.
The tag can be read afterwards.

Once the NFC field is not within range anymore, the sample goes back to the System OFF mode.

Fully working NFC Tag is not a requirement for wake up by NFC field feature.
It is also possible to skip NFC Tag configuration and use NFCT peripheral registers directly.
In this case reader will not be able to read anything, but it will be able to wake up the system.
Following function shows how to do it.
It can be used instead of ``start_nfc()`` function from the sample.

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

Requirements
************

* One of the following development boards:

  * |nRF52840DK|
  * |nRF52DK|

* Smartphone or tablet with the NFC feature.


User interface
**************

LED 1:
   On when an NFC field is present within range.

LED 2:
   On when the system is on.


Building and running
********************

.. |sample path| replace:: :file:`samples/nfc/system_off`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your board, test it by performing the following steps:

1. Observe that LED 2 on the Tag device turns off after 3 seconds after the programming is complete.
   This indicates that the system is in the System OFF mode.
#. Make sure that the NFC feature is activated on the smartphone or tablet.
   Check the device documentation for information about how to enable the NFC feature.
#. With the smartphone or tablet, touch the NFC antenna on the NFC Tag device.
#. Observe that LED 2 on the Tag device is lit, followed by LED 1 shortly after that.
   Moreover, a ``Hello World!`` notification appears on the smartphone or tablet.
   The notification text is obtained from NFC.
#. Move the smartphone or tablet away from the NFC antenna.
   LED 1 turns off.
#. Observe that LED 2 on the Tag device turns off after 3 seconds.
   This indicates that system is in the System OFF mode again.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nfc_ndef_msg`
* :ref:`nfc_text`

In addition, it uses the Type 2 Tag library from nrfxlib:

* :ref:`nrfxlib:nfc_api_type2`

The sample uses the following Zephyr libraries:

* ``include/zephyr.h``
* ``include/device.h``
* ``include/power/power.h``
* :ref:`GPIO Interface <zephyr:api_peripherals>`
