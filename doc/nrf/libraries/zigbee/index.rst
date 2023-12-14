.. _lib_zigbee:

Libraries for Zigbee
####################

|zigbee_description|
See the :ref:`ug_zigbee` user guide for an overview of the technology.

Zigbee libraries are a set of components that can be directly reused in Zigbee applications.
They implement the default behavior of a Zigbee device and are used in :ref:`zigbee_samples` in this SDK.

The following table lists libraries enabled by default for each available Zigbee sample (✔) or available through a configuration file (``**``):

.. list-table::
    :widths: auto
    :header-rows: 1

    * - Library name
      - :ref:`Light bulb <zigbee_light_bulb_sample>`
      - :ref:`Light switch <zigbee_light_switch_sample>`
      - :ref:`Network coordinator <zigbee_network_coordinator_sample>`
      - :ref:`NCP <zigbee_ncp_sample>`
      - :ref:`Shell (sample) <zigbee_shell_sample>`
      - :ref:`Template <zigbee_template_sample>`
    * - ZBOSS OSIF
      - ✔
      - ✔
      - ✔
      - ✔
      - ✔
      - ✔
    * - Shell (library)
      -
      -
      -
      -
      - ✔
      -
    * - Application utilities (default signal handler)
      - ✔
      - ✔
      - ✔
      -
      - ✔
      - ✔
    * - Error handler
      - ✔
      - ✔
      - ✔
      -
      - ✔
      - ✔
    * - FOTA
      -
      - ``**``
      -
      -
      -
      -
    * - Endpoint logger
      -
      -
      -
      -
      - ✔
      -
    * - Scene helper
      - ✔
      -
      -
      -
      -
      -

Read the table symbols in the following manner:

* ``✔`` - The library is enabled by default for the given Zigbee sample.
* ``**`` - The library support is available through the :file:`prj_fota.conf` file for the given Zigbee sample.
* No symbol - The library is not enabled for the given sample, but you can enable support by following the instructions on the :ref:`ug_zigbee_configuring_libraries`.

.. toctree::
   :maxdepth: 1
   :glob:
   :caption: Subpages:

   *
