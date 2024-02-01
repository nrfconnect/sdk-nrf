.. _ug_matter:
.. _ug_chip:

Matter
######

.. contents::
   :local:
   :depth: 2

.. matter_intro_start

`Matter`_ (formerly Project Connected Home over IP or Project CHIP) is an open-source application layer that aims at creating a unified communication standard across smart home devices, mobile applications, and cloud services.
It supports a wide range of existing technologies, including Wi-Fi, Thread, and Bluetooth® LE, and uses IPv6-based transport protocols like TCP and UDP to ensure connectivity between different kinds of networks.

.. matter_intro_end

The |NCS| allows you to develop applications with different versions of Matter, as per the following table:

+--------------------------+-----------------------------------------------------+------------------------+
| nRF Connect SDK version  | Matter specification version                        | `Matter SDK version`_  |
+==========================+=====================================================+========================+
| v2.5.99 (latest)         | :ref:`1.2.0 <ug_matter_overview_dev_model_support>` | 1.2.0.1                |
+--------------------------+-----------------------------------------------------+------------------------+
| |release|                | :ref:`1.1.0 <ug_matter_overview_dev_model_support>` | 1.1.0.1                |
+--------------------------+                                                     |                        |
| v2.5.1                   |                                                     |                        |
+--------------------------+                                                     |                        |
| v2.5.0                   |                                                     |                        |
+--------------------------+                                                     |                        |
| v2.4.3                   |                                                     |                        |
+--------------------------+                                                     |                        |
| v2.4.2                   |                                                     |                        |
+--------------------------+                                                     |                        |
| v2.4.1                   |                                                     |                        |
+--------------------------+                                                     |                        |
| v2.4.0                   |                                                     |                        |
+--------------------------+-----------------------------------------------------+------------------------+
| v2.3.0                   | :ref:`1.0.0 <ug_matter_overview_dev_model_support>` | 1.0.0.2                |
+--------------------------+                                                     +------------------------+
| v2.2.0                   |                                                     | 1.0.0.0                |
+--------------------------+                                                     +------------------------+
| v2.1.4                   |                                                     | 1.0.0.0                |
+--------------------------+                                                     +------------------------+
| v2.1.3                   |                                                     | 1.0.0.0                |
+--------------------------+                                                     +------------------------+
| v2.1.2                   |                                                     | 1.0.0.0                |
+--------------------------+-----------------------------------------------------+------------------------+

.. note::
   The Matter SDK version is taken as the base for the `dedicated Matter fork`_, which can then include additional changes for each |NCS| release.
   These changes are listed in the Matter fork section of the |NCS| :ref:`release_notes`.

.. important::
   For Matter over Thread samples, starting with |NCS| releases after v2.5.2, the default cryptography backend is Arm PSA Crypto API instead of Mbed TLS, which was used in earlier versions.
   To :ref:`inherit Thread certification <ug_matter_device_certification_reqs_dependent>` from Nordic Semiconductor, you must use the PSA Crypto API backend.
   See the :ref:`migration guide <migration_2.6>` to learn about all changes if you are migrating from an earlier version of the |NCS|.

For more information about Matter compatibility, see :ref:`ug_matter_overview_dev_model_support` and :ref:`supported Matter features per SoC <software_maturity_protocol_matter>`.

See :ref:`matter_samples` for the list of available samples, :ref:`Matter Weather Station <matter_weather_station_app>` or :ref:`Matter bridge <matter_bridge_app>` for specific Matter application.
If you are new to Matter, you can follow along with the video tutorials on Nordic Semiconductor's YouTube channel, for example `Developing Matter 1.0 products with nRF Connect SDK`_.

.. note::
    |matter_gn_required_note|

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   overview/index
   getting_started/index
   end_product/index
