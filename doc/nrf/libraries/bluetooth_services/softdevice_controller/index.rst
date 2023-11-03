.. _softdevice_controller:

SoftDevice Controller
#####################

The |controller| is an RTOS-agnostic library built for the Nordic Semiconductor nRF52 and nRF53 Series.
It supports Bluetooth 5.
The library utilizes services provided by the :ref:`mpsl`.

The |controller| is distributed in different variants containing different features.
Variants for the Arm Cortex-M4 processor are available as soft-float, softfp-float, and hard-float.
Variants for the Arm Cortex-M33 processor are available as soft-float only.

|BLE| feature support:

+--------------------------------+-----------------+--------------+-----------+
|                                | Peripheral-only | Central-only | Multirole |
+================================+=================+==============+===========+
| 2 Mbps PHY                     | X               | X            | X         |
+--------------------------------+-----------------+--------------+-----------+
| Advertiser                     | X               |              | X         |
+--------------------------------+-----------------+--------------+-----------+
| Slave                          | X               |              | X         |
+--------------------------------+-----------------+--------------+-----------+
| Scanner                        |                 | X            | X         |
+--------------------------------+-----------------+--------------+-----------+
| Master                         |                 | X            | X         |
+--------------------------------+-----------------+--------------+-----------+
| Data Length Extensions         | X               | X            | X         |
+--------------------------------+-----------------+--------------+-----------+
| Advertising Extensions         |                 |              | X         |
+--------------------------------+-----------------+--------------+-----------+
| Periodic Advertising           |                 |              | X         |
+--------------------------------+-----------------+--------------+-----------+
| Connectionless CTE Advertising |                 |              | X         |
+--------------------------------+-----------------+--------------+-----------+
| Connection CTE Response        |                 |              | X         |
+--------------------------------+-----------------+--------------+-----------+
| Coded PHY (Long Range)         |                 |              | X         |
+--------------------------------+-----------------+--------------+-----------+
| LE Power Control Request       | X               | X            | X         |
+--------------------------------+-----------------+--------------+-----------+
| Periodic Advertising Sync      |                 |              | X         |
| Transfer - Sender              |                 |              |           |
+--------------------------------+-----------------+--------------+-----------+
| Periodic Advertising Sync      |                 |              | X         |
| Transfer - Receiver            |                 |              |           |
+--------------------------------+-----------------+--------------+-----------+
| Sleep Clock Accuracy Updates   |                 |              | X         |
+--------------------------------+-----------------+--------------+-----------+
| Periodic Advertising with      |                 |              | X         |
| Responses - Advertiser         |                 |              |           |
+--------------------------------+-----------------+--------------+-----------+
| Periodic Advertising with      |                 |              | X         |
| Responses - Scanner            |                 |              |           |
+--------------------------------+-----------------+--------------+-----------+
| Connected Isochronous Stream   |                 |              | X         |
| - Central                      |                 |              |           |
| (experimental support)         |                 |              |           |
+--------------------------------+-----------------+--------------+-----------+
| Connected Isochronous Stream   |                 |              | X         |
| - Peripheral                   |                 |              |           |
| (experimental support)         |                 |              |           |
+--------------------------------+-----------------+--------------+-----------+
| Isochronous Broadcaster        |                 |              | X         |
| (experimental support)         |                 |              |           |
+--------------------------------+-----------------+--------------+-----------+
| Synchronized Receiver          |                 |              | X         |
| (experimental support)         |                 |              |           |
+--------------------------------+-----------------+--------------+-----------+

.. note::
   The following limitations apply to the listed features:

   * For Connectionless CTE Advertising, angle of arrival (AoA) is supported, but angle of departure (AoD) is not.
   * For Connection CTE Response, angle of arrival (AoA) is supported, but angle of departure (AoD) is not.
   * For Periodic Advertising Sync Transfer - Receiver, only one sync transfer reception may be in progress at any one time per link.
   * For the Isochronous Channels features, nRF52820 and nRF52833 are the nRF52 Series devices that are supported.

.. _sdc_proprietary_feature_support:

Proprietary feature support:

+--------------------------+-----------------+--------------+-----------+-----------------------------------------------------------------------------+
|                          | Peripheral-only | Central-only | Multirole | Description                                                                 |
+==========================+=================+==============+===========+=============================================================================+
| Low Latency Packet mode  |                 |              | X         | Enables using connection intervals below 7.5 ms                             |
+--------------------------+-----------------+--------------+-----------+-----------------------------------------------------------------------------+
| QoS Conn Event Reports   | X               | X            | X         | Reports QoS for every connection event.                                     |
|                          |                 |              |           | The application can then set an adapted channel map to avoid busy channels. |
+--------------------------+-----------------+--------------+-----------+-----------------------------------------------------------------------------+
| QoS Channel Survey       |                 | X            | X         | Provides measurements of the energy levels on the Bluetooth LE channels.    |
| (experimental support)   |                 |              |           | The application can then set an adapted channel map to avoid busy channels. |
+--------------------------+-----------------+--------------+-----------+-----------------------------------------------------------------------------+


.. note::

   Low Latency Packet mode is not supported on the nRF53 Series.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   softdevice_controller
   scheduling
   limitations
   CHANGELOG
   api
