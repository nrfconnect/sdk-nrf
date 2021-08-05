.. _nrf_modem_lib_readme:

Modem library integration layer
###############################

.. contents::
   :local:
   :depth: 2


The Modem library integration layer handles the integration of the Modem library into |NCS|.
The integration layer is constituted by the library wrapper and functionalities like socket offloading, OS abstraction, memory reservation by the Partition manager, and diagnostics.

Library wrapper
***************

The library wrapper provides an encapsulation over the core Modem library functions such as initialization and shutdown.
The library wrapper is implemented in :file:`nrf\\lib\\nrf_modem_lib\\nrf_modem_lib.c`.

The library wrapper encapsulates the :c:func:`nrf_modem_init` and :c:func:`nrf_modem_shutdown` calls of the Modem library with :c:func:`nrf_modem_lib_init` and :c:func:`nrf_modem_lib_shutdown` calls, respectively.
The library wrapper eases the task of initializing the Modem library by automatically passing the size and address of all the shared memory regions of the Modem library to the :c:func:`nrf_modem_init` call.

:ref:`partition_manager` is the component that reserves the RAM memory for the shared memory regions used by the Modem library.
For more information, see :ref:`partition_mgr_integration`.

The library wrapper can also initialize the Modem library during system initialization using :c:macro:`SYS_INIT`.
The :kconfig:`CONFIG_NRF_MODEM_LIB_SYS_INIT` Kconfig option can be used to control the initialization.
Some libraries in |NCS|, such as the :ref:`at_cmd_readme` and the :ref:`lte_lc_readme` have similar configuration options to initialize during system initialization and these options depend on the configuration option of the integration layer.
If your application performs an update of the nRF9160 modem firmware, you must disable this functionality to have full control on the initialization of the library.

The library wrapper also coordinates the shutdown operation among different parts of the application that use the Modem library.
This is done by the :c:func:`nrf_modem_lib_shutdown` function call, by waking the sleeping threads when the modem is being shut down.

When using the Modem library in |NCS|, the library should be initialized and shutdown using the :c:func:`nrf_modem_lib_init` and :c:func:`nrf_modem_lib_shutdown` function calls, respectively.

Socket offloading
*****************

Zephyr Socket API offers the :ref:`socket offloading functionality <zephyr:net_socket_offloading>` to redirect or *offload* function calls to BSD socket APIs such as ``socket()`` and ``send()``.
The integration layer utilizes this functionality to offload the socket API calls to the Modem library and thus eases the task of porting the networking code to the nRF9160 by providing a wrapper for Modem library's native socket API such as :c:func:`nrf_socket` and :c:func:`nrf_send`.

The socket offloading functionality in the integration layer is implemented in :file:`nrf\\lib\\nrf_modem_lib\\nrf91_sockets.c`.

Modem library socket API sets errnos as defined in :file:`nrf_errno.h`.
The socket offloading support in the integration layer in |NCS| converts those errnos to the errnos that adhere to the selected C library implementation.

The socket offloading functionality is enabled by default.
To disable the functionality, set the :kconfig:`CONFIG_NET_SOCKETS_OFFLOAD` Kconfig option to ``n`` in your project configuration.
If you disable the socket offloading functionality, the socket calls will no longer be offloaded to the nRF9160 modem firmware.
Instead, the calls will be relayed to the native Zephyr TCP/IP implementation.
This can be useful to switch between an emulator and a real device while running networking code on these devices.
Note that the even if the socket offloading is disabled, Modem library's own socket APIs such as :c:func:`nrf_socket` and :c:func:`nrf_send` remain available.

OS abstraction layer
********************

For functioning, the Modem library requires the implementation of an OS abstraction layer, which is an interface over the operating system functionalities such as interrupt setup, threads, and heap.
The integration layer provides an implementation of the OS abstraction layer using |NCS| components.
The OS abstraction layer is implemented in the :file:`nrfxlib\\nrf_modem\\include\\nrf_modem_os.c`.

The behavior of the functions in the OS abstraction layer is dependent on the |NCS| components that are used in their implementation.
This is relevant for functions such as :c:func:`nrf_modem_os_shm_tx_alloc`, which uses :ref:`Zephyr's Heap implementation <zephyr:heap_v2>` to dynamically allocate memory.
In this case, the characteristics of the allocations made by these functions depend on the heap implementation by Zephyr.

