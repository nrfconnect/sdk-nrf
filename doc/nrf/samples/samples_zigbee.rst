.. _zigbee_samples:

Zigbee samples
##############

The nRF Connect SDK provides the following samples showcasing the :ref:`ug_zigbee` protocol.
You can build the samples for various boards and configure them for different usage scenarios.

In addition to their basic functionalities, you can expand the samples with variants and extensions.
The following table lists variants and extensions available for each Zigbee sample:

.. list-table::
    :widths: auto
    :header-rows: 1

    * - Variant or extension
      - Light bulb
      - Light switch
      - Network coordinator
      - NCP
      - Shell
      - Template
    * - FEM support
      - ✔
      - ✔
      - ✔
      - ✔
      - ✔
      - ✔
    * - Sleepy End Device behavior
      - :ref:`*** <zigbee_ug_sed>`
      - ✔
      -
      -
      - :ref:`** <zigbee_ug_sed>`
      - :ref:`** <zigbee_ug_sed>`
    * - Multiprotocol Bluetooth LE
      - :ref:`*** <ug_multiprotocol_support>`
      - ✔
      - :ref:`*** <ug_multiprotocol_support>`
      -
      - :ref:`*** <ug_multiprotocol_support>`
      - :ref:`*** <ug_multiprotocol_support>`
    * - Zigbee FOTA
      - :ref:`** <ug_zigbee_configuring_components_ota>`
      - ✔
      - :ref:`** <ug_zigbee_configuring_components_ota>`
      -
      - :ref:`** <ug_zigbee_configuring_components_ota>`
      - :ref:`** <ug_zigbee_configuring_components_ota>`
    * - Endpoint logger
      - :ref:`** <ug_zigbee_configuring_components_logger_ep>`
      - :ref:`** <ug_zigbee_configuring_components_logger_ep>`
      - :ref:`** <ug_zigbee_configuring_components_logger_ep>`
      -
      - ✔
      - :ref:`** <ug_zigbee_configuring_components_logger_ep>`
    * - ZCL scene helper
      - ✔
      - :ref:`*** <ug_zigbee_configuring_components_scene_helper>`
      - :ref:`*** <ug_zigbee_configuring_components_scene_helper>`
      -
      - :ref:`*** <ug_zigbee_configuring_components_scene_helper>`
      - :ref:`*** <ug_zigbee_configuring_components_scene_helper>`

Read the table symbols in the following manner:

* ``✔`` - Using this variant or extension either requires changes in Kconfig through the provided overlay file or the variant or extension is already enabled by default in the sample.
  See the sample documentation for detailed configuration steps for these variants and extensions.
* ``**`` - Implementing this variant or extension requires minimal changes in the sample source code and Kconfig options.
  Click the link for implementation details.
* ``***`` - Implementing this variant or extension requires several changes in both selected Kconfig options and source files, and may require adding new source files to the sample (for example, command implementation).
  Click the link for implementation details.
* No symbol - The variant or extension is not available for the given sample.

Some variants and extensions might also require additional hardware and software, such as mobile applications for testing purposes.

.. toctree::
   :maxdepth: 1
   :caption: Subpages
   :glob:

   ../../../samples/zigbee/*/README
