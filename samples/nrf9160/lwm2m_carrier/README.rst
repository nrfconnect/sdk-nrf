.. _lwm2m_carrier:

nRF9160: LwM2M carrier
######################

The LwM2M carrier sample demonstrates how to run the **LwM2M carrier** library (liblwm2m_carrier) in an application in order to connect to the operator LwM2M network.

Requirements
************

* The following development board:

  * |nRF9160DK|

* .. include:: /includes/spm.txt

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/lwm2m_carrier`

.. include:: /includes/build_and_run_nrf9160.txt

Testing
=======

After programming the sample and all prerequisites to the board, test it by performing the following steps:

1. Connect the USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the kit prints the following information::

        LWM2M Carrier library sample.
#. Observe that the application receives events from the **LwM2M carrier** library via the registered event handler.


Troubleshooting
===============

Bootstrapping can take several minutes.
This is expected and dependent on the availability of the LTE link.
During boostrap, several ``LWM2M_CARRIER_EVENT_CONNECT`` and ``LWM2M_CARRIER_EVENT_DISCONNECT`` events are printed.
This is expected and is part of the bootstrapping procedure.


Dependencies
************

This sample uses the following libraries:

From |NCS|
  * ``drivers/lte_link_control``
  * :ref:`at_cmd_readme`
  * :ref:`lib_download_client`

From nrfxlib
  * :ref:`nrfxlib:bsdlib`

From Zephyr
  * :ref:`MQTT <zephyr:mqtt_socket_interface>`

In addition, it uses the following samples:

From |NCS|
  * :ref:`secure_partition_manager`
