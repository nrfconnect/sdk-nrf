.. fota_upgrades_update_start

The :zephyr:code-sample:`smp-svr` demonstrates how to set up your project to support FOTA updates.

The sample documentation is from the Zephyr project.
When working in the |NCS| environment, ignore the part of the sample documentation that describes the building and programming steps.
In |NCS|, you can build and program the :zephyr:code-sample:`smp-svr` as any other sample using the following commands:

.. tabs::

    .. group-tab:: nRF5340 SoCs

        .. parsed-literal::
           :class: highlight

            west build -b *board_target* -- -DEXTRA_CONF_FILE=overlay-bt.conf -DSB_CONFIG_NETCORE_HCI_IPC=y
            west flash

    .. group-tab:: Single-core SoCs

        .. parsed-literal::
           :class: highlight

            west build -b *board_target* -- -DEXTRA_CONF_FILE=overlay-bt.conf
            west flash

Make sure to indicate the :file:`overlay-bt.conf` overlay configuration for the Bluetooth transport like in the command example.
This configuration was carefully selected to achieve the maximum possible throughput of the FOTA update transport over Bluetooth with the help of the following features:

* Bluetooth MTU - To increase the packet size of a single Bluetooth packet transmitted over the air (:kconfig:option:`CONFIG_BT_BUF_ACL_RX_SIZE` and others).
* Bluetooth connection parameters - To adaptively change the connection interval and latency on the detection of the SMP service activity (:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL`).
* MCUmgr packet reassembly - To allow exchange of large SMP packets (:kconfig:option:`CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY`, :kconfig:option:`CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE` and others).

Consider using these features in your project to speed up the FOTA update process.

.. fota_upgrades_update_end
