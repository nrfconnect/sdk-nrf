.. _partition_mgr_integration:

Partition manager integration
#############################

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

* :kconfig:option:`CONFIG_NRF_MODEM_LIB_SHMEM_RX_SIZE` for the RX region
* :kconfig:option:`CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE` for the TX region
* :kconfig:option:`CONFIG_NRF_MODEM_LIB_SHMEM_TRACE_SIZE` for the Trace region

The size of the Control region is fixed.
The Modem library exports the size value through :kconfig:option:`CONFIG_NRF_MODEM_SHMEM_CTRL_SIZE`.
This value is automatically passed by the integration layer to the library during the initialization through :c:func:`nrf_modem_lib_init`.

When the application is built using CMake, the :ref:`partition_manager` automatically reads the Kconfig options of the integration layer.
Partition manager decides about the placement of the regions in RAM and reserves memory according to the given size.
As a result, the Partition manager generates the following definitions:

* ``PM_NRF_MODEM_LIB_CTRL_ADDRESS`` - Address of the Control region
* ``PM_NRF_MODEM_LIB_TX_ADDRESS`` - Address of the TX region
* ``PM_NRF_MODEM_LIB_RX_ADDRESS`` - Address of the RX region
* ``PM_NRF_MODEM_LIB_TRACE_ADDRESS`` - Address of the Trace region
* ``PM_NRF_MODEM_LIB_CTRL_SIZE`` - Size of the Control region
* ``PM_NRF_MODEM_LIB_TX_SIZE`` - Size of the TX region
* ``PM_NRF_MODEM_LIB_RX_SIZE`` - Size of the RX region
* ``PM_NRF_MODEM_LIB_TRACE_SIZE`` - Size of the Trace region

These definitions will have identical values as the ``CONFIG_NRF_MODEM_LIB_SHMEM_*_SIZE`` configuration options.

.. important::
   The heap implementation used for allocations on the TX region has an overhead of up to 128 bytes.
   Adjust the size of the TX region accordingly, so that its size is 128 bytes larger than the largest allocation you expect to happen (longest AT command, largest payload passed to :c:func:`nrf_send`) in your application.

When the Modem library is initialized by the integration layer in |NCS|, the integration layer automatically passes the boundaries of each shared memory region to the Modem library during the :c:func:`nrf_modem_lib_init` call.
