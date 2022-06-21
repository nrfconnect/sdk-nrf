.. _app_memory:

Memory footprint optimization
#############################

.. contents::
   :local:
   :depth: 2

When developing an application, ROM and RAM footprint are important factors, especially when the firmware runs on the most resource-constrained devices like nRF52810 or nRF52811.

.. _app_memory_general:

General recommendations
***********************

To reduce the memory footprint, ensure that your application uses the minimum required resources and tune the |NCS| configuration parameters.
Complete the following actions to optimize your application:

* Follow the guides for :ref:`optimizing Zephyr <zephyr:optimizations>`.
  Also see the implementation of the :ref:`zephyr:minimal_sample` sample.
* Analyze stack usage in each thread of your application by using the :ref:`zephyr:thread_analyzer`.
  Reduce the stack sizes where possible.
* Limit or disable debugging features, such as logging or asserts.
* Go through each component and subsystem and turn off all peripherals and features that your application does not use.

The following subsections give more information on how to optimize specific subsystems.

.. _app_memory_bt:

Bluetooth
*********

Besides applying `General recommendations`_, you can also complete the following actions to optimize the Bluetooth® part of your application:

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

  * :kconfig:option:`CONFIG_BT_CTLR_SDC_RX_STACK_SIZE`
  * :kconfig:option:`CONFIG_BT_RX_STACK_SIZE`
  * :kconfig:option:`CONFIG_BT_HCI_TX_STACK_SIZE`
  * :kconfig:option:`CONFIG_MPSL_WORK_STACK_SIZE`

* Reduce the overall number and the sizes of the Bluetooth buffers, based on the expected data traffic in your application.

  The following configuration options affect the Bluetooth buffers:

  * :kconfig:option:`CONFIG_BT_BUF_EVT_DISCARDABLE_COUNT`
  * :kconfig:option:`CONFIG_BT_BUF_EVT_DISCARDABLE_SIZE`
  * :kconfig:option:`CONFIG_BT_BUF_EVT_RX_COUNT`
  * :kconfig:option:`CONFIG_BT_CONN_TX_MAX`
  * :kconfig:option:`CONFIG_BT_L2CAP_TX_BUF_COUNT`
  * :kconfig:option:`CONFIG_BT_CTLR_RX_BUFFERS`
  * :kconfig:option:`CONFIG_BT_BUF_ACL_TX_COUNT`
  * :kconfig:option:`CONFIG_BT_BUF_ACL_TX_SIZE`

For reference, you can find minimal footprint configurations of the :ref:`peripheral_lbs` sample in :file:`nrf/samples/bluetooth/peripheral_lbs/minimal.conf` and the :ref:`peripheral_uart` sample in :file:`nrf/samples/bluetooth/peripheral_uart/minimal.conf`.

.. _app_memory_matter:

Matter
******

Besides applying `General recommendations`_, you can also complete the following actions to optimize the :ref:`Matter <ug_matter>` part of your application:

* Make sure Zephyr's :ref:`zephyr:shell_api` is disabled for your application.
  Related configuration options are listed in a dedicated section in each Matter sample's :file:`prj.conf` file.
* Use :file:`prj_release.conf` for building the application.
  The release configuration has a smaller memory footprint than the default, debug-enabled :file:`prj.conf`.
* If the logs in your application do not use the default log level, you can change the default log level of Zephyr modules from ``info`` to ``warning`` by setting :kconfig:option:`CONFIG_LOG_DEFAULT_LEVEL` to ``2``.
* Change the log level of the Matter logs from ``debug`` to ``info`` by setting :kconfig:option:`CONFIG_MATTER_LOG_LEVEL_INFO` to ``y``.
* Reduce the verbosity of assert messages by setting :kconfig:option:`CONFIG_ASSERT_VERBOSE` to ``n``.
* Check `Thread`_ memory footprint optimization actions, as the Matter application layer uses OpenThread.

.. _app_memory_nfc:

NFC
***

The :ref:`ug_nfc` protocol implementation in the |NCS| provides some options for optimizing memory footprint for both the tag and the poller roles.

Tag
---

To optimize your application that supports the NFC tag functionality, follow the `General recommendations`_.
The NFC :ref:`type_2_tag` and :ref:`type_4_tag` libraries do not provide configuration options that have an effect on memory usage in an application.
However, there are a few Kconfig configuration options you can use to optimize memory usage related to NFC.

