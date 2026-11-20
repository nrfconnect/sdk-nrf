.. _tfm_hello_world:

.. ncs-sample::
   :title: TF-M Hello World

   The TF-M Hello World sample is based on Hello World that demonstrates adding Trusted Firmware-M (TF-M) to an application.

This sample uses the Platform Security Architecture (PSA) API to calculate a SHA-256 digest and the TF-M platform read service to read platform-specific secure registers (FICR on nRF53 and nRF91 Series, CPUID on nRF54L Series and nRF7120 DK).
The PSA API call is handled by the TF-M secure firmware.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Configuration
*************

|config|

Building and running
********************

.. |sample path| replace:: :ncs-file:`/samples/tfm/tfm_hello_world`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

1. |connect_terminal|
#. Build and program the application.
#. Observe the logs from the application using the terminal emulator.
   For example, the log output should look like this:

.. code-block:: console

    Hello World! nrf5340dk/nrf5340/cpuapp/ns
    Reading some secure memory that NS is allowed to read
    Approximate IPC overhead us: 65
    FICR->INFO.FLASH: 0x00000400
    Generating random number
    0x03607cef3bdcbbec52edebeb7a0e80756b96d647c82fab068f26d436ff078152
    Hashing 'Hello World! nrf5340dk/nrf5340/cpuapp/ns'
    SHA256 digest:
    0x486c6219904c9aacc3add3ccc503ac94778e9b13fe3e8b1e2f7572bf881fcb5f
    Example finished successfully!

On nRF54L Series and nRF7120 DK board targets, the secure-memory line shows ``NRF_APPLICATION_CPUC_S->CPUID`` instead of ``FICR->INFO.FLASH``.

Dependencies
************

This sample uses the TF-M module that can be found in the following location in the |NCS| folder structure:

* :ncs-file:`trusted-firmware-m:/`

This sample uses the following libraries:

* :ref:`lib_tfm_ioctl_api`
