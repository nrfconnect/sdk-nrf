.. _devicetree_integration:

Devicetree integration
######################

The :ref:`nrfxlib:nrf_modem`, which runs on the application core, shares a region of RAM memory with the modem core.
During the initialization, the Modem library accepts the boundaries of this region of RAM and configures the communication with the modem core accordingly.

However, it is the responsibility of the application to reserve that RAM during linking so that this memory area is not used for other purposes and remains dedicated for use by the Modem library.

The size and location of the shared memory can be defined in devicetree by creating the following RAM sections:

* ``sram0_ns_modem``:

  This section contains all the subsections needed for communicating with the modem.
  You must place it in a specific area of RAM depending on the device (see the :ref:`following table <modem_shmem_variations>`).

  * ``cpuapp_cpucell_ipc_shm_ctrl``:

    This section is used for control messages.
    It has a minimum size depending on the modem firmware (see the :ref:`following table <modem_shmem_variations>`).

  * ``cpuapp_cpucell_ipc_shm_heap``:

    This section is used for data sent from the Modem library to the modem.
    The heap implementation used has an overhead of up to 128 bytes.
    Adjust the size of this region so it is 128 bytes larger than the largest allocation you expect to happen in your application (such as the longest AT command or the largest payload passed to :c:func:`nrf_send`).

  * ``cpucell_cpuapp_ipc_shm_heap``:

    This section is used for data sent from the modem to the Modem library.

  * ``cpucell_cpuapp_ipc_shm_trace``:

    This section is used for modem tracing.
    It is optional and only needs to be added when modem tracing is enabled.

* ``sram0_ns_app``:

  This section is used as the application memory.
  The size of this section will be assigned to ``CONFIG_SRAM_SIZE`` in kB, so this section must be 1024-byte aligned.

.. _modem_shmem_variations:

.. table:: Control region size and shared memory location

  +---------------+--------------+-----------------------+---------------------+------------------------+
  | Device series | Variant      | Firmware              | Control region size | Shared memory location |
  +---------------+--------------+-----------------------+---------------------+------------------------+
  | nRF91         | DECT NR+     | * mfw_nr+_nrf91x1     | 1832 bytes          | first 128 kB of RAM    |
  |               |              | * mfw_nr+-phy_nrf91x1 |                     |                        |
  |               +--------------+-----------------------+---------------------+                        |
  |               | Cellular     | * mfw_nrf9160         | 1256 bytes          |                        |
  |               |              | * mfw_nrf91x1         |                     |                        |
  |               |              | * mfw_nrf9151-ntn     |                     |                        |
  +---------------+--------------+-----------------------+---------------------+------------------------+
