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

* :kconfig:`CONFIG_BT_RPMSG`
* :kconfig:`CONFIG_NRF_802154_SER_HOST`
* :kconfig:`CONFIG_BT_RPMSG_NRF53`

You might need to adjust the Kconfig configuration of this sample to make it compatible with the peer application.
For example, :kconfig:`CONFIG_BT_MAX_CONN` must be equal to the maximum number of connections supported by the peer application.

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
In this way, the sample is built automatically as a child image when both :kconfig:`CONFIG_BT_RPMSG_NRF53` and :kconfig:`CONFIG_NRF_802154_SER_HOST` are enabled.

See :ref:`configure_application` for information about how to configure the sample.

For example, you can include the Multiprotocol RPMsg sample in a multi-image build by building the :ref:`sockets-echo-server-sample` sample for the nRF5340 application core and adding the following configuration files to your build as CMake options:

* ``overlay-802154.conf``
* ``overlay-bt.conf``

See :ref:`cmake_options` for instructions on how to add these options to your build.
To see an example of this multi-image build on the command line, run the following command:

.. parsed-literal::
   :class: highlight

   west build -b nrf5340dk_nrf5340_cpuapp -p -- -DOVERLAY_CONFIG="overlay-802154.conf;overlay-bt.conf"

However, it is also possible to build the sample as a stand-alone image.

.. include:: /includes/build_and_run.txt

Testing
*******

The testing methods for this sample depend on how it was built and programmed to the device.
For example, if you built the sample in a multi-image build containing also the :ref:`sockets-echo-server-sample` sample mentioned before, you can test it as follows:

1. Run the IEEE 802.15.4 variant of the :ref:`sockets-echo-client-sample` on a second development kit that supports IEEE 802.15.4.
#. |connect_terminal|
   As the nRF5340 DK has multiple UART instances, you must identify the correct port.
#. Observe that IPv6 packets are exchanged between the echo client and server over the IEEE 802.15.4 interface.
   You can use a smartphone to see that the nRF5340 device advertises over Bluetooth LE in parallel to the echo exchanges.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`softdevice_controller`
* :ref:`nrf_802154_sl`
* :ref:`mpsl`
