.. _bluetooth_isochronous_time_synchronization:

Bluetooth: ISO time synchronization
###################################

.. contents::
   :local:
   :depth: 2

The Bluetooth® isochronous time synchronization sample showcases time-synchronized processing of isochronous data.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also supports other development kits running SoftDevice Controller variants that support isochronous channels (see :ref:`SoftDevice Controller feature support <softdevice_controller>`).
To demonstrate time synchronization between the transmitter and a receiver, you need two development kits.
To demonstrate time synchronization between the transmitter and multiple receivers, you need at least three development kits.
You can use any combination of the development kits mentioned above, mixing different development kits.

Additionally, the sample requires a connection to a computer with a serial terminal for each development kit in use.

To observe the toggling of the LEDs, use a logic analyzer or an oscilloscope.

Overview
********

The sample demonstrates the following features:

 * How to send isochronous data on multiple streams so that the receiving devices and the sending device can process the data at the same time.
 * How the receivers of the isochronous data can process the received data synchronously.
 * How to provide the isochronous data to the controller right before it is sent on-air.
 * How to achieve this for either broadcast (BIS) or over connections (CIS).

This sample configures a single device as a transmitter of its **BUTTON1** state.
The transmitting and receiving devices toggle **LED1** synchronously with the accuracy of a few microseconds.
When the :ref:`CONFIG_LED_TOGGLE_IMMEDIATELY_ON_SEND_OR_RECEIVE <CONFIG_LED_TOGGLE_IMMEDIATELY_ON_SEND_OR_RECEIVE>` Kconfig option is enabled, **LED2** is toggled upon sending or receiving data.
This allows you to measure the minimal end-to-end latency.

.. note::
   This sample requires less hardware resources when it is run on an nRF54L Series device compared to the nRF52 or nRF53 Series devices.
   On an nRF54L Series device, only one GRTC channel and PPI channel is needed to set up accurate toggling of an LED.
   On nRF52 and nRF53 Series devices, you also need one RTC peripheral, one TIMER peripheral, one EGU channel, three PPI channels, and one PPI group.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following Kconfig options:

.. _CONFIG_LED_TOGGLE_IMMEDIATELY_ON_SEND_OR_RECEIVE:

CONFIG_LED_TOGGLE_IMMEDIATELY_ON_SEND_OR_RECEIVE
   This configuration option enables immediate toggling of **LED2** when isochronous data is sent or received.
   It allows for measurement of the minimum end-to-end latency.

.. _CONFIG_SDU_INTERVAL_US:

CONFIG_SDU_INTERVAL_US
   This configuration option determines how often the button value is transmitted to the receiving devices.

.. _CONFIG_TIMED_LED_PRESENTATION_DELAY_US:

CONFIG_TIMED_LED_PRESENTATION_DELAY_US
   This configuration option represents the time from the SDU synchronization reference until the value is applied.
   The application needs to ensure it can process the received SDU within this amount of time.
   The end-to-end latency will be at least the sum of the configured transport latency and presentation delay.

Isochronous channels
********************

The Bluetooth LE Isochronous Channels feature is a way of using Bluetooth LE to transfer time-bounded data between devices.
It provides a mechanism that makes sure that multiple sink devices, receiving data from the same source, render it at the same time.
Data has a time-limited validity period, at the end of which it is said to expire.
Expired data that has not yet been transmitted, will be discarded.
This means that receiver devices only ever receive data that is valid with respect to rules regarding its age and acceptable latency.

The key concepts of isochronous channels are the following:

Topology
    Isochronous channels can either be used for one-to-many (Broadcast) or one-to-one (Connected).
    For both topologies, data may be processed simultaneously or time-synchronized on all transmitting and receiving devices.
    Connected isochronous streams can either transfer data from central to peripheral, peripheral to central, or in both directions.

