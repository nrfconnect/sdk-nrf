.. _ug_wifi_stack_configuration:

Wi-Fi stack configuration and performance
#########################################

.. contents::
   :local:
   :depth: 2

This guide provides information on the configuration of the Wi-Fi® stack and the nRF Wi-Fi driver's performance.

Zephyr OS factors
*****************
The following sections explain the factors that are applicable when using the Zephyr OS on the nRF5340 SoC.

CPU frequency
=============

The nRF5340 host has two operating frequencies, 64 MHz and 128 MHz, default frequency is 64 MHz.
For low power applications, it is recommended to use 64 MHz as the CPU frequency, but the Wi-Fi performance of the nRF70 Series device might be impacted.
For high performance applications, it is recommended to use 128 MHz as the CPU frequency.

.. _constrained_host_networking_stack:

Networking stack
================

The nRF Wi-Fi driver uses the Zephyr networking stack for Wi-Fi protocol implementation.
You can configure the networking stack to use different networking buffers and queue depths.
The nRF Wi-Fi driver can be configured to use different numbers of TX buffers and RX buffers based on the use case.
A separate configuration option is provided to configure the number of packets, and the number of buffers used by each packet can also be configured.

The following table explains the configuration options:

+------------------------------------------+-----------------------------+--------------------------------------------------------------------------------------------------------------------------+
|Configuration Option                      | Values                      | Description                                                                                                              |
+==========================================+=============================+==========================================================================================================================+
|:kconfig:option:`CONFIG_NET_PKT_TX_COUNT` | ``1`` - Unlimited           | Number of TX packets. This is the TX queue depth, higher depth saturates the Wi-Fi link but consumes more memory,        |
|                                          | (based on available memory) | lower depth reduces memory usage but does not saturate the Wi-Fi link leading to poor performance.                       |
+------------------------------------------+-----------------------------+--------------------------------------------------------------------------------------------------------------------------+
| :kconfig:option:`CONFIG_NET_PKT_RX_COUNT`| ``1`` - Unlimited           | Number of RX packets. This is the RX queue depth, higher depth can keep up with the RX traffic but consumes more memory, |
|                                          | (based on available memory) | lower depth reduces memory usage but does not keep up with the RX traffic leading to packet drops.                       |
+------------------------------------------+-----------------------------+--------------------------------------------------------------------------------------------------------------------------+
| :kconfig:option:`CONFIG_NET_BUF_TX_COUNT`| ``1`` - Unlimited           | Number of TX buffers. Typically for Wi-Fi, each packet has two buffers,                                                  |
|                                          | (based on available memory) | so this has to be twice the number of packets.                                                                           |
+------------------------------------------+-----------------------------+--------------------------------------------------------------------------------------------------------------------------+
| :kconfig:option:`CONFIG_NET_BUF_RX_COUNT`| ``1`` - Unlimited           | Number of RX buffers. Typically for Wi-Fi, each packet has one buffer,                                                   |
|                                          | (based on available memory) | so this has to be equal to the number of packets.                                                                        |
+------------------------------------------+-----------------------------+--------------------------------------------------------------------------------------------------------------------------+

.. _constrained_host_driver_memory_controls:

nRF Wi-Fi driver performance and memory fine-tuning controls
************************************************************

The nRF Wi-Fi driver provides the following software configurations to fine-tune memory and performance based on the use case:

