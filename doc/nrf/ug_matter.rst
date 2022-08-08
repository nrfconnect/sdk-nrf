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

Matter is in an early development stage and must be treated as an experimental feature.

.. matter_intro_end

.. note::
    |matter_gn_required_note|

The |NCS| allows you to develop applications that are compatible with Matter.
See :ref:`matter_samples` for the list of available samples or :ref:`Matter Weather Station <matter_weather_station_app>` for specific Matter application.
If you are new to Matter, you can watch the following webinar video and follow along with the video tutorials on Nordic Semiconductor's YouTube channel, for example `Developing Matter products with nRF Connect SDK`_.

.. raw:: html

   <h1 align="center">
   <iframe id="Matter_Introduction_TechWebinar"
           title="NordicTech Webinar: Introduction to Matter"
           width="700"
           height="393"
           src="https://webinars.nordicsemi.com/v.ihtml/player.html?source=embed&photo%5fid=70782494">
   </iframe>
   </h1>

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   ug_matter_architecture.rst
   ug_matter_configuring.rst
   ug_matter_tools.rst
   ug_matter_creating_accessory.rst
