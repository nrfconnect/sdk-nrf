.. _ug_matter_device_optimizing_memory:

Optimizing memory usage in Matter applications
##############################################

.. contents::
   :local:
   :depth: 2

You can use different approaches to optimizing the memory usage of your Matter application, both on the |NCS| side and in the Matter SDK.

Reducing memory usage on the |NCS| side
***************************************

See the :ref:`app_memory` guide for information about how to reduce memory usage for the |NCS| generally and for specific subsystems in particular, including Bluetooth® LE, Matter, and Thread.

.. _ug_matter_device_optimizing_memory_logs:

Cutting off log regions for Matter SDK modules
**********************************************

The Matter SDK, included in the |NCS| as one of the submodule repositories using a `dedicated Matter fork`_, provides a custom mechanism for optimizing memory usage in a Matter application.
This solution defines a series of logging modules, each of which is a logical section of code that is a source of log messages and can include one or more files.
For the complete list of modules, see the Matter SDK's `LogModule enumeration`_.

You can reduce the memory usage of each of these modules in the following ways:

* Define a custom logging level for each module by setting one of the `Matter SDK logging levels <Matter SDK's Logging header_>`_ (Error, Progress, Detail, or Automation) in :file:`src/chip_project_config.h`,  located in your application's directory.
  For example, the following snippet shows the Bluetooth LE module set to logging at the `Detail` level:

  .. code-block:: c

     CHIP_CONFIG_LOG_MODULE_Ble_DETAIL 1

* Turn off logging for any of the modules by setting its respective enabler to ``0`` in the Matter SDK's `LogModule enumeration`_.
  For example, the following snippet shows the Bluetooth LE module cut off from logging its entries:

  .. code-block:: c

     CHIP_CONFIG_LOG_MODULE_Ble 0

Enabling Link Time Optimization (LTO)
*************************************

You can reduce the memory usage of your Matter application by enabling Link Time Optimization (LTO).

LTO is an advanced compilation technique that performs optimization across all compiled units of an application at the link stage, rather than within each unit separately.

To enable LTO, set the :kconfig:option:`CONFIG_LTO` and :kconfig:option:`CONFIG_ISR_TABLES_LOCAL_DECLARATION` Kconfig options to ``y``.

.. note::
   Support for Link Time Optimization is experimental.

.. _ug_matter_device_memory_profiling:

Profiling memory in Matter applications
***************************************

This section offers useful commands for measuring usage of stacks, heaps, and Zephyr settings, and Kconfig options for configuring their memory size.
Measuring memory usage allows you to adjust these values according to your project's requirements.

Measuring memory usage
======================

You can obtain the current memory statistics from the device using Kconfig options and UART shell commands.
To do this, set the :kconfig:option:`CONFIG_CHIP_MEMORY_PROFILING` global Matter memory profiling Kconfig to ``y``.
This activates all the other necessary Kconfig options and enables all UART shell commands for measuring memory usage.
Alternatively, you can enable each option separately.

The Kconfig option enables the following functionalities on the Matter device:

