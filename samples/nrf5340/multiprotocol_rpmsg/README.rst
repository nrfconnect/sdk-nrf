.. _multiprotocol-rpmsg-sample:

nRF5340: Multiprotocol RPMsg
############################

.. contents::
   :local:
   :depth: 2

This sample exposes the :ref:`softdevice_controller` and the IEEE 802.15.4 radio driver support to the nRF5340 application CPU, using RPMsg transport as part of `OpenAMP`_.

Overview
********

The sample is compatible with the HCI RPMsg driver provided by the |NCS| Bluetooth :ref:`bt_hci_drivers` core and with the nRF 802.15.4 Serialization Host module.

See the following configuration options for more information:

* :option:`CONFIG_BT_RPMSG`
* :option:`CONFIG_NRF_802154_SER_HOST`
* :option:`CONFIG_BT_RPMSG_NRF53`

You might need to adjust the Kconfig configuration of this sample to make it compatible with the peer application.
For example, :option:`CONFIG_BT_MAX_CONN` must be equal to the maximum number of connections supported by the peer application.

The following components have been disabled to make this sample energy-efficient:

* Serial console (in :file:`prj.conf`)
* Logger (in :file:`prj.conf`)

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpunet

The testing procedure of the sample also requires a second development kit that supports IEEE 802.15.4.

Building and Running
********************

.. |sample path| replace:: :file:`samples/nrf5340/multiprotocol_rpmsg`

You must program this sample to the nRF5340 network core.

The recommended way of building the sample is to use the multi-image feature of the build system.
In this way, the sample is built automatically as a child image when both :option:`CONFIG_BT_RPMSG_NRF53` and :option:`CONFIG_NRF_802154_SER_HOST` are enabled.

However, it is also possible to build the sample as a stand-alone image.

.. include:: /includes/build_and_run.txt

Testing
*******

|test_sample|

1. Go to the :file:`samples/net/sockets/echo_server` path in the Zephyr's sample directory
#. Run the following command:

   .. code-block:: console

      west build -b nrf5340dk_nrf5340_cpuapp -p -- -DOVERLAY_CONFIG="overlay-802154.conf;overlay-bt.conf"

   During the build, the :ref:`multiprotocol-rpmsg-sample` image will be automatically included.
#. Run this command to program both the application and network core firmwares to the nRF5340 SoC:

   .. code-block:: console

      west flash
#. Run the IEEE 802.15.4 variant of the :ref:`sockets-echo-client-sample` on a second development kit that supports IEEE 802.15.4.
#. |connect_terminal|
   As the nRF5340 DK has multiple UART instances, the correct port must be identified.
#. Observe that IPv6 packets are exchanged between the echo client and server over the IEEE 802.15.4 interface.
   You can use a smartphone to see that the nRF5340 device advertises over Bluetooth LE in parallel to the echo exchanges.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`softdevice_controller`
* :ref:`nrf_802154_sl`
* :ref:`mpsl`