.. list-table::
   :header-rows: 1

   * - Configuration Option
     - Values
     - Description
     - Impact
     - Purpose
   * - :kconfig:option:`CONFIG_WPA_SUPP`
     - ``y`` or ``n``
     - Enable or disable Wi-Fi Protected Access (WPA™) supplicant
     - Memory savings
     - This specifies the inclusion of the WPA supplicant module.
       Disabling this flag restricts the nRF Wi-Fi driver's functionality to STA scan only.
   * - :kconfig:option:`CONFIG_NRF70_AP_MODE`
     - ``y`` or ``n``
     - Enable or disable Access Point (AP) mode
     - Memory savings
     - This specifies the inclusion of the AP mode module.
       Disabling this flag restricts the nRF Wi-Fi driver's functionality to :term:`Station mode (STA)` only.
   * - :kconfig:option:`CONFIG_NRF70_P2P_MODE`
     - ``y`` or ``n``
     - Enable or disable Wi-Fi direct mode
     - Memory Savings
     - This specifies the inclusion of the P2P mode module.
       Disabling this flag restricts the nRF Wi-Fi driver's functionality to STA or AP mode only.
   * - :kconfig:option:`CONFIG_NRF70_MAX_TX_TOKENS`
     - ``5``, ``10``, ``11``, ``12``
     - Maximum number of TX tokens.
       These are distributed across all WMM® access categories (including a pool for all).
     - Performance tuning and Memory savings
     - This specifies the maximum number of TX tokens that can be used in the token bucket algorithm.
       More tokens imply more concurrent transmit opportunities for RPU but can lead to poor aggregation performance
       if the pipeline is not saturated. But to saturate the pipeline, a greater number of networking stack buffers,
       or queue depth, is required.
   * - :kconfig:option:`CONFIG_NRF70_MAX_TX_AGGREGATION`
     - ``1`` to ``Unlimited`` (based on available memory in nRF70 Series device)
     - Maximum number of frames that are coalesced into a single Wi-Fi frame (for example, MPDU's in an A-MPDU, or MSDU's in an A-MSDU).
       The coalescing greatly improves the throughput for small frames or under high traffic load.
     - Performance tuning and Memory savings
     - This specifies the maximum number of frames that can be coalesced into a single Wi-Fi frame.
       More frames imply more coalescing opportunities but can add latency to the TX path as we wait for more frames to arrive.
   * - :kconfig:option:`CONFIG_NRF70_RX_NUM_BUFS`
     - ``1`` to ``Unlimited`` (based on available memory in nRF70 Series device)
     - Number of RX buffers
     - Memory savings
     - This specifies the number of RX buffers that can be used by the nRF Wi-Fi driver.
       The number of buffers must be enough to keep up with the RX traffic, otherwise packets might be dropped.
   * - :kconfig:option:`CONFIG_NRF70_TX_MAX_DATA_SIZE`
     - ``64`` to ``1600``
     - Maximum TX data size
     - Memory savings
     - This specifies the maximum size of Wi-Fi protocol frames that can be transmitted.
       Large frame sizes imply more memory usage but can efficiently utilize the bandwidth.
       If the application does not need to send large frames, then this can be reduced to save memory.
   * - :kconfig:option:`CONFIG_NRF70_RX_MAX_DATA_SIZE`
     - ``64`` to ``1600``
     - Maximum RX data size
     - Memory savings
     - This controls the maximum size of the frames that can be received by the Wi-Fi protocol.
       Large frame sizes imply more memory usage but can efficiently utilize the bandwidth.
       If the application does not need to receive large frames, then this can be reduced to save memory.

The configuration options must be used in conjunction with the Zephyr networking stack configuration options to achieve the desired performance and memory usage.
These options form a staged pipeline all the way to the nRF70 Series chip, any change in one stage of the pipeline will impact the performance and memory usage of the next stage.
For example, solving bottleneck in one stage of the pipeline might lead to a bottleneck in the next stage.

.. _constrained_host_packet_memory:

nRF70 Series packet memory
**************************
The nRF70 Series device chipset has a special memory called the packet memory to store the Wi-Fi protocol frames for both TX and RX.
The various configuration options that control the size of the packet memory are listed below:

* :kconfig:option:`CONFIG_NRF70_TX_MAX_DATA_SIZE`
* :kconfig:option:`CONFIG_NRF70_RX_MAX_DATA_SIZE`
* :kconfig:option:`CONFIG_NRF70_MAX_TX_TOKENS`
* :kconfig:option:`CONFIG_NRF70_MAX_TX_AGGREGATION`
* :kconfig:option:`CONFIG_NRF70_RX_NUM_BUFS`

The packet memory is divided into two parts, one for TX and one for RX. The size of the TX packet memory is calculated as follows:

.. code-block:: none

   (CONFIG_NRF70_TX_MAX_DATA_SIZE + 52 ) * CONFIG_NRF70_MAX_TX_TOKENS * CONFIG_NRF70_MAX_TX_AGGREGATION

The size of the RX packet memory is calculated as follows:

.. code-block:: none

   CONFIG_NRF70_RX_MAX_DATA_SIZE * CONFIG_NRF70_RX_NUM_BUFS

