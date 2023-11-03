.. _nrf_modem_bootloader:

Full firmware updates
#####################

.. contents::
   :local:
   :depth: 2

Initializing the Modem library in bootloader mode gives access to the functionality of the modem bootloader, through the :file:`nrf_modem_bootloader.h` interface.
The modem bootloader can be used to perform a full update of the modem firmware.
Full modem firmware update is the only operation that can be performed when the library is initialized in bootloader mode.
AT commands and networking sockets are not available to the application when the modem is in bootloader mode.

It is possible to switch between the bootloader and normal operation modes by reinitializing the library, by calling :c:func:`nrf_modem_shutdown` before :c:func:`nrf_modem_bootloader_init` or :c:func:`nrf_modem_init`, respectively.

Memory requirements
*******************

The bootloader mode requires a shared memory area of size equal to the value of ``NRF_MODEM_SHMEM_BOOTLOADER_SIZE``.

.. note::

   When using the |NCS|, the library is initialized by the glue, which configures the size and position of the shared memory regions in RAM.

Modem library initialization
============================

To perform a full firmware update of the modem, the library must be initialized in bootloader mode as shown in the following code:

.. code-block:: c

   /* Initialize modem to prepare for full modem firmware update */
   nrf_modem_bootloader_init(bootloader_init_params);

If the library has already been initialized in normal mode, it must be shut down through the :c:func:`nrf_modem_shutdown` function and reinitialized as shown in the following code:

.. code-block:: c

   nrf_modem_init(init_params);
   /* Shutdown and re-initialize modem to prepare for full modem firmware update */
   nrf_modem_shutdown();
   nrf_modem_bootloader_init(bootloader_init_params);

Considerations for corrupted modem firmware
===========================================

When designing your application, you might have to consider an interrupted modem firmware update process, which can lead to corrupted modem firmware.
There can be various reasons, for example power loss, which can cause an interruption in a firmware update.
The modem does not have a back-up ROM and hence, an interruption in the modem firmware update process can prevent the modem from further boot up.

As a workaround to the above scenario, either the application must tolerate the situation or another means of recovery must be provided, for example, reprogramming the modem with PC tools.

The Modem library must then be manually initialized with the following code:

.. code-block:: c

   int main(void)
   {
     int rc = nrf_modem_init(init_params);

     if (rc < 0) {
       // Recover the modem firmware from external flash
       nrf_modem_bootloader_init(bootloader_init_params);
       // Perform the full modem firmware update.
       nrf_modem_shutdown();
       nrf_modem_init(init_params);
     }
     // Modem firmware updated, continue as normal
   }


Bootloader API
**************

A full firmware update of the modem consists of the following steps:

1. Initialization
#. Programming the bootloader
#. Programming the modem firmware
#. Verification

Bootloader forms the first segment of the firmware package and it must be programmed initially.
If any failures happen, the sequence of steps must be restarted from the initialization phase.

Initialization
==============

To initialize the full firmware update process for the modem, call the following function:

.. code-block:: c

   int nrf_modem_bootloader_init(struct nrf_modem_bootloader_digest *digest_buffer);

Programming the bootloader
==========================

To program a bootloader, call the following function:

.. code-block:: c

   int nrf_modem_bootloader_bl_write(void *src, uint32_t len)

The bootloader may be written in smaller chunks, which are internally appended together by the library.
When all pieces are written, call the following function:

.. code-block:: c

   int nrf_modem_bootloader_update(void)

After a successful call, the modem changes to the DFU mode.
At this stage, you can write firmware segments or issue any other DFU commands like ``verify``.

Programming the modem firmware
==============================

Firmware segments are written by using the following function call:

.. code-block:: c

   int nrf_modem_bootloader_fw_write(uint32_t addr, void *src, uint32_t len)

The Modem library buffers the data with the same destination address, until one of the following conditions occur:

* The buffered data reaches 8kb.
* The destination address changes.

At this point, the buffer is written to the flash.
When all the segments are written, you must call the following function:

.. code-block:: c

   int nrf_modem_bootloader_update(void)

Verification
============

To verify the content of the modem flash, use the following function:

.. code-block:: c

   nrf_modem_bootloader_digest(uint32_t addr, uint32_t size, struct nrf_modem_bootloader_digest *digest_buffer);

This function calculates SHA-256 hash over the given flash area.
Compare the hash to the precalculated value that comes with the modem firmware package, to ensure that the image is programmed successfully.
