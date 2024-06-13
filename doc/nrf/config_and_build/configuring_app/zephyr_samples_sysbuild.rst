.. _zephyr_samples_sysbuild:

Using Zephyr samples with sysbuild
##################################

With the introduction of sysbuild in |NCS|, images for devices that have multiple cores must now configure which images they need in order to be built successfully and run.
For applications and samples in the |NCS| repository, this is handled automatically but for samples that are in the ``zephyr`` directory that come from upstream Zephyr, these images must be selected manually when building an image for nRF53 devices (for nRF52 as this is a single core device, no additional images are needed for Bluetooth samples).

+---------------------------------------------------------+-------------------------+-----------+----------+-----------------------------------------------------------------------------------------------------------------------+
| Sysbuild Kconfig option                                 | Image                   | Bluetooth | 802.15.4 | Details                                                                                                               |
+=========================================================+=========================+===========+==========+=======================================================================================================================+
| :kconfig:option:`SB_CONFIG_NETCORE_EMPTY`               | ``empty_net_core``      | ✕         | ✕        |                                                                                                                       |
+---------------------------------------------------------+-------------------------+-----------+----------+-----------------------------------------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_NETCORE_HCI_IPC`             | ``hci_ipc``             | ✓         | ✕        |                                                                                                                       |
+---------------------------------------------------------+-------------------------+-----------+----------+-----------------------------------------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_NETCORE_RPC_HOST`            | ``rpc_host``            | ✓         | ✕        | Requires that application be setup for this mode                                                                      |
+---------------------------------------------------------+-------------------------+-----------+----------+-----------------------------------------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_NETCORE_802154_RPMSG`        | ``802154_rpmsg``        | ✕         | ✓        |                                                                                                                       |
+---------------------------------------------------------+-------------------------+-----------+----------+-----------------------------------------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_NETCORE_MULTIPROTOCOL_RPMSG` | ``multiprotocol_rpmsg`` | ✓         | ✓        |                                                                                                                       |
+---------------------------------------------------------+-------------------------+-----------+----------+-----------------------------------------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO`           | ``ipc_radio``           | ✓         | ✓        | Requires additional configuration. The following Kconfigs provide predefined configurations:                          |
|                                                         |                         |           |          | :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC` enables HCI serialization                                    |
|                                                         |                         |           |          | for Bluetooth, :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO_BT_RPC` enables nRF RPC serialization for Bluetooth, or   |
|                                                         |                         |           |          | :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO_IEEE802154` enables spinel serialization for IEEE 802.15.4.              |
+---------------------------------------------------------+-------------------------+-----------+----------+-----------------------------------------------------------------------------------------------------------------------+
| :kconfig:option:`SB_CONFIG_NETCORE_NONE`                | No image                | -         | -        |                                                                                                                       |
+---------------------------------------------------------+-------------------------+-----------+----------+-----------------------------------------------------------------------------------------------------------------------+

The default for Thingy:53 is the ``empty_net_core`` sample application.
The default for other nRF53 devices is having no image added to the build.

When configuring an application, such as :zephyr_file:`zephyr/samples/bluetooth/peripheral_hr`, you must configure it with a supported network core image to ensure proper functionality.
For basic Bluetooth samples, you can use `hci_ipc`, `multiprotocol_rpmsg`, or `ipc_radio`.
The following instructions demonstrate how to build a sample with the `hci_ipc` network image selected:

.. tabs::

    .. group-tab:: west

        west build -b <board> -- -DSB_CONFIG_NETCORE_HCI_IPC=y

    .. group-tab:: cmake

        cmake -GNinja -DBOARD=<board> -DSB_CONFIG_NETCORE_HCI_IPC=y -DAPP_DIR=<app_dir> <path_to_zephyr/share/sysbuild>

When building and programming such a project, both the main application and the selected network core image will be programmed to the device and the sample application will run as expected.
