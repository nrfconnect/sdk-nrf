.. _ug_nrf5340_multi_image:

Multi-image builds on the nRF5340 DK using child and parent images
##################################################################

.. contents::
   :local:
   :depth: 2

.. note::
    |sysbuild_related_deprecation_note|

If a sample consists of several images (in this case, different images for the application core and for the network core), you can build these images separately or combined as a :ref:`multi-image build <ug_multi_image>`, depending on the sample configuration.

In a multi-image build, the image for the application core is usually the parent image, and the image for the network core is treated as a child image in a separate domain.
For this to work, the network core image must be explicitly added as a child image to one of the application core images.
See :ref:`ug_multi_image_defining` for details.

.. note::
   When using the :ref:`nrf5340_empty_app_core` sample, the image hierarchy is inverted.
   In this case, the network core image is the parent image and the application core image is the child image.

Default build configuration
***************************

By default, the two images are built together for all Bluetooth LE, Thread, Zigbee, and Matter samples in the |NCS|.
Samples that are designed to run only on the network core include the :ref:`nrf5340_empty_app_core` sample as a child image.
For other samples, the images are built separately.

The :term:`build configuration` depends on the following Kconfig options that must be set in the configuration of the parent image:

* :kconfig:option:`CONFIG_BT_HCI_IPC` - set to ``y`` in all Bluetooth LE samples for the application core
* :kconfig:option:`CONFIG_NRF_802154_SER_HOST` - set to ``y`` in all Thread, Zigbee, and Matter samples for the application core
* :kconfig:option:`CONFIG_NCS_SAMPLE_EMPTY_APP_CORE_CHILD_IMAGE` - set to ``y`` in all network core samples that require the :ref:`nrf5340_empty_app_core` sample

The combination of these options determines which (if any) sample is included in the build of the parent image:

.. list-table::
   :header-rows: 1

   * - Enabled options
     - Child image sample for the network core
     - Child image sample for the application core
   * - :kconfig:option:`CONFIG_BT_HCI_IPC`
     - :zephyr:code-sample:`bluetooth_hci_ipc`
     - ---
   * - :kconfig:option:`CONFIG_NRF_802154_SER_HOST`
     - :zephyr:code-sample:`nrf_ieee802154_rpmsg`
     - ---
   * - :kconfig:option:`CONFIG_BT_HCI_IPC` and :kconfig:option:`CONFIG_NRF_802154_SER_HOST`
     - :ref:`multiprotocol-rpmsg-sample`
     - ---
   * - :kconfig:option:`CONFIG_NCS_SAMPLE_EMPTY_APP_CORE_CHILD_IMAGE`
     - ---
     - :ref:`nrf5340_empty_app_core`

Configuration of the child image
********************************

When a network sample is built automatically as a child image in a multi-image build, you can define the relevant Kconfig options (if required) in a :file:`.conf` file.
Name the file :file:`network_sample*\ .conf`, where *network_sample* is the name of the child image (for example, :file:`hci_ipc.conf`).
Place the file in a :file:`child_image` subfolder of the application sample directory.
See :ref:`ug_multi_image_variables` for more information.

This way of defining the Kconfig options allows to align the configurations of both images.

For example, see the :ref:`ble_throughput` child image configuration in :file:`nrf/samples/bluetooth/throughput/child_image/hci_ipc.conf`.
