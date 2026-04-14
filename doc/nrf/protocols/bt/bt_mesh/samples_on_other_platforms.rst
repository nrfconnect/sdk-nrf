.. _ug_bt_mesh_samples_on_other_platforms:

Running Bluetooth Mesh samples on other platforms
#################################################

.. contents::
   :local:
   :depth: 2

The :ref:`bt_mesh_samples` are provided with support for a subset of available Nordic Semiconductor boards and SoCs.
However, you can adapt several samples to run on other platforms.
The following sections describe how to do this.

Running samples on the nRF54LM20B SoC
*************************************

The nRF54LM20B SoC has a similar peripheral set and memory layout to the nRF54LM20A SoC.
Because of this, Bluetooth Mesh samples that support the nRF54LM20A SoC can typically also run on the nRF54LM20B SoC.

To build a Bluetooth Mesh sample that already supports the nRF54LM20A SoC for the nRF54LM20B SoC target, complete the following steps:

1. In the sample's :file:`boards` directory, copy each file related to nRF54LM20A (meaning, files with ``nrf54lm20a`` in the filename), if there are any.
   Rename each copied file by replacing ``nrf54lm20a`` with ``nrf54lm20b``.
   If the sample is otherwise unmodified, these files typically do not require additional content changes.
#. Build the sample with ``-b nrf54lm20dk/nrf54lm20b/cpuapp`` to target the nRF54LM20B SoC.
   For any build configuration beyond the board target, follow the sample-specific build and run instructions in the sample documentation.

For example, the :ref:`bluetooth_mesh_light_lc` sample includes board files for the nRF54LM20A SoC, and you can adapt it to run on the nRF54LM20B SoC as follows:

1. Copy the following files from :file:`samples/bluetooth/mesh/light_ctrl/boards` to the same directory:

   * :file:`samples/bluetooth/mesh/light_ctrl/boards/nrf54lm20dk_nrf54lm20a_cpuapp.conf`
   * :file:`samples/bluetooth/mesh/light_ctrl/boards/nrf54lm20dk_nrf54lm20a_cpuapp_emds.overlay`

   Rename each copied file by replacing ``nrf54lm20a`` with ``nrf54lm20b``.

#. Build the sample using ``west build -b nrf54lm20dk/nrf54lm20b/cpuapp``, or ``west build -b nrf54lm20dk/nrf54lm20b/cpuapp -- -DFILE_SUFFIX=emds`` to build the sample with EMDS support.