- :ref:`ug_matter_configuring_settings_shell` by setting the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_SETTINGS_SHELL` Kconfig option to ``y``.
- :doc:`matter:nrfconnect_examples_cli` by setting the :kconfig:option:`CONFIG_CHIP_LIB_SHELL` Kconfig option to ``y``.
- Zephyr Kernel commands by setting the :kconfig:option:`CONFIG_KERNEL_SHELL` Kconfig option to ``y``.
- Zephyr Settings shell by setting the :kconfig:option:`CONFIG_SETTINGS_SHELL` Kconfig option to ``y``.
- OpenThread shell by setting the :kconfig:option:`CONFIG_OPENTHREAD_SHELL` Kconfig option to ``y`` if you build the Matter over Thread variant.
- Matter statistics shell by setting the :kconfig:option:`CONFIG_CHIP_STATISTICS` Kconfig option to ``y``.

All the functionalities listed below are automatically enabled if the Matter memory profiling Kconfig option is activated.
However, you can also find the specific Kconfig options required for each functionality to enable them separately.

Heap usage and Matter-related statistics
----------------------------------------

You can measure heap usage by monitoring peak usage.
To do this, you need to set the following in your application :file:`prj.conf` file:

.. parsed-literal::
   :class: highlight

    CONFIG_SHELL=y
    CONFIG_CHIP_STATISTICS=y
    CONFIG_CHIP_MALLOC_SYS_HEAP_WATERMARKS_SUPPORT=y
    CONFIG_CHIP_MALLOC_SYS_HEAP=y
    CONFIG_SYS_HEAP_RUNTIME_STATS=y

To monitor peak usage, you first reset the current peak measurement, then read the peak usage, perform some operations on the device, and read the peak usage again.
The difference between the initial and subsequent peak values indicates the number of bytes dynamically allocated by all operations performed by the device during that interval.
This method allows you to check for memory leaks or determine the amount of memory specific operations dynamically allocate but some objects may be also destroyed in the meantime, so to obtain the most accurate values the intervals should be as small as possible.
By using these results, you can adjust the maximum heap size for your application, thereby optimizing RAM usage for other purposes.

To measure the heap usage, complete the following steps:

1. Clear the current peak values:

   .. code-block:: console

      uart:~$ matter stat reset

#. Read the current peak values:

   .. code-block:: console

      uart:~$ matter stat peak

   As a result you will get the following log:

   .. code-block:: console

      Packet Buffers: 0
      Timers: 2
      TCP endpoints: 0
      UDP endpoints: 1
      Exchange contexts: 0
      Unsolicited message handlers: 5
      Platform events: 1
      Heap allocated bytes: 208

   The value for ``Heap allocated bytes`` shows the current peak usage.
   In addition to heap statistics, you can also obtain other information about the Matter stack from this command.

#. Perform some operations on your device.
#. Read the current peak values using the ``matter stat peak`` command again.

   The difference between the current ``Heap allocated bytes`` value and the value in Step 2 shows the heap usage.

Non-Volatile Storage (NVS) Settings usage
-----------------------------------------

You can measure the NVS Settings usage by monitoring peak and current usage.
To do this, you need to set the following in your application :file:`prj.conf` file:

.. parsed-literal::
   :class: highlight

    CONFIG_SHELL=y
    CONFIG_NCS_SAMPLE_MATTER_SETTINGS_SHELL=y
    CONFIG_SHELL_MINIMAL=n
    CONFIG_SETTINGS_SHELL=y

The NVS Settings usage may change during the device's lifetime.
The ``settings_storage`` partition can only be changed by reflashing the Matter device.
This means that it cannot be altered through DFU (Device Firmware Update).
Because of this, you need to be careful when setting the partition, and should allocate some free space to ensure that it can accommodate more data in the future.
The data used within this partition may increase with updates to Matter and the |NCS|.

If this functionality is enabled, you can use :ref:`ug_matter_configuring_settings_shell`.

To see the full list of available commands, use the following UART shell command on your device:

.. code-block:: console

    uart:~$ matter_settings

You will see a list of the available commands like this one:

.. code-block:: console

    peak      : Print peak settings size in Bytes. This size is reset during
              reboot.
              Usage: matter_settings peak
    reset     : Reset peak settings size in Bytes.
                Usage: matter_settings reset
    get_size  : Get size of the chosen settings entry.
                Usage: matter_settings get_size <name>
    current   : Get current settings size in Bytes.
                Usage: matter_settings current
    free      : Get free settings space in Bytes.
                Usage: matter_settings free

Similarly to heap measurements, you can reset the current peak usage value, read the peak value, perform some operations on the device, and read the peak value again to obtain the difference.

1. Reset the peak usage value:

   .. code-block:: console

      uart:~$ matter_settings reset

#. Measure the peak usage:

   .. code-block:: console

      uart:~$ matter_settings peak

#. Perform some operations on your device.
#. Read the current peak usage again using the ``matter_settings peak`` command again.

   The difference between the current peak value and the value in Step 2 shows the peak usage.

The ``matter_settings`` command also allows you also to read the current value of Zephyr settings usage.
To read it from the device, use the following UART shell command on your device:

.. code-block:: console

   uart:~$ matter_settings current

You can also read the size of a specific settings entry by calling the ``matter_settings get_size <name>`` UART shell command on your device.
To obtain the name of an entry, you can use the ``settings list`` command from the ``settings`` UART shell command set.

To read the size of a specific settings entry, complete the following steps:

1. View the list of all available settings:

    .. code-block:: console

        uart:~$ settings list

    .. code-block:: console

        mt/g/im/ec
        mt/g/gdc
        mt/g/gcc
        mt/g/lkgt
        mt/ctr/reboot-count
        mt/cfg/unique-id
        its/0000000000020001

#. Choose one of the available keys, for example ``mt/ctr/reboot-count`` to read size of the reboot counter.
#. Read the size of the chosen key:

    .. code-block:: console

        matter_settings get_size mt/ctr/reboot-count

To learn about other ``settings`` UART shell commands, use the following UART shell command on your device:

.. code-block:: console

    uart:~$ settings

You will see subcommand descriptions like the following:

.. code-block:: console

    settings - Settings shell commands
    Subcommands:
    list    : List all settings in a subtree (omit to list all)
            Usage: settings list [subtree]
    read    : Read a specific setting
            Usage: settings read [type] <name>
            type: string or hex (default: hex)
    write   : Write to a specific setting
            Usage: settings write [type] <name> <value>
            type: string or hex (default: hex)
    delete  : Delete a specific setting
            Usage: settings delete <name>

.. note::

   The :ref:`ug_matter_configuring_settings_shell` provide only the peak value of the current settings usage.
   To estimate the space needed for the ``settings_storage`` partitions, gather the size of each settings key and decide how often the value is updated during the device's lifetime.

Stack usage for all threads
---------------------------

You can measure the stack usage by monitoring peak usage of each thread stack.
To do this, you need to set the following in your application :file:`prj.conf` file:

.. parsed-literal::
   :class: highlight

    CONFIG_SHELL=y
    CONFIG_KERNEL_SHELL=y

You can also measure the peak stack usage of each thread running on the Matter device.
This measurement can help in setting the proper stack size value and saving RAM space for other stacks or the heap.

To see all statistics for each running thread, use the following UART shell command on your device:

.. code-block:: console

    kernel stacks

You will see statistics similar to the following ones, although the number of threads may be different:

.. code-block:: console

    0x20011568 CHIP                             (real size 6144):	unused 3952	usage 2192 / 6144 (35 %)
    0x200069e8 BT RX WQ                         (real size 1216):	unused 1040	usage  176 / 1216 (14 %)
    0x20006930 BT TX                            (real size 1536):	unused 1080	usage  456 / 1536 (29 %)
    0x20006d08 rx_q[0]                          (real size 1536):	unused 1384	usage  152 / 1536 ( 9 %)
    0x20006e18 openthread                       (real size 4096):	unused 3432	usage  664 / 4096 (16 %)
    0x20007be8 ot_radio_workq                   (real size 1024):	unused  848	usage  176 / 1024 (17 %)
    0x20006768 shell_uart                       (real size 3200):	unused 2104	usage 1096 / 3200 (34 %)
    0x20002580 nrf5_rx                          (real size 1024):	unused  832	usage  192 / 1024 (18 %)
    0x2000d510 sysworkq                         (real size 1152):	unused  880	usage  272 / 1152 (23 %)
    0x20007b10 MPSL Work                        (real size 1024):	unused  808	usage  216 / 1024 (21 %)
    0x2000d3a0 idle                             (real size  320):	unused  272	usage   48 /  320 (15 %)
    0x2000d458 main                             (real size 6144):	unused 4584	usage 1560 / 6144 (25 %)
    0x20025d00 IRQ 00                           (real size 2048):	unused 1120	usage  928 / 2048 (45 %)


You can read the peak usage measurement for each thread and learn about the total size of the stack, and unused bytes.
You can adjust the stack values for your application using estimations based on these measurements.

Configuring memory usage
========================

Most of the Matter samples in the |NCS| have a safe configuration that assumes a high number of free space for heap, stacks, and settings partition size.
After measuring the memory usage, you may want to adjust the memory parameters according to your project's requirements.

The following sections present a guide on how to adjust specific maximum memory values.

Settings usage
--------------

.. important::

    The ``settings_storage`` partition can only be changed by reflashing the Matter device.
    This means that it cannot be altered through DFU (Device Firmware Update).
    Because of this, you need to be careful when setting the partition, and should allocate some free space to ensure that it can accommodate more data in the future.
    The data used within this partition may increase with updates to Matter and the |NCS|.

To adjust the settings usage, you need to modify the :file:`pm_static` file related to your target board.
For example, to modify the ``settings_storage`` partition in the :ref:`Matter Template <matter_template_sample>` sample for the ``nrf52840dk_nrf52840`` target, complete the following steps:

1. Locate the :file:`pm_static_nrf52840dk_nrf52840.yml` in the sample directory
#. Locate the ``settings_storage`` partition within the ``pm_static`` file.

   For example:

   .. code-block:: console

       settings_storage:
           address: 0xf8000
           size: 0x8000
           region: flash_primary

#. Modify the ``size`` value.
#. Align all other partitions to not overlap any memory regions.

   To learn more about how to configure partitions in the :file:`pm_static` file, see the :ref:`partition_manager` documentation.
#. Align the :kconfig:option:`CONFIG_SETTINGS_NVS_SECTOR_COUNT` Kconfig option value to the used NVS sectors.
   Each target in |NCS| Matter samples uses 4 kB NVS sectors, so you can divide the ``settings_storage`` partition size by 4096 (0x1000) to get the value you need to set for the :kconfig:option:`CONFIG_SETTINGS_NVS_SECTOR_COUNT` Kconfig option.

To learn more about partitioning, see the :ref:`ug_matter_device_bootloader_partition_layout` guide.

Stack sizes for all threads
---------------------------

Each thread has its own Kconfig option to configure the maximum stack size.
You can modify Kconfig values to increase or decrease the maximum stack sizes according to your project's requirements.

The following table presents the possible threads used in a Matter application and the Kconfig options dedicated to setting the maximum stack usage for each of them:

.. _matter_threads_table:

+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| Thread name         | Kconfig option                                                   | Description of the related stack                               |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| CHIP                | :kconfig:option:`CONFIG_CHIP_TASK_STACK_SIZE`                    | Matter thread stack.                                           |
|                     |                                                                  | For example, all functions scheduled to be execute from        |
|                     |                                                                  | the Matter thread context using                                |
|                     |                                                                  | the ``SystemLayer().ScheduleLambda`` function.                 |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| openthread          | :kconfig:option:`CONFIG_OPENTHREAD_THREAD_STACK_SIZE`            | OpenThread thread stack.                                       |
|                     |                                                                  | For Matter over Thread only.                                   |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| main                | :kconfig:option:`CONFIG_MAIN_STACK_SIZE`                         | Application thread stack.                                      |
|                     |                                                                  | For example, all functions scheduled to be execute from        |
|                     |                                                                  | the Main thread context using                                  |
|                     |                                                                  | the ``Nrf::PostTask`` function.                                |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| idle                | :kconfig:option:`CONFIG_IDLE_STACK_SIZE`                         | The Idle thread that work while any other thread is not        |
|                     |                                                                  | working.                                                       |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| MPSL Work           | :kconfig:option:`CONFIG_MPSL_WORK_STACK_SIZE`                    | :ref:`lib_mpsl_libraries` thread stack.                        |
|                     |                                                                  | Switching times slots for multi-protocol purposes.             |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| sysworkq            | :kconfig:option:`CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE`             | Zephyr stack. Switching context purposes.                      |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| shell_uart          | :kconfig:option:`CONFIG_SHELL_STACK_SIZE`                        | Zephyr shell purposes.                                         |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| BT TX               | :kconfig:option:`CONFIG_BT_HCI_TX_STACK_SIZE`                    | Bluetooth LE transmitting thread stack.                        |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| nrf5_rx             | :kconfig:option:`CONFIG_IEEE802154_NRF5_RX_STACK_SIZE`           | Bluetooth LE receiving thread stack.                           |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| BT RX WQ            | :kconfig:option:`CONFIG_BT_RX_STACK_SIZE`                        | Bluetooth LE processing thread stack.                          |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| ot_radio_workq      | :kconfig:option:`CONFIG_OPENTHREAD_RADIO_WORKQUEUE_STACK_SIZE`   | IEEE 802.15.4 radio processing thread stack.                   |
|                     |                                                                  | For Matter over Thread only.                                   |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| net_mgmt            | :kconfig:option:`CONFIG_NET_MGMT_EVENT_STACK_SIZE`               | Zephyr network management event processing thread stack.       |
|                     |                                                                  | For Matter over Wi-Fi only.                                    |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| wpa_supplicant_main | :kconfig:option:`CONFIG_WPA_SUPP_THREAD_STACK_SIZE`              | WPA supplicant main thread.                                    |
|                     |                                                                  | Processing Wi-Fi requests and connections.                     |
|                     |                                                                  | For Matter over Wi-Fi only.                                    |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| wpa_supplicant_wq   | :kconfig:option:`CONFIG_WPA_SUPP_WQ_STACK_SIZE`                  | WPA supplicant work queue thread.                              |
|                     |                                                                  | Processing Wi-Fi task queue                                    |
|                     |                                                                  | For Matter over Wi-Fi only.                                    |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| nrf700x_bh_wq       | :kconfig:option:`CONFIG_NRF70_BH_WQ_STACK_SIZE`                  | nRF700x Wi-Fi driver work queue.                               |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| nrf700x_intr_wq     | :kconfig:option:`CONFIG_NRF70_IRQ_WQ_STACK_SIZE`                 | Interrupts processing generated by the nRF700X Wi-Fi radio.    |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+
| mbox_wq             | :kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_RPMSG_WQ_STACK_SIZE` | Inter Processor Communication.                                 |
|                     |                                                                  | For multi-processors targets only.                             |
+---------------------+------------------------------------------------------------------+----------------------------------------------------------------+

