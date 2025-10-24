.. _bluetooth_channel_map_updates:

Bluetooth: Autonomous Channel Map Updates
##############################


The Channel Map Updates sample demonstrates an autonomous channel map update algorithm for Bluetooth® Low Energy connections using Quality of Service (QoS) events.
The algorithm automatically evaluates channel performance and generates optimized channel maps to improve connection reliability.

Requirements
************

The sample supports the following development kits:

See the supported boards listed in ``samples/bluetooth/channel_map_updates/sample.yaml``.

You need at least two development kits to physically test this sample:

* This sample currently supports channel map updates to one peripheral device. To demonstrate it, you need two development kits.

You can mix different development kits from the list above.

The sample also requires a connection to a computer with a serial terminal for each of the development kits.

Overview
********

The sample implements a channel map update algorithm that uses connection event reports to collect Quality of Service data.
It automatically evaluates channel performance based on CRC errors and RX timeouts, then generates optimized channel maps to improve connection reliability.

The sample can operate in two roles:

* **Central (tester)**: Initiates connections and collects QoS statistics.
* **Peripheral (peer)**: Accepts connections and participates in the channel evaluation process.

Algorithm Description
=====================

Rating Calculation
-------------------

The algorithm evaluates each channel after collecting a configurable number of samples using the following formula:

.. code-block:: text

   rating = (1.0f - w_3) * prev_rating + w_3 * (1.0f - (w_1 * crc_error_rate + w_2 * rx_timeout_rate));

Where:

* ``w_1``: Weight for CRC errors
* ``w_2``: Weight for RX timeouts
* ``w_3``: Weight for old rating
* ``crc_error_rate``: ``crc_errors / packets_sent``
* ``rx_timeout_rate``: ``rx_timeouts / packets_sent``
* ``prev_rating``: The rating from the previous evaluation

Channel Map Generation
----------------------

Channels are included in the channel map if and only if:

1. Their rating is above a configurable threshold (default: 0.8).
2. The number of active channels is greater than the minimum configurable amount (default: 3), to prevent failure of the adaptive channel hopping algorithm.

Key Features
============

* **Autonomous Operation**: Automatically collects QoS data and updates channel maps.
* **Configurable Parameters**: Adjustable weights and thresholds for different environments.
* **Rating System**: Each channel gets a performance rating based on CRC errors and RX timeouts.
* **Minimum Channel Guarantee**: Ensures a minimum number of active channels for connectivity.

User interface
**************

The sample uses a console interface to select the device role:

* Type ``c`` to set the device as central (tester)
* Type ``p`` to set the device as peripheral (peer)

Building and running
********************

Sample path: ``samples/bluetooth/channel_map_updates``

For physical DKs: Build and flash the sample for two development kits (one acting as central, one as peripheral) using your usual nRF Connect SDK workflow (for example, with ``west build`` and ``west flash``). Refer to the nRF Connect SDK documentation for detailed build and flashing instructions.

For BabbleSim: Build the sample then go to the testing steps below:

.. code-block:: bash

   west build -b nrf54l15bsim/nrf54l15/cpuapp


Testing
=======

Testing Physical DKs:
*********************

After programming the sample to both development kits, complete the following steps to test it:

1. Connect a terminal emulator to both development kits.
2. Reset both kits.
3. On the first development kit, type ``c`` in the terminal to set it as central (tester).
4. On the second development kit, type ``p`` in the terminal to set it as peripheral (peer).
5. Observe that the kits establish a connection. The central will start gathering QoS statistics.
6. After the evaluation interval is reached (default: 2000 packets), the algorithm will run.
7. After a set number of evaluations (default: 5), an evaluation report is printed showing:

   On Central Unit
   * Channel statistics (packets sent, CRC errors, RX timeouts)
   * Channel ratings
   * Channel states (active/inactive)
   * Updated channel map application

   On Peripheral Unit
   * Prints the Channel Map if it has been updated from last iteration

Testing in Simulation:
**********************
1. Start two instances of the BabbleSim board (one acting as central, one as peripheral):

   .. code-block:: bash

      cd build/zephyr
      ./zephyr.exe -s=<simulator_name> -p=<phy_name> -d=<device_id>

   Replace the placeholders to match your BabbleSim setup.

2. In the first terminal, type ``c`` to set it as central (tester).
3. In the second terminal, type ``p`` to set it as peripheral (peer).
4. Start an interferer (optional), for example the burst interferer that is installed together with BabbleSim:

   .. code-block:: bash

      cd <babblesim_root>/tools/bsim/bin
      ./bs_device_2G4_burst_interf -s=<simulator_name> -p=<phy_name> -d=<device_id> -type=WLAN -centerfreq=2440 -power=10

   This starts a WLAN-like interference process centered at 2440 MHz with 10 dBm power.

5. Start the physical layer used by the devices:

   .. code-block:: bash

      cd <babblesim_root>/tools/bsim/bin
      ./bs_2G4_phy_v1 -s=<simulator_name> -p=<phy_name> -D=<total_devices> -sim_length=10e8 -channel=Indoorv1 -defmodem=BLE_simple

   This starts a physical layer process with the Indoorv1 channel preset and the BLE_simple modem for more realistic packet transfer.

