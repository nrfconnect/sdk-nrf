.. _ug_wifi_band_edge_compensation:

Band edge compensation
######################

.. contents::
   :local:
   :depth: 2

Use backoff to reduce the transmit power on the band edge channels to be within regulatory limits.

In general, a device transmits at the maximum possible power for a given data rate.
When using this maximum power for band edge channels, the regulatory requirements for :term:`Spectral Emission Mask (SEM)` might be violated.
For such cases, the transmit power is reduced by a particular value called backoff to meet SEM requirements.
The backoff is applied only for edge channels of a frequency band defined by the regulatory domain.

The following table shows examples of frequency ranges for the U.S. regulatory domain.

+----------------+-----------------------+
| Band           | Frequency range (MHz) |
+================+=======================+
| 2.4 GHz band   | 2400 to 2472          |
+----------------+-----------------------+
| UNII-1         | 5150 to 5250          |
+----------------+-----------------------+
| UNII-2A        | 5250 to 5350          |
+----------------+-----------------------+
| UNII-2C        | 5470 to 5730          |
+----------------+-----------------------+
| UNII-3         | 5730 to 5850          |
+----------------+-----------------------+
| UNII-4         | 5850 to 5895          |
+----------------+-----------------------+

When applying backoff, consider the following:

* A channel is considered an edge channel if the difference between the edge frequency of a band and the center frequency of the channel is 15 MHz or less.
* In the U.S. regulatory domain, devices operating on channel 11 of the 2.4 GHz band (2462 MHz) must apply backoff as channel 11 is at the edge for this frequency band.
* For world regulatory domain, channel 14 is the edge channel as the frequency range for world regulatory domain is 2400 MHz to 2494 MHz.
* Backoff values also vary based on the frame type, affecting the maximum transmit power. Different backoff values are configured for each frame type.

Backoff values, which are expressed in dB, are inputs to the Wi-Fi® firmware driver through Kconfig parameters.
The Kconfig parameters are defined in the :file:`<ncs_repo>/zephyr/drivers/wifi/nrf_wifi/Kconfig.nrfwifi` file.

Set the Kconfig parameter values (which are positive numbers) according to the required backoff in the specific frequency band for the regulatory domain.
The default value of the Kconfig parameter is zero.

The following tables show the Kconfig parameters for lower and upper edge backoff.

+---------+-----------+---------------------------------------------------------------+
| Band    | Frame type| Kconfig parameter for lower edge backoff                      |
+=========+===========+===============================================================+
| 2.4 GHz | DSSS      | CONFIG_NRF70_BAND_2G_LOWER_EDGE_BACKOFF_DSSS                  |
+         +-----------+---------------------------------------------------------------+
|         | HT/V HT   | CONFIG_NRF70_BAND_2G_LOWER_EDGE_BACKOFF_HT                    |
+         +-----------+---------------------------------------------------------------+
|         | HE        | CONFIG_NRF70_BAND_2G_LOWER_EDGE_BACKOFF_HE                    |
+---------+-----------+---------------------------------------------------------------+
| UNII-1  | HT/V HT   | CONFIG_NRF70_BAND_UNII_1_LOWER_EDGE_BACKOFF_HT                |
+         +-----------+---------------------------------------------------------------+
|         | HE        | CONFIG_NRF70_BAND_UNII_1_LOWER_EDGE_BACKOFF_HE                |
+---------+-----------+---------------------------------------------------------------+
| UNII-2A | HT/V HT   | CONFIG_NRF70_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HT               |
+         +-----------+---------------------------------------------------------------+
|         | HE        | CONFIG_NRF70_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HE               |
+---------+-----------+---------------------------------------------------------------+
| UNII-2C | HT/V HT   | CONFIG_NRF70_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HT               |
+         +-----------+---------------------------------------------------------------+
|         | HE        | CONFIG_NRF70_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HE               |
+---------+-----------+---------------------------------------------------------------+
| UNII-3  | HT/V HT   | CONFIG_NRF70_BAND_UNII_3_LOWER_EDGE_BACKOFF_HT                |
+         +-----------+---------------------------------------------------------------+
|         | HE        | CONFIG_NRF70_BAND_UNII_3_LOWER_EDGE_BACKOFF_HE                |
+---------+-----------+---------------------------------------------------------------+
| UNII-4  | HT/V HT   | CONFIG_NRF70_BAND_UNII_4_LOWER_EDGE_BACKOFF_HT                |
+         +-----------+---------------------------------------------------------------+
|         | HE        | CONFIG_NRF70_BAND_UNII_4_LOWER_EDGE_BACKOFF_HE                |
+---------+-----------+---------------------------------------------------------------+

