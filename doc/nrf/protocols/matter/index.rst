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

Matter over Thread support is in the :ref:`production state <software_maturity>`, but Matter over Wi-Fi is in an early development stage and must be treated as an :ref:`experimental feature <software_maturity>`.

.. matter_intro_end

The |NCS| allows you to develop applications that are compatible with Matter.
See :ref:`matter_samples` for the list of available samples or :ref:`Matter Weather Station <matter_weather_station_app>` for specific Matter application.
If you are new to Matter, you can follow along with the video tutorials on Nordic Semiconductor's YouTube channel, for example `Developing Matter 1.0 products with nRF Connect SDK`_.

.. note::
    |matter_gn_required_note|

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   overview/index
   getting_started/index
   end_product/index