Heap size
---------

The Matter application defines the static size of the heap used by all memory allocations.
This configuration ignores the :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` Kconfig option value in the application configuration.
The static size is determined by the :kconfig:option:`CONFIG_CHIP_MALLOC_SYS_HEAP` and :kconfig:option:`CONFIG_CHIP_MALLOC_SYS_HEAP_OVERRIDE` Kconfig options.
To use a dynamic heap size on your Matter device, set them both to ``n``.

The static heap size means that you can define the maximum heap size for your application by setting the :kconfig:option:`CONFIG_CHIP_MALLOC_SYS_HEAP_SIZE` Kconfig value.
You can also adjust the heap dedicated for MbedTLS purposes by setting the :kconfig:option:`CONFIG_MBEDTLS_HEAP_SIZE` Kconfig option value.

Memory profiling troubleshooting
================================

When you manipulate memory sizes, your device can experience issues and faults.
Many of these issues can be caused by incorrect stack or heap configurations.
The following can help you troubleshoot if you notice problems with your application:

- Check the name of the thread from which the fault originates.

  If you notice a fault on your Matter device, look at the logs to find the thread that was executing when the error occurred.
  Sometimes, if there is not enough space in a specific thread, a crash can occur.
  In such cases, consider increasing the size of the stack dedicated to that thread.
  Find the thread in the :ref:`thread and Kconfig option <matter_threads_table>` table and change the value of the corresponding Kconfig option in your project configuration.

- The code fails on ``calloc``/``malloc`` functions.

  If your code fails on functions that allocate dynamic memory, try increasing the :kconfig:option:`CONFIG_CHIP_MALLOC_SYS_HEAP_SIZE` Kconfig value.

- The code fails while executing some cryptographic operations on an nRF54 Series SoC.

  Currently, most cryptographic operations on the nRF54 Series use the current thread's stack to store operating data.
  This means that if a demanding cryptographic operation is executed from a thread context with a stack size that is too small, it will cause an unexpected crash.
  If you notice such an issue, increase the stack size for the failing thread to verify that the cryptographic operations begin to function properly again.