+---------+-----------+---------------------------------------------------------------+
| Band    | Frame type| Kconfig parameter for upper edge backoff                      |
+=========+===========+===============================================================+
| 2.4 GHz | DSSS      | CONFIG_NRF70_BAND_2G_UPPER_EDGE_BACKOFF_DSSS                  |
+         +-----------+---------------------------------------------------------------+
|         | HT/V HT   | CONFIG_NRF70_BAND_2G_UPPER_EDGE_BACKOFF_HT                    |
+         +-----------+---------------------------------------------------------------+
|         | HE        | CONFIG_NRF70_BAND_2G_UPPER_EDGE_BACKOFF_HE                    |
+---------+-----------+---------------------------------------------------------------+
| UNII-1  | HT/V HT   | CONFIG_NRF70_BAND_UNII_1_UPPER_EDGE_BACKOFF_HT                |
+         +-----------+---------------------------------------------------------------+
|         | HE        | CONFIG_NRF70_BAND_UNII_1_UPPER_EDGE_BACKOFF_HE                |
+---------+-----------+---------------------------------------------------------------+
| UNII-2A | HT/V HT   | CONFIG_NRF70_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HT               |
+         +-----------+---------------------------------------------------------------+
|         | HE        | CONFIG_NRF70_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HE               |
+---------+-----------+---------------------------------------------------------------+
| UNII-2C | HT/V HT   | CONFIG_NRF70_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HT               |
+         +-----------+---------------------------------------------------------------+
|         | HE        | CONFIG_NRF70_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HE               |
+---------+-----------+---------------------------------------------------------------+
| UNII-3  | HT/V HT   | CONFIG_NRF70_BAND_UNII_3_UPPER_EDGE_BACKOFF_HT                |
+         +-----------+---------------------------------------------------------------+
|         | HE        | CONFIG_NRF70_BAND_UNII_3_UPPER_EDGE_BACKOFF_HE                |
+---------+-----------+---------------------------------------------------------------+
| UNII-4  | HT/V HT   | CONFIG_NRF70_BAND_UNII_4_UPPER_EDGE_BACKOFF_HT                |
+         +-----------+---------------------------------------------------------------+
|         | HE        | CONFIG_NRF70_BAND_UNII_4_UPPER_EDGE_BACKOFF_HE                |
+---------+-----------+---------------------------------------------------------------+

Setting band edge parameters
****************************

Review if the transmission (TX) power of any band-edge channels requires backoff according to the active regulatory domain.
The Kconfig parameters and their default values are defined in the :file:`<ncs_repo>/zephyr/drivers/wifi/nrf_wifi/Kconfig.nrfwifi` file.
If backoff is needed, set the following Kconfig parameters to ``<backoff_dB>`` in the project :file:`prj.conf` configuration file.

Kconfig parameters for the lower edge backoff:

* :kconfig:option:`CONFIG_NRF70_BAND_2G_LOWER_EDGE_BACKOFF_DSSS`
* :kconfig:option:`CONFIG_NRF70_BAND_2G_LOWER_EDGE_BACKOFF_HT`
* :kconfig:option:`CONFIG_NRF70_BAND_2G_LOWER_EDGE_BACKOFF_HE`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_1_LOWER_EDGE_BACKOFF_HT`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_1_LOWER_EDGE_BACKOFF_HE`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HT`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_2A_LOWER_EDGE_BACKOFF_HE`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HT`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_2C_LOWER_EDGE_BACKOFF_HE`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_3_LOWER_EDGE_BACKOFF_HT`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_3_LOWER_EDGE_BACKOFF_HE`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_4_LOWER_EDGE_BACKOFF_HT`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_4_LOWER_EDGE_BACKOFF_HE`

Kconfig parameters for the upper edge backoff:

* :kconfig:option:`CONFIG_NRF70_BAND_2G_UPPER_EDGE_BACKOFF_DSSS`
* :kconfig:option:`CONFIG_NRF70_BAND_2G_UPPER_EDGE_BACKOFF_HT`
* :kconfig:option:`CONFIG_NRF70_BAND_2G_UPPER_EDGE_BACKOFF_HE`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_1_UPPER_EDGE_BACKOFF_HT`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_1_UPPER_EDGE_BACKOFF_HE`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HT`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_2A_UPPER_EDGE_BACKOFF_HE`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HT`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_2C_UPPER_EDGE_BACKOFF_HE`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_3_UPPER_EDGE_BACKOFF_HT`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_3_UPPER_EDGE_BACKOFF_HE`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_4_UPPER_EDGE_BACKOFF_HT`
* :kconfig:option:`CONFIG_NRF70_BAND_UNII_4_UPPER_EDGE_BACKOFF_HE`