The total packet memory size is calculated as follows:

.. code-block:: none

   (CONFIG_NRF70_TX_MAX_DATA_SIZE + 52 ) * CONFIG_NRF70_MAX_TX_TOKENS * CONFIG_NRF70_MAX_TX_AGGREGATION +
   CONFIG_NRF70_RX_MAX_DATA_SIZE * CONFIG_NRF70_RX_NUM_BUFS

There is a build time check to ensure that the total packet memory size does not exceed the available packet memory size in the nRF70 Series chip.

.. note::
   The ``52`` bytes in the above equations are the overhead bytes required by the nRF70 Series chip to store the headers and footers of the Wi-Fi protocol frames.

.. _constrained_host_usage_profiles:

Usage profiles
**************

The nRF Wi-Fi driver can be used in the following profiles (not an exhaustive list):

.. list-table::
   :header-rows: 1

   * - Features
     - Profile
     - Configuration Options
     - Use cases
     - Throughputs
   * - STA scan only
     - Scan only
     - ``CONFIG_WPA_SUPP=n``
       ``CONFIG_NRF70_AP_MODE=n``
       ``CONFIG_NRF70_P2P_MODE=n``
       ``CONFIG_NET_PKT_TX_COUNT=1``
       ``CONFIG_NET_PKT_RX_COUNT=1``
       ``CONFIG_NET_BUF_TX_COUNT=1``
       ``CONFIG_NET_BUF_RX_COUNT=1``
     - Location services
     - ``N/A``
   * - :abbr:`STA (Station)` mode
     - IoT devices
     - ``CONFIG_WPA_SUPP=y``
       ``CONFIG_NRF70_AP_MODE=n``
       ``CONFIG_NRF70_P2P_MODE=n``
       ``CONFIG_NET_PKT_TX_COUNT=6``
       ``CONFIG_NET_PKT_RX_COUNT=6``
       ``CONFIG_NET_BUF_TX_COUNT=12``
       ``CONFIG_NET_BUF_RX_COUNT=6``
       ``CONFIG_NRF70_RX_NUM_BUFS=6``
       ``CONFIG_NET_BUF_DATA_SIZE=800``
       ``CONFIG_HEAP_MEM_POOL_SIZE=230000``
       ``CONFIG_SPEED_OPTIMIZATIONS=y``
       ``CONFIG_NRF70_UTIL=n``
       ``CONFIG_NRF70_MAX_TX_AGGREGATION=1``
       ``CONFIG_NRF70_MAX_TX_TOKENS=5``
     - IoT devices
     - ``TCP-TX: 5.2 Mbps``
       ``TCP-RX: 3.4 Mbps``
       ``UDP-TX: 5.5 Mbps``
       ``UDP-RX: 4.1 Mbps``
   * - :abbr:`STA (Station)` mode
     - Memory optimized :abbr:`STA (Station)` mode
     - ``CONFIG_WPA_SUPP=y``
       ``CONFIG_NRF70_AP_MODE=n``
       ``CONFIG_NRF70_P2P_MODE=n``
       ``CONFIG_NET_PKT_TX_COUNT=6``
       ``CONFIG_NET_PKT_RX_COUNT=6``
       ``CONFIG_NET_BUF_TX_COUNT=12``
       ``CONFIG_NET_BUF_RX_COUNT=6``
       ``CONFIG_NRF70_RX_NUM_BUFS=6``
       ``CONFIG_NET_BUF_DATA_SIZE=500``
       ``CONFIG_HEAP_MEM_POOL_SIZE=230000``
       ``CONFIG_SPEED_OPTIMIZATIONS=y``
       ``CONFIG_NRF70_UTIL=n``
       ``CONFIG_NRF70_MAX_TX_AGGREGATION=1``
       ``CONFIG_NRF70_MAX_TX_TOKENS=5``
     - Sensors with low data requirements
     - ``TCP-TX: 0.3 Mbps``
       ``TCP-RX: 1.5 Mbps``
       ``UDP-TX: 5.1 Mbps``
       ``UDP-RX: 0.5 Mbps``
   * - :abbr:`STA (Station)` mode
     - High performance :abbr:`STA (Station)` mode
     - ``CONFIG_WPA_SUPP=y``
       ``CONFIG_NRF70_AP_MODE=n``
       ``CONFIG_NRF70_P2P_MODE=n``
       ``CONFIG_NET_PKT_TX_COUNT=30``
       ``CONFIG_NET_PKT_RX_COUNT=30``
       ``CONFIG_NET_BUF_TX_COUNT=60``
       ``CONFIG_NET_BUF_RX_COUNT=30``
       ``CONFIG_NET_BUF_DATA_SIZE=1100``
       ``CONFIG_HEAP_MEM_POOL_SIZE=230000``
       ``CONFIG_SPEED_OPTIMIZATIONS=y``
       ``CONFIG_NRF70_UTIL=n``
       ``CONFIG_NRF70_MAX_TX_AGGREGATION=9``
       ``CONFIG_NRF70_MAX_TX_TOKENS=12``
     - High data rate IoT devices
     - ``TCP-TX: 14.2 Mbps``
       ``TCP-RX: 7.4  Mbps``
       ``UDP-TX: 26.2 Mbps``
       ``UDP-RX: 12.4 Mbps``
   * - :abbr:`STA (Station)` mode
     - TX prioritized :abbr:`STA (Station)` mode
     - ``CONFIG_WPA_SUPP=y``
       ``CONFIG_NRF70_AP_MODE=n``
       ``CONFIG_NRF70_P2P_MODE=n``
       ``CONFIG_NET_PKT_TX_COUNT=32``
       ``CONFIG_NET_PKT_RX_COUNT=10``
       ``CONFIG_NET_BUF_TX_COUNT=64``
       ``CONFIG_NET_BUF_RX_COUNT=10``
       ``CONFIG_NRF70_RX_NUM_BUFS=10``
       ``CONFIG_NET_BUF_DATA_SIZE=1100``
       ``CONFIG_HEAP_MEM_POOL_SIZE=230000``
       ``CONFIG_SPEED_OPTIMIZATIONS=y``
       ``CONFIG_NRF70_UTIL=n``
       ``CONFIG_NRF70_MAX_TX_AGGREGATION=9``
       ``CONFIG_NRF70_MAX_TX_TOKENS=12``
     - Sensors with high data rate
     - ``TCP-TX: 9.2  Mbps``
       ``TCP-RX: 3.6  Mbps``
       ``UDP-TX: 26.6 Mbps``
       ``UDP-RX: 12.8 Mbps``
   * - :abbr:`STA (Station)` mode
     - RX prioritized :abbr:`STA (Station)` mode
     - ``CONFIG_WPA_SUPP=y``
       ``CONFIG_NRF70_AP_MODE=n``
       ``CONFIG_NRF70_P2P_MODE=n``
       ``CONFIG_NET_PKT_TX_COUNT=5``
       ``CONFIG_NET_PKT_RX_COUNT=64``
       ``CONFIG_NET_BUF_TX_COUNT=10``
       ``CONFIG_NET_BUF_RX_COUNT=64``
       ``CONFIG_NRF70_RX_NUM_BUFS=64``
       ``CONFIG_NET_BUF_DATA_SIZE=1100``
       ``CONFIG_HEAP_MEM_POOL_SIZE=230000``
       ``CONFIG_SPEED_OPTIMIZATIONS=y``
       ``CONFIG_NRF70_UTIL=n``
       ``CONFIG_NRF70_MAX_TX_AGGREGATION=2``
       ``CONFIG_NRF70_MAX_TX_TOKENS=5``
     - Display devices streaming data
     - ``TCP-TX: 5.3  Mbps``
       ``TCP-RX: 7.9  Mbps``
       ``UDP-TX: 8.6  Mbps``
       ``UDP-RX: 12.7 Mbps``

.. note::
   The measured throughputs, as shown in the table above, are based on tests conducted using the nRF7002 DK.
   The results represent the best throughput, averaged over three iterations, and were obtained with a good RSSI signal in a clean environment.

   The above configuration values can be passed when :ref:`configuring Kconfig options <configuring_kconfig>` before a build or by adding them in an :file:`overlay` file and passing to west build as CMake argument :makevar:`EXTRA_CONF_FILE` using the respective :ref:`CMake option <cmake_options>`.