SDU interval and size
    The SDU interval specifies how often a service data unit (SDU) is transferred.
    The SDU interval and size as well as the number of streams are limited by the bandwidth of the link layer.
    In this sample, the SDU interval is configured with the :ref:`CONFIG_SDU_INTERVAL_US <CONFIG_SDU_INTERVAL_US>` Kconfig option.

Maximum transport latency and retransmission count
    Upon setting up an isochronous stream as a central or a broadcaster, the application provides a maximum transport latency and retransmission count.
    The Bluetooth controller uses the provided parameters to select link layer parameters that satisfy these constraints.
    This involves selecting an isochronous interval, PDU sizes, and isochronous parameters, such as burst number (BN), number of subevents (NSE), and flush timeout (FT) or pretransmission offset (PTO).
    For more details about how the SoftDevice Controller selects these parameters, see :ref:`Parameter selection <iso_parameter_selection>`.

Presentation delay
    The receiver(s) of isochronous data may receive their data at different points in time, but need to render or present it synchronously.
    Presentation delay represents the amount of time that is added to the received timestamp to obtain the presentation time of the data.
    In this sample, the presentation delay is configured with the :ref:`CONFIG_TIMED_LED_PRESENTATION_DELAY_US <CONFIG_TIMED_LED_PRESENTATION_DELAY_US>` Kconfig option.

For more details about this feature, see `Bluetooth Core Specification Version 5.2 Feature Overview <https://www.bluetooth.com/wp-content/uploads/2020/01/Bluetooth_5.2_Feature_Overview.pdf>`_.

This sample demonstrates the following:

* Any of the possible isochronous topologies with unidirectional data transfer.
  The topology is selected when the application starts.
* The transmitter of the data sends it over all configured and connected streams.
  The number of streams is configured with the :kconfig:option:`CONFIG_BT_ISO_MAX_CHAN` Kconfig option.
* The receiver of the isochronous data receives data from one transmitter only.
* The maximum transport latency and retransmission count is configured when the application starts.

Sample structure
****************

The sample code is divided into multiple source files, which makes it easier to compile in or out certain functionalities.

``main.c``
  The main entry point of the sample allows you to select the role and parameters to be used.

``bis_transmitter.c``
  This file implements a device that sets up a BIS transmitter with a configured number of streams.
  That is:

  1. Configure and start an extended advertiser.
     The extended advertiser data contains the device name.
  #. Configure and start a periodic advertiser.
  #. Configure and start a broadcast isochronous group (BIG).

``bis_receiver.c``
  This file implements a device that syncs to a BIS broadcaster with a given BIS index.

  1. Scan for extended advertisers containing the device name.
  #. Synchronize to the corresponding periodic advertiser.
  #. Synchronize to the selected BIS.

  When the ISO channel is disconnected, it tries to sync to it again.

``cis_central.c``
  This file implements a device that acts as a CIS central.
  The central can be configured either as a transmitter or a receiver.

  1. Configure a connected isochronous group (CIG).
  #. Scan and connect to a device that contains the device name.
  #. Set up a CIS to the connected device.
  #. Continue to scanning for more peripherals, if there are more available isochronous streams.

``cis_peripheral.c``
  This file implements a device that acts as a CIS peripheral.
  The peripheral can be configured either as a transmitter or a receiver.

  1. Set up an isochronous server so that it can later on accept the setup or a CIS.
  #. Start advertising its device name.
  #. Once a device has connected, accept the incoming CIS request.

``iso_rx.c``
  This file handles receiving of isochronous data.
  Once an isochronous stream is connected, the isochronous parameters are printed.

  When a valid SDU is received, the following operations are performed:

  * If the :ref:`CONFIG_LED_TOGGLE_IMMEDIATELY_ON_SEND_OR_RECEIVE <CONFIG_LED_TOGGLE_IMMEDIATELY_ON_SEND_OR_RECEIVE>` Kconfig option is enabled, **LED2** is toggled immediately.
    You can use this to observe that different receivers may receive the SDU at different points in time.
  * A timer trigger is configured to toggle **LED1** :ref:`CONFIG_TIMED_LED_PRESENTATION_DELAY_US <CONFIG_TIMED_LED_PRESENTATION_DELAY_US>` after the received timestamp.
    This ensures that all receivers and the transmitter toggle it at the same time.

