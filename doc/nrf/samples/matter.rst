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
      - Smoke CO alarm
      - Temperature sensor
      - Contact sensor
      - Closure
    * - FEM support
      - ✔
      - ✔
      - ✔
      - ✔
      - ✔
      -
      -
      -
      -
      -
    * - DFU support
      - ✔
      - ✔
      - ✔
      - ✔
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
      - MED
      - SED
      - SED
      - SED
      - FTD
    * - :ref:`ICD mode <ug_matter_device_low_power_icd>`
      - Not supported
      - SIT
      - SIT
      - Not supported
      - SIT
      - Not supported
      - LIT
      - LIT
      - LIT
      - Not supported
    * - Wi-Fi® support
      - ✔
      - ✔
      - ✔
      - ✔
      -
      - ✔
      -
      -
      -
      - ✔
    * - Thread and Wi-Fi switching
      -
      -
      - ✔
      -
      -
      -
      -
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
      - ✔
      - ✔
      - ✔
      -

See the sample documentation pages for instructions about how to enable these variants and extensions.

In addition to these samples, check also the following Matter applications:

* :ref:`Thingy:53 Matter weather station <matter_weather_station_app>`
* :ref:`Matter bridge <matter_bridge_app>`

.. include:: ../samples.rst
    :start-after: samples_general_info_start
    :end-before: samples_general_info_end

|filter_samples_by_board|

.. toctree::
   :maxdepth: 1
   :caption: Subpages
   :glob:

   ../../../samples/matter/*/README
   ../../../samples/matter/common/config
   ../../../samples/matter/common/config_matter_stack
