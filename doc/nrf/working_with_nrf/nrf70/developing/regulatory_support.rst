.. _ug_nrf70_developing_regulatory_support:

Operating with regulatory support
#################################

.. contents::
   :local:
   :depth: 2

The nRF70 Series devices operate in the license exempt 2.4 GHz and 5 GHz radio frequency spectrum bands.
However, in order to satisfy license exemption, the supported channels in each band need to adhere to regulatory operation rules.
The regulatory rules vary based on the country.
The rules are framed on the basis of the following key parameters:

* Channel scan mode (Active/Passive)
* Dynamic Frequency Selection (DFS) operation
* Maximum transmit power

Regulatory Database
*******************

The regulatory database used by the nRF70 Series captures regulatory rules in 173 different countries.
In addition, the regulatory database has an entry for a world regulatory domain, which has highly restrictive transmission rules making it suitable for worldwide operation.

The regulatory database is a part of the nRF70 Series ROM based firmware images.
You can configure the country of operation and based on this configuration, the appropriate rules of operation are enforced.

Supported 20 MHz channels
*************************

The nRF70 Series device supports up to 42 channels.

The device supports the following 14 channels in the 2.4 GHz band:

.. list-table::
   :header-rows: 1

   * - Channel
     - Start frequency
     - End frequency
     - Center frequency
   * - 1
     - 2401
     - 2423
     - 2412
   * - 2
     - 2406
     - 2428
     - 2417
   * - 3
     - 2411
     - 2433
     - 2422
   * - 4
     - 2416
     - 2438
     - 2427
   * - 5
     - 2421
     - 2443
     - 2432
   * - 6
     - 2426
     - 2448
     - 2437
   * - 7
     - 2431
     - 2453
     - 2442
   * - 8
     - 2436
     - 2458
     - 2447
   * - 9
     - 2441
     - 2463
     - 2452
   * - 10
     - 2446
     - 2468
     - 2457
   * - 11
     - 2451
     - 2473
     - 2462
   * - 12
     - 2456
     - 2478
     - 2467
   * - 13
     - 2461
     - 2483
     - 2472
   * - 14
     - 2473
     - 2495
     - 2484


Where the 5 GHz band is supported, the device supports the following 28 channels:

.. list-table::
   :header-rows: 1

   * - Channel
     - Start frequency
     - End frequency
     - Center frequency
     - Band
   * - 36
     - 5170
     - 5190
     - 5180
     - U-NII-1
   * - 40
     - 5190
     - 5210
     - 5200
     - U-NII-1
   * - 44
     - 5210
     - 5230
     - 5220
     - U-NII-1
   * - 48
     - 5230
     - 5250
     - 5240
     - U-NII-1
   * - 52
     - 5250
     - 5270
     - 5260
     - U-NII-2A
   * - 56
     - 5270
     - 5290
     - 5280
     - U-NII-2A
   * - 60
     - 5290
     - 5310
     - 5300
     - U-NII-2A
   * - 64
     - 5310
     - 5330
     - 5320
     - U-NII-2A
   * - 100
     - 5490
     - 5510
     - 5500
     - U-NII-2C
   * - 104
     - 5510
     - 5530
     - 5520
     - U-NII-2C
   * - 108
     - 5530
     - 5550
     - 5540
     - U-NII-2C
   * - 112
     - 5550
     - 5570
     - 5560
     - U-NII-2C
   * - 116
     - 5570
     - 5590
     - 5580
     - U-NII-2C
   * - 120
     - 5590
     - 5610
     - 5600
     - U-NII-2C
   * - 124
     - 5610
     - 5630
     - 5620
     - U-NII-2C
   * - 128
     - 5630
     - 5650
     - 5640
     - U-NII-2C
   * - 132
     - 5650
     - 5670
     - 5660
     - U-NII-2C
   * - 136
     - 5670
     - 5690
     - 5680
     - U-NII-2C
   * - 140
     - 5690
     - 5710
     - 5700
     - U-NII-2C
   * - 144
     - 5710
     - 5730
     - 5720
     - U-NII-2C
   * - 149
     - 5735
     - 5755
     - 5745
     - U-NII-3
   * - 153
     - 5755
     - 5775
     - 5765
     - U-NII-3
   * - 157
     - 5775
     - 5795
     - 5785
     - U-NII-3
   * - 161
     - 5795
     - 5815
     - 5805
     - U-NII-3
   * - 165
     - 5815
     - 5835
     - 5825
     - U-NII-3
   * - 169
     - 5835
     - 5855
     - 5845
     - U-NII-4
   * - 173
     - 5855
     - 5875
     - 5865
     - U-NII-4
   * - 177
     - 5875
     - 5895
     - 5885
     - U-NII-4