``iso_tx.c``
  This file handles time-synchronized transmitting of isochronous data on all established streams.
  Each SDU contains a counter and the current value of **BUTTON1**.
  The implementation ensures the following:

  * The SDU is transmitted right before it is supposed to be sent on air.
    This is achieved by setting up a timer to trigger right before the next TX timestamp.
  * The SDU is transmitted on all the established isochronous channels with the same timestamp.
    This ensures that all the receivers can toggle their corresponding LEDs at the same time.
    The very first SDU is provided without a timestamp, because the timestamp is not known at this point in time.
  * **LED1** is configured to toggle synchronously with the **LED1** on all the receivers.
    The toggle time is determined by the TX timestamp and defined relative to the synchronization reference on the receiver.
  * If the :ref:`CONFIG_LED_TOGGLE_IMMEDIATELY_ON_SEND_OR_RECEIVE <CONFIG_LED_TOGGLE_IMMEDIATELY_ON_SEND_OR_RECEIVE>` Kconfig option is enabled, **LED2** is toggled right before sending the SDU.
    You can use this to observe the end-to-end latency.

``timed_led_toggle.c``
  This file implements timed toggling a LED.
  The ``GPIOTE`` peripheral is used together with ``PPI`` to achieve accurate timing.

``controller_time_<device>.c``
  The SDU timestamps sent to and received from the controller are based upon the controller clock.
  These files allow the application to read the current timestamp and set up a PPI trigger at a given point in time.

  The implementation for nRF52 and nRF53 Series devices is implemented by shadowing an RTC peripheral combined with a timer peripheral.
  The implementation for nRF54L Series devices uses the GRTC and is simpler to use.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/iso_time_sync`

.. include:: /includes/build_and_run.txt

Testing
*******

The sample can demonstrate broadcast isochronous streams or connected isochronous streams.

Testing broadcast isochronous streams
=====================================

After programming the sample to the development kits, perform the following steps to test it:

1. |connect_terminal_specific|
#. Reset the kits.
#. Configure one of the devices as an isochronous broadcaster by typing either ``b`` or ``c`` in the terminal emulator.
#. Configure the number of retransmissions and maximum transport latency.
#. Observe that the broadcaster starts and begins to transmit SDUs.
#. On the other terminal(s), type ``r`` to configure the device(s) as receivers of the broadcast isochronous stream.
#. Select which BIS the receiver should synchronize to.
#. Observe that the device(s) synchronize to the broadcaster and start receiving isochronous data.
#. Press **BUTTON1** on the broadcaster.
#. Observe that **LED1** toggles on both the broadcaster and the receivers.

Testing connected isochronous streams
=====================================

After programming the sample to the development kits, perform the following steps to test it:

1. |connect_terminal_specific|
#. Reset the kits.
#. Configure one of the devices as a connected isochronous stream central by typing ``c`` in the terminal emulator.
#. Configure the number of retransmissions and the maximum transport latency.
#. Select data direction.
   If the central is configured for transmission, it connects to multiple peripherals.
#. On the other terminal(s), type ``p`` to configure the device(s) as connected isochronous stream peripheral(s).
#. Select data direction.
#. Observe that the peripheral(s) connect to the central and start receiving isochronous data.
#. Press **BUTTON1** on the central device.
#. Observe that **LED1** toggles on both the central and peripheral devices.

Observe time-synchronized ISO data processing
=============================================

When you press **BUTTON1** on the transmitting device, you can observe that **LED1** toggles simultaneously on all devices.
To observe the accurate toggling, use a logic analyzer or an oscilloscope.

Sample output
*************

The result should look similar to the following output:

* For the isochronous broadcaster::

   Bluetooth ISO Time Sync Demo
   Choose role - cis_central (c) / cis_peripheral (p) / bis_transmitter (b) / bis_receiver (r) : b
   Choose retransmission number [0..30] : 2
   Choose max transport latency in ms [5..4000] : 20
   Starting BIS transmitter with BIS indices [1..4], RTN: 2, max transport latency 20 ms
   BIS transmitter started
   ISO channel index 0 connected: interval: 10 ms, NSE: 3, BN: 1, IRC: 2, PTO: 1, transport latency: 12418 us
   ISO channel index 1 connected: interval: 10 ms, NSE: 3, BN: 1, IRC: 2, PTO: 1, transport latency: 12418 us
   ISO channel index 2 connected: interval: 10 ms, NSE: 3, BN: 1, IRC: 2, PTO: 1, transport latency: 12418 us
   ISO channel index 3 connected: interval: 10 ms, NSE: 3, BN: 1, IRC: 2, PTO: 1, transport latency: 12418 us
   Sent SDU, counter: 0, btn_val: 0, LED will be set in 30264 us
   Sent SDU, counter: 100, btn_val: 0, LED will be set in 22177 us
   Sent SDU, counter: 200, btn_val: 0, LED will be set in 22177 us
   Sent SDU, counter: 300, btn_val: 0, LED will be set in 22177 us
   Sent SDU, counter: 400, btn_val: 0, LED will be set in 22208 us
   Sent SDU, counter: 500, btn_val: 0, LED will be set in 22208 us

* For the isochronous broadcast receiver::

   Bluetooth ISO Time Sync Demo
   Choose role - cis_central (c) / cis_peripheral (p) / bis_transmitter (b) / bis_receiver (r) : r
   Choose bis index [1..31] : 2
   Starting BIS receiver, BIS index 2
   Scanning for periodic advertiser
   Waiting for BigInfo
   Synced to periodic advertiser
   BigInfo received
   Syncing to BIG index 2
   ISO Channel connected: interval: 10 ms, NSE: 3, BN: 1, IRC: 2, PTO: 1, transport latency: 12418 us
   Received SDU with counter: 51500, btn_val: 0, LED will be set in 21035 us
   Received SDU with counter: 51600, btn_val: 0, LED will be set in 21044 us
   Received SDU with counter: 51700, btn_val: 0, LED will be set in 21024 us
   Received SDU with counter: 51800, btn_val: 0, LED will be set in 21034 us
   Received SDU with counter: 51900, btn_val: 0, LED will be set in 21045 us
   Received SDU with counter: 52000, btn_val: 0, LED will be set in 21024 us

* For the connected isochronous stream central configured for transmission::

   Bluetooth ISO Time Sync Demo
   Choose role - cis_central (c) / cis_peripheral (p) / bis_transmitter (b) / bis_receiver (r) : c
   Choose direction - TX (t) / RX (r) : t
   Choose retransmission number [0..30] : 3
   Choose max transport latency in ms [5..4000] : 20
   Starting CIS peripheral, dir: tx, RTN: 3, max transport latency 20 ms
   CIS central started scanning for peripheral(s)
   Connected: FA:BB:79:57:D6:45 (random)
   Connecting ISO channel
   Continue scanning for more peripherals...
   ISO channel index 0 connected: interval: 10 ms, NSE: 2, BN: 1, FT: 2, transport latency: 13696 us
   Sent SDU, counter: 0, btn_val: 0, LED will be set in 30854 us
   Sent SDU, counter: 100, btn_val: 0, LED will be set in 24232 us
   Sent SDU, counter: 200, btn_val: 0, LED will be set in 24232 us
   Sent SDU, counter: 300, btn_val: 0, LED will be set in 24232 us
   Sent SDU, counter: 400, btn_val: 0, LED will be set in 24232 us
   Sent SDU, counter: 500, btn_val: 0, LED will be set in 24232 us

* For the connected isochronous stream peripheral configured for reception::

   Bluetooth ISO Time Sync Demo
   Choose role - cis_central (c) / cis_peripheral (p) / bis_transmitter (b) / bis_receiver (r) : p
   Choose direction - TX (t) / RX (r) : r
   Starting CIS peripheral, dir: rx
   CIS peripheral started advertising
   Connected: E8:DC:8D:B3:47:69 (random)
   Incoming request from 0x20002440
   ISO Channel connected: interval: 10 ms, NSE: 2, BN: 1, FT: 2, transport latency: 13696 us
   Received SDU with counter: 0, btn_val: 0, LED will be set in 20237 us
   Received SDU with counter: 100, btn_val: 0, LED will be set in 20715 us
   Received SDU with counter: 200, btn_val: 0, LED will be set in 20705 us
   Received SDU with counter: 300, btn_val: 0, LED will be set in 20696 us
   Received SDU with counter: 400, btn_val: 0, LED will be set in 20715 us
   Received SDU with counter: 500, btn_val: 0, LED will be set in 20706 us

* For the connected isochronous stream central configured for reception::

   Bluetooth ISO Time Sync Demo
   Choose role - cis_central (c) / cis_peripheral (p) / bis_transmitter (b) / bis_receiver (r) : c
   Choose direction - TX (t) / RX (r) : r
   Choose retransmission number [0..30] : 3
   Choose max transport latency in ms [5..4000] : 20
   Starting CIS peripheral, dir: rx, RTN: 3, max transport latency 20 ms
   CIS central started scanning for peripheral(s)
   Connected: FA:BB:79:57:D6:45 (random)
   Connecting ISO channel
   ISO Channel connected: interval: 10 ms, NSE: 2, BN: 1, FT: 2, transport latency: 10924 us
   Received SDU with counter: 0, btn_val: 0, LED will be set in 9493 us
   Received SDU with counter: 100, btn_val: 0, LED will be set in 9493 us
   Received SDU with counter: 200, btn_val: 0, LED will be set in 9493 us
   Received SDU with counter: 300, btn_val: 0, LED will be set in 9493 us
   Received SDU with counter: 400, btn_val: 0, LED will be set in 9493 us
   Received SDU with counter: 500, btn_val: 0, LED will be set in 9493 us

* For the connected isochronous stream peripheral configured for transmission::

   Bluetooth ISO Time Sync Demo
   Choose role - cis_central (c) / cis_peripheral (p) / bis_transmitter (b) / bis_receiver (r) : p
   Choose direction - TX (t) / RX (r) : t
   Starting CIS peripheral, dir: tx
   CIS peripheral started advertising
   Connected: E8:DC:8D:B3:47:69 (random)
   Incoming request from 0x20002440
   ISO channel index 0 connected: interval: 10 ms, NSE: 2, BN: 1, FT: 2, transport latency: 10924 us
   Sent SDU, counter: 0, btn_val: 0, LED will be set in 20308 us
   Sent SDU, counter: 100, btn_val: 0, LED will be set in 10470 us
   Sent SDU, counter: 200, btn_val: 0, LED will be set in 10460 us
   Sent SDU, counter: 300, btn_val: 0, LED will be set in 10450 us
   Sent SDU, counter: 400, btn_val: 0, LED will be set in 10470 us
   Sent SDU, counter: 500, btn_val: 0, LED will be set in 10460 us

Dependencies
*************

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:softdevice_controller`

In addition, it uses the following Zephyr libraries:

* :file:`include/console.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :file:`include/sys/printk.h`
* :file:`include/zephyr/types.h`
* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/iso.h`
  * :file:`include/bluetooth/conn.h`
  * :file:`include/bluetooth/hci.h`
  * :file:`include/bluetooth/scan.h`

References
***********

For more information about how to use isochronous channels with the SoftDevice Controller, see :ref:`Isochronous channels <softdevice_controller_iso>`.
