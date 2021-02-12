.. _lwm2m_carrier:

nRF9160: LwM2M carrier
######################

.. contents::
   :local:
   :depth: 2

The LwM2M carrier sample demonstrates how to run the :ref:`liblwm2m_carrier_readme` library in an application in order to connect to the operator LwM2M network.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
     :header: heading
     :rows: nrf9160dk_nrf9160ns

.. include:: /includes/spm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/lwm2m_carrier`

.. include:: /includes/build_and_run_nrf9160.txt

Testing
=======

After programming the sample and all prerequisites to the development kit, test it by performing the following steps:

1. Connect the USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the kit prints the following information::

        LWM2M Carrier library sample.
#. Observe that the application receives events from the **LwM2M carrier** library via the registered event handler.


Troubleshooting
===============

Bootstrapping can take several minutes.
This is expected and dependent on the availability of the LTE link.
During bootstrap, several :c:macro:`LWM2M_CARRIER_EVENT_CONNECTED` and :c:macro:`LWM2M_CARRIER_EVENT_DISCONNECTED` events are printed.
This is expected and is part of the bootstrapping procedure.
For more information, see the :ref:`lwm2m_events` and :ref:`lwm2m_msc` sections in the LwM2M carrier library documentation.

To completely restart and trigger a new bootstrap, the device must be erased and re-programmed, as mentioned in :ref:`lwm2m_app_int`.


Dependencies
************

This sample uses the following |NCS| libraries:

* |NCS| modules abstracted via the LwM2M carrier OS abstraction layer (:file:`lwm2m_os.h`)

  .. include:: /../../lib/bin/lwm2m_carrier/doc/app_integration.rst
    :start-after: lwm2m_osal_mod_list_start
    :end-before: lwm2m_osal_mod_list_end

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