Conformance to regulatory restrictions
**************************************

Once a regulatory domain is configured, the following restrictions are imposed on the operation of the nRF70 Series device:

1. Channels: The device will operate only on the channels allowed in that country.
#. Scan mode: The device will use Active scans where allowed and resort to Passive scans on channels where Active scans are not allowed.
#. DFS: DFS operation is enabled on the channel.
   In the connected state, the device will switch to another channel as directed by the Access Point (AP) in the event of radar detection.
#. Transmit power: The maximum transmit power on each channel is restricted to a value allowed for that channel in that country.

Without user configuration, the nRF70 Series device will default to operating with world regulatory rules and utilize regulatory hints extracted from the AP beacon packet in order to adjust to more appropriate regional rules.


Configuration options
*********************

You can configure the regulatory domain through build time or run time.

Build time
==========

Use the :kconfig:option:`CONFIG_NRF700X_REG_DOMAIN` Kconfig option to set the regulatory region.
The regulatory region will take a ISO/IEC alpha-2 country code for the country in which the device is expected to operate.
The beacon's regulatory region (if present) will be given higher precedence over the Kconfig option.

Run time
========

You can also set the regulatory domain using either an API or a shell command as shown in the following table:

.. list-table:: Wi-Fi regulatory domain network management APIs
   :header-rows: 1

   * - Network management APIs
     - Command
     - Description
     - Expected output
   * - | net_mgmt(NET_REQUEST_WIFI_CMD_REG_DOMAIN, <XY>, force=true)
       | (Set option 1)
     - ``wifi reg_domain <XY> -f``
     - Override country code regulatory hints
     - Wi-Fi regulatory domain is set to: <XY>
   * - | net_mgmt(NET_REQUEST_WIFI_CMD_REG_DOMAIN, <XY>)
       | (Set option 2)
     - ``wifi reg_domain <XY>``
     - Set regulatory domain using regulatory hints from the AP beacon, else set regulatory domain to <XY>
     - Wi-Fi regulatory domain is set based on AP beacon hints or to <XY>

If you set a specific regulatory region with ``force=true`` flag (Set option 1), the beacon's country or region information will be ignored, and the current region will be overridden with the set value.

In the automatic region setting, the beacon's regulatory region (if present) will be given higher precedence over the following scenarios:

* The set region during booting and Kconfig options.
* The user configured regulatory region (``force=false`` flag (Set option 2)).

Once the nRF70 Series device disconnects from the AP, the device restores the regulatory region to world regulatory (code: 00) if automatic mode was enabled.

Regulatory region query
***********************

You can get the configured regulatory domain using either an API or a shell command as shown in the table:

.. list-table:: Wi-Fi regulatory region query
   :header-rows: 1

   * - Network management APIs
     - Command
     - Description
     - Expected output
   * - | net_mgmt(NET_REQUEST_WIFI_CMD_REG_DOMAIN)
       | (Get option)
     - ``wifi reg_domain``
     - Get current regulatory domain setting
     - Wi-Fi regulatory domain is: <XY>