* :kconfig:option:`CONFIG_NFC_PLATFORM_LOG_LEVEL_CHOICE` to reduce logging level in the NFC integration module.

For an application that uses the :ref:`type_4_tag` library, you can set the following options:

* :kconfig:option:`CONFIG_NDEF_FILE_SIZE` for the maximum NDEF file size, if the read-write mode is supported,
* :kconfig:option:`CONFIG_NFC_TNEP_RX_MAX_RECORD_CNT` and :kconfig:option:`CONFIG_NFC_TNEP_RX_MAX_RECORD_SIZE` for the maximum buffer size of NDEF message exchange, if the :ref:`lib_nfc_tnep` is supported.

Poller
------

To optimize an application that supports the NFC poller functionality using the :ref:`st25r3911b_nfc_readme` library, you can set the following options:

* :kconfig:option:`CONFIG_ST25R3911B_LIB_LOG_LEVEL_CHOICE` and similar options to reduce the logging level of the NFC components used in the application.
* :kconfig:option:`CONFIG_NFC_T4T_HL_PROCEDURE_CC_BUFFER_SIZE` and :kconfig:option:`CONFIG_NFC_T4T_HL_PROCEDURE_APDU_BUF_SIZE` to adjust the buffer sizes in the NFC T4T protocol implementation of the poller side.

If the application supports the :ref:`lib_nfc_tnep`, you can set the following options:

* :kconfig:option:`CONFIG_NFC_TNEP_POLLER_RX_MAX_RECORD_CNT` for the maximum number of NDEF records in the received message.
* :kconfig:option:`CONFIG_NFC_TNEP_CH_POLLER_RX_BUF_SIZE` for the Connection Handover receive buffer size of the poller mode, if the application uses the :ref:`nfc_tnep_ch_readme` library.

If the application uses the NFC TNEP protocol for the Bluetooth LE out-of-band pairing procedure (:ref:`nfc_tnep_ch_readme`), regardless of the role (tag or poller), you can set the following options:

* :kconfig:option:`CONFIG_NFC_TNEP_CH_MAX_RECORD_COUNT`
* :kconfig:option:`CONFIG_NFC_TNEP_CH_MAX_LOCAL_RECORD_COUNT`
* :kconfig:option:`CONFIG_NFC_TNEP_CH_PARSER_BUFFER_SIZE`

.. _app_memory_thread:

Thread
******

The current Thread memory requirements are listed on the :ref:`thread_ot_memory` page.

Besides applying `General recommendations`_, you can also complete the following actions to optimize the :ref:`Thread <ug_thread>` part of your application:

* Disable Thread features that your application does not use.
  For example, disable network shell and OpenThread CLI shell support (see :ref:`ug_thread_configuring_additional`)
* :ref:`Configure the OpenThread stack. <ug_thread_configuring_basic_building>`
* :ref:`Select the appropriate OpenThread device type. <thread_ug_device_type>`
* Reduce the stack sizes of the Thread internal threads where possible.
  Use the :ref:`zephyr:thread_analyzer` to analyze the stack usage.

  The following configuration options affect the stack sizes of the Thread threads:

  * :kconfig:option:`CONFIG_OPENTHREAD_THREAD_STACK_SIZE`
  * :kconfig:option:`CONFIG_NET_CONNECTION_MANAGER_STACK_SIZE`
  * :kconfig:option:`CONFIG_NET_RX_STACK_SIZE`
  * :kconfig:option:`CONFIG_NET_TX_STACK_SIZE`
  * :kconfig:option:`CONFIG_NET_MGMT_EVENT_STACK_SIZE`
  * :kconfig:option:`CONFIG_IEEE802154_NRF5_RX_STACK_SIZE`
  * :kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`
  * :kconfig:option:`CONFIG_MPSL_WORK_STACK_SIZE`
  * :kconfig:option:`CONFIG_SHELL_STACK_SIZE`
  * :kconfig:option:`CONFIG_IDLE_STACK_SIZE`
  * :kconfig:option:`CONFIG_MAIN_STACK_SIZE`
  * :kconfig:option:`CONFIG_ISR_STACK_SIZE`

.. _app_memory_zigbee:

Zigbee
******

The current Zigbee memory requirements are listed on the :ref:`zigbee_memory` page.

Apply `General recommendations`_ to optimize the :ref:`Zigbee <ug_zigbee>` part of your application.