6. Observe that the devices establish a connection. The central will start gathering QoS statistics.
7. After the evaluation interval is reached (default: 2000 packets), the algorithm will run.
8. After a set number of evaluations (default: 5), an evaluation report is printed showing:

Sample output
=============

The result should look similar to the following output on the central device::


   *** Booting nRF Connect SDK v3.0.2-89ba1294ac9b ***
   *** Using Zephyr OS v4.0.99-f791c49f492c ***
   Starting Bluetooth Standard Connection Sample
   Starting Bluetooth Channel Map Update Sample
   I: SoftDevice Controller build revision: 
   I: 89 9a 50 8a 95 01 9c 58 |..P....X
   I: fc 39 d2 c1 10 04 ee 02 |.9......
   I: 64 ce 25 be             |d.%.    
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF54Lx (0x0005)
   I: Firmware: Standard Bluetooth controller (0x00) Version 137.20634 Build 261734951
   I: Bluetooth initialized

   I: Choose device role - type c (central) or p (peripheral): 
   I: Central. Starting scanning
   I: Channel map and algorithm initialized
   Algorithm will evaluate every 2000 packets
   Scanning successfully started

   Filters matched. Address: D2:A9:E7:9A:A7:72 (random) connectable: 1
   Connected as central
   Connection interval: 6 units (7.5 ms)
   Security changed: level 2, err: 0 
   Service discovery completed

   Channel map evaluation completed successfully
   Active channels: 33
   I: Applying new channel map
   I: Successfully applied channel map

   .
   .
   .

   I: 
   --- Algorithm Channel Report ---
   I: Ch | Total_packets_sent  | CRC_Errors | RX_Timeouts | Rating Made | Last Rating | State
   I: ---|---------------------|------------|-------------|-------------|-------------|-------
   I:  0 |                 661 |          0 |          11 |       0.986 |       0.985 | 1
   I:  1 |                 647 |          0 |          17 |       0.977 |       0.975 | 1
   I:  2 |                 632 |          0 |          11 |       0.986 |       0.989 | 1
   I:  3 |                 626 |          0 |          14 |       0.981 |       0.979 | 1
   I:  4 |                 658 |          0 |          12 |       0.985 |       0.986 | 1
   I:  5 |                 629 |          0 |          15 |       0.980 |       0.980 | 1
   I: ...
   I: 20 |                 281 |          4 |          82 |       0.780 |       1.000 | 0
   I: 21 |                 264 |          2 |         149 |       0.579 |       1.000 | 0
   I: 22 |                 283 |          3 |         240 |       0.368 |       1.000 | 0
   I: 23 |                 282 |          3 |         211 |       0.442 |       1.000 | 0
   I: 24 |                 272 |          0 |         132 |       0.639 |       1.000 | 0
   I: ...


The result should look similar to the following output on the peripheral device::

   *** Booting nRF Connect SDK v3.0.2-89ba1294ac9b ***
   *** Using Zephyr OS v4.0.99-f791c49f492c ***
   I: Starting Bluetooth Channel Map Update Sample

   I: SoftDevice Controller build revision: 
   I: 89 9a 50 8a 95 01 9c 58 |..P....X
   I: fc 39 d2 c1 10 04 ee 02 |.9......
   I: 64 ce 25 be             |d.%.    
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF54Lx (0x0005)
   I: Firmware: Standard Bluetooth controller (0x00) Version 137.20634 Build 2617349514
   I: Identity: D2:A9:E7:9A:A7:72 (random)
   I: HCI: version 6.0 (0x0e) revision 0x30f3, manufacturer 0x0059
   I: LMP: version 6.0 (0x0e) subver 0x30f3
   I: Bluetooth initialized

   I: Choose device role - type c (central) or p (peripheral): 
   I: Peripheral. Starting advertising

   Advertising successfully started
   Connected as peripheral
   Connection interval: 6 units (7.5 ms)
   Security changed: level 2, err: 0 
   Service discovery completed

   Detected Channel Map Update, (formatted CH36 down to CH0)
   LL channel map, HEX: 1f ff ff ff ff
   LL channel map, BITS: 1111111111111111111111111111111111111

   Detected Channel Map Update, (formatted CH36 down to CH0)
   LL channel map, HEX: 1f fb ff ff ff
   LL channel map, BITS: 1111111111011111111111111111111111111

   .
   .
   .

   Detected Channel Map Update, (formatted CH36 down to CH0)
   LL channel map, HEX: 1f c3 ff 5f ff
   LL channel map, BITS: 1111111000011111111110101111111111111

   Detected Channel Map Update, (formatted CH36 down to CH0)
   LL channel map, HEX: 1f c3 ff 4f ff
   LL channel map, BITS: 1111111000011111111110100111111111111
   

Dependencies
************

This sample uses the following nRF Connect SDK libraries:

* ``latency``
* ``latency_client``
* ``gatt_dm``
* ``nrf_bt_scan``

In addition, it uses the following Zephyr components and headers:

* ``include/zephyr/types.h``
* ``include/console.h``
* Kernel API (``include/kernel.h``)
* ``include/sys/printk.h``
* Bluetooth API (``include/bluetooth/bluetooth.h``, ``include/bluetooth/conn.h``, ``include/bluetooth/gatt.h``, ``include/bluetooth/hci.h``, ``include/bluetooth/uuid.h``)
* Logging API (``include/logging/log.h``)