.. _partition_mgr_integration:

Partition manager integration
*****************************

The Modem library, which runs on the application core, shares an area of RAM memory with the nRF9160 modem core.
During the initialization, the Modem library accepts the boundaries of this area of RAM and configures the communication with the modem core accordingly.

However, it is the responsibility of the application to reserve that RAM during linking, so that this memory area is not used for other purposes and remain dedicated for use by the Modem library.

In |NCS|, the application can configure the size of the memory area dedicated to the Modem library through the integration layer.
The integration layer provides a set of Kconfig options that help the application reserve the required amount of memory for the Modem library by integrating with another |NCS| component, the :ref:`partition_manager`.

The RAM area that the Modem library shares with the nRF9160 modem core is divided into the following four regions:

* Control
* RX
* TX
* Trace

The size of the RX, TX and the Trace regions can be configured by the following Kconfig options of the integration layer:

* :kconfig:`CONFIG_NRF_MODEM_LIB_SHMEM_RX_SIZE` for the RX region
* :kconfig:`CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE` for the TX region
* :kconfig:`CONFIG_NRF_MODEM_LIB_SHMEM_TRACE_SIZE` for the Trace region

The size of the Control region is fixed.
The Modem library exports the size value through :kconfig:`CONFIG_NRF_MODEM_SHMEM_CTRL_SIZE`.
This value is automatically passed by the integration layer to the library during the initialization through :c:func:`nrf_modem_lib_init`.

When the application is built using CMake, the :ref:`partition_manager` automatically reads the Kconfig options of the integration layer.
Partition manager decides about the placement of the regions in RAM and reserves memory according to the given size.
As a result, the Partition manager generates the following parameters:

* ``PM_NRF_MODEM_LIB_CTRL_ADDRESS`` - Address of the Control region
* ``PM_NRF_MODEM_LIB_TX_ADDRESS`` - Address of the TX region
* ``PM_NRF_MODEM_LIB_RX_ADDRESS`` - Address of the RX region
* ``PM_NRF_MODEM_LIB_TRACE_ADDRESS`` - Address of the Trace region

Partition manager also generates the following additional parameters:

* ``PM_NRF_MODEM_LIB_CTRL_SIZE`` - Size of the Control region
* ``PM_NRF_MODEM_LIB_TX_SIZE`` - Size of the TX region
* ``PM_NRF_MODEM_LIB_RX_SIZE`` - Size of the RX region
* ``PM_NRF_MODEM_LIB_TRACE_SIZE`` - Size of the Trace region

These parameters will have identical values as the ``CONFIG_NRF_MODEM_LIB_SHMEM_*_SIZE`` configuration options.

When the Modem library is initialized by the integration layer in |NCS|, the integration layer automatically passes the boundaries of each shared memory region to the Modem library during the :c:func:`nrf_modem_lib_init` call.

Diagnostic functionality
************************

The Modem library integration layer in |NCS| provides some diagnostic functionalities to log the allocations on the Modem library heap and the TX memory region.
These functionalities can be turned on by the :kconfig:`CONFIG_NRF_MODEM_LIB_DEBUG_ALLOC` and :kconfig:`CONFIG_NRF_MODEM_LIB_DEBUG_SHM_TX_ALLOC` options.

The contents of both the Modem library heap and the TX memory region can be examined through the :c:func:`nrf_modem_lib_heap_diagnose` and :c:func:`nrf_modem_lib_shm_tx_diagnose` functions, respectively.
Additionally, it is possible to schedule a periodic report of the contents of these two areas of memory by using the :kconfig:`CONFIG_NRF_MODEM_LIB_HEAP_DUMP_PERIODIC` and :kconfig:`CONFIG_NRF_MODEM_LIB_SHM_TX_DUMP_PERIODIC` options, respectively.
The report will be printed by a dedicated work queue that is distinct from the system work queue at configurable time intervals.

API documentation
*****************

| Header file: :file:`include/modem/nrf_modem_lib.h`
| Source file: :file:`lib/nrf_modem_lib.c`

.. doxygengroup:: nrf_modem_lib
   :project: nrf
   :members:
