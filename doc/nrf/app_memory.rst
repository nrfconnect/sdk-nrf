.. _app_memory:

Memory footprint optimization
#############################

.. contents::
   :local:
   :depth: 2

When developing an application, ROM and RAM footprint are important factors, especially when the firmware runs on the most resource-constrained devices like nRF52810 or nRF52811.

To reduce the memory footprint, ensure that your application uses the minimum required resources and tune the |NCS| configuration parameters.
Complete the following actions to optimize your application:

* Follow the guides for :ref:`optimizing Zephyr <zephyr:optimizations>`.
  Also see the implementation of the :ref:`zephyr:minimal_sample` sample.
* Analyze stack usage in each thread of your application by using the :ref:`zephyr:thread_analyzer`.
  Reduce the stack sizes where possible.
* Limit or disable debugging features such as logging or asserts.
* Go through each component and subsystem and turn off all features that your application does not use.

The following subsections give more information on how to optimize specific subsystems.


Bluetooth
*********

Complete the following actions to optimize the Bluetooth part of your application:

* Disable features that your application does not use.
  For example, disable the following features:

  * Data Length Update
  * Extended Advertising
  * PHY Update
  * Security Manager Protocol (if no encryption and authentication is needed)
  * GATT Caching
  * GATT Service Changed

* Reduce the stack sizes of the Bluetooth internal threads where possible.
  Use the :ref:`zephyr:thread_analyzer` to analyze the stack usage.

  The following configuration options affect the stack sizes of the Bluetooth threads:

  * :option:`CONFIG_SDC_RX_STACK_SIZE`
  * :option:`CONFIG_BT_RX_STACK_SIZE`
  * :option:`CONFIG_BT_HCI_TX_STACK_SIZE`
  * :option:`CONFIG_MPSL_SIGNAL_STACK_SIZE`

* Reduce the overall number and the sizes of the Bluetooth buffers, based on the expected data traffic in your application.

  The following configuration options affect the Bluetooth buffers:

  * :option:`CONFIG_BT_DISCARDABLE_BUF_COUNT`
  * :option:`CONFIG_BT_DISCARDABLE_BUF_SIZE`
  * :option:`CONFIG_BT_RX_BUF_COUNT`
  * :option:`CONFIG_BT_CONN_TX_MAX`
  * :option:`CONFIG_BT_L2CAP_TX_BUF_COUNT`
  * :option:`CONFIG_BT_ATT_TX_MAX`
  * :option:`CONFIG_BT_CTLR_RX_BUFFERS`
  * :option:`CONFIG_BT_CTLR_TX_BUFFERS`
  * :option:`CONFIG_BT_CTLR_TX_BUFFER_SIZE`

For reference, you can find minimal footprint configurations of the :ref:`peripheral_lbs` sample in :file:`nrf/samples/bluetooth/peripheral_lbs/minimal.conf` and the :ref:`peripheral_uart` sample in :file:`nrf/samples/bluetooth/peripheral_uart/minimal.conf`.


Thread
******

Complete the following actions to optimize the Thread part of your application:

* Disable features that your application does not use.
  For example, disable the following features:

  * Asserts
  * Logging
  * Network shell and OpenThread CLI shell support (see :ref:`ug_thread_configuring_additional`)

* :ref:`Configure the OpenThread stack. <ug_thread_configuring_basic_building>`
* :ref:`Select the appropriate OpenThread device type. <thread_ug_device_type>`
* Reduce the stack sizes of the Thread internal threads where possible.
  Use the :ref:`zephyr:thread_analyzer` to analyze the stack usage.

  The following configuration options affect the stack sizes of the Thread threads:

  * :option:`CONFIG_OPENTHREAD_THREAD_STACK_SIZE`
  * :option:`CONFIG_NET_CONNECTION_MANAGER_STACK_SIZE`
  * :option:`CONFIG_NET_RX_STACK_SIZE`
  * :option:`CONFIG_NET_TX_STACK_SIZE`
  * :option:`CONFIG_NET_MGMT_EVENT_STACK_SIZE`
  * :option:`CONFIG_IEEE802154_NRF5_RX_STACK_SIZE`
  * :option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`
  * :option:`CONFIG_MPSL_SIGNAL_STACK_SIZE`
  * :option:`CONFIG_SHELL_STACK_SIZE`
  * :option:`CONFIG_IDLE_STACK_SIZE`
  * :option:`CONFIG_MAIN_STACK_SIZE`
  * :option:`CONFIG_ISR_STACK_SIZE`

For reference, you can find minimal footprint configurations for the single protocol and multiprotocol variants of the :ref:`ot_cli_sample` sample in :file:`nrf/samples/openthread/cli/overlay-minimal_*protocol.conf`.
