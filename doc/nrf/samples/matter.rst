.. _matter_samples:

Matter samples
##############

The |NCS| provides several samples showcasing the :ref:`Matter <ug_matter>` protocol.
You can build the samples for a variety of board targets and configure them for different usage scenarios.

The following table lists variants and extensions available out of the box for each Matter sample:

.. list-table::
    :widths: auto
    :header-rows: 1

    * - Variant or extension
      - Light bulb
      - Light switch
      - Door lock
      - Template
      - Window covering
      - Thermostat
    * - FEM support
      - ✔
      - ✔
      - ✔
      - ✔
      - ✔
      -
    * - DFU support
      - ✔
      - ✔
      - ✔
      - ✔
      - ✔
      - ✔
    * - Thread support
      - ✔
      - ✔
      - ✔
      - ✔
      - ✔
      - ✔
    * - :ref:`Thread role <thread_ot_device_types>`
      - Router
      - SED
      - SED
      - MED
      - SSED
      -
    * - Wi-Fi support
      - ✔
      - ✔
      - ✔
      - ✔
      -
      - ✔
    * - Thread and Wi-Fi switching
      -
      -
      - ✔
      -
      -
      -
    * - Low power configuration by default
      -
      - ✔
      - ✔
      -
      - ✔
      -

See the sample documentation pages for instructions about how to enable these variants and extensions.

In addition to these samples, check also the following Matter applications:

* :ref:`Thingy:53 Matter weather station <matter_weather_station_app>`
* :ref:`Matter bridge <matter_bridge_app>`

.. toctree::
   :maxdepth: 1
   :caption: Subpages
   :glob:

   ../../../samples/matter/*/README
