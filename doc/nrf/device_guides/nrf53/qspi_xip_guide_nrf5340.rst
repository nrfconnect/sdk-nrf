.. _qspi_xip:
.. _ug_nrf5340_intro_xip:

External execute in place (XIP) configuration on the nRF5340 SoC
################################################################

.. contents::
   :local:
   :depth: 2

This guide describes the external execute in place (XIP) support for the nRF5340 SoC.
The nRF5340 SoC is equipped with a Quad Serial Peripheral Interface (QSPI) memory interface, which is capable of exposing QSPI flash as memory for the CPU used to fetch and execute the program code.
The QSPI is available for the application core.
Therefore, it is possible to relocate a part of the application's code to an external memory.
The external flash memory supports on-the-fly encryption and decryption.
NCS supports dividing an application into an internal and external part, along with the MCUboot support.

For more information about QSPI XIP hardware support, the `Execute in place page in the nRF5340 Product Specification`_.

For placing individual source code files into defined memory regions, check the :ref:`zephyr:code_relocation_nocopy` sample in Zephyr.

Enabling configuration options
******************************

Application configuration must support an external XIP and image splitting.

Enable the following options:

* :kconfig:option:`CONFIG_CODE_DATA_RELOCATION`
* :kconfig:option:`CONFIG_HAVE_CUSTOM_LINKER_SCRIPT`
* :kconfig:option:`CONFIG_BUILD_NO_GAP_FILL`
* :kconfig:option:`CONFIG_XIP`
* :kconfig:option:`CONFIG_NORDIC_QSPI_NOR_XIP`
* :kconfig:option:`CONFIG_XIP_SPLIT_IMAGE`
* :kconfig:option:`CONFIG_BOOTLOADER_MCUBOOT` - This option enables the MCUboot build and support for the application.

Additionally, set the following options:

* :kconfig:option:`CONFIG_CUSTOM_LINKER_SCRIPT` to ``"<linker_file_for_relocation>"``
* :kconfig:option:`CONFIG_UPDATEABLE_IMAGE_NUMBER` to ``3``
* :kconfig:option:`CONFIG_FLASH_INIT_PRIORITY` to ``40`` - You must ensure the QSPI device initialization priority, as it makes the external XIP code accessible before it is executed.
  If any initialization code is expected to be run from the QSPI XIP, then its initialization priority value must be lower than the QSPI device initialization priority.

Setting up QSPI flash
*********************

The QSPI flash DTS must be available for the application and the MCUboot.
You must correctly set up the QSPI flash chip in the board devicetree file, including the operating mode.
The flash chip does not have to run in the QSPI mode for XIP to function, but using other modes will reduce the execution speed of the application.

See the following snippet for an example of the Nordic Thingy:53 configuration that supports DSPI:

.. code-block:: devicetree

        &qspi {
        status = "okay";
        pinctrl-0 = <&qspi_default>;
        pinctrl-1 = <&qspi_sleep>;
        pinctrl-names = "default", "sleep";
        mx25r64: mx25r6435f@0 {
            compatible = "nordic,qspi-nor";
            reg = <0>;
            writeoc = "pp2o";
            readoc = "read2io";
            sck-frequency = <8000000>;
            jedec-id = [c2 28 17];
            sfdp-bfp = [
                e5 20 f1 ff  ff ff ff 03  44 eb 08 6b  08 3b 04 bb
                ee ff ff ff  ff ff 00 ff  ff ff 00 ff  0c 20 0f 52
                10 d8 00 ff  23 72 f5 00  82 ed 04 cc  44 83 68 44
                30 b0 30 b0  f7 c4 d5 5c  00 be 29 ff  f0 d0 ff ff
            ];
            size = <67108864>;
            has-dpd;
            t-enter-dpd = <10000>;
            t-exit-dpd = <35000>;
        };
    };

.. note::
    Due to QSPI peripheral product anomaly, the QSPI peripheral must be ran with the ``HFCLK192MCTRL=0`` setting.
    Any other value may cause undefined operation of the device.

Add the following to the DTS overlay for your board:

.. code-block:: devicetree

    / {
        chosen {
                nordic,pm-ext-flash = &mx25r64;
        };
    };

Setting up static partition manager
***********************************

You need to complete the setup in order to use a static partitioning in your project.
The configuration must have 3 images with 2 slots each:

.. figure:: images/nrf5340_static_partition_manager_slots.svg
   :alt: Static partitioning slots in the nRF5340 SoC

   Static partitioning slots in the nRF5340 SoC.

* The first set of slots is for the internal flash part of the application.
  These slots should be named ``mcuboot_primary`` and ``mcuboot_secondary``.
* The second set of slots is for the network core update.
  These slots should be named ``mcuboot_primary_1`` and ``mcuboot_secondary_1``.
* The third set of slots is for the QSPI XIP part of the application.
  These slots should be named ``mcuboot_primary_2`` and ``mcuboot_secondary_2``.

This means a basic dual image configuration for the nRF5340 DK needs to describe an external QSPI XIP code partition as ``mcuboot_primary_2`` partition.
Additionally, ensure that:

* The ``mcuboot_primary_2`` address is expressed as the QSPI flash physical address.
* The ``device`` field is the QSPI device name.
* The ``region`` field is set as ``external_flash``.

See the following snippet for an example of the static configuration for partition manager:

.. code-block:: console

    app:
        address: 0x10200
        end_address: 0xe4000
        region: flash_primary
        size: 0xd3e00
    external_flash:
        address: 0x120000
        device: MX25R64
        end_address: 0x800000
        region: external_flash
        size: 0x6e0000
    mcuboot:
        address: 0x0
        end_address: 0x10000
        region: flash_primary
        size: 0x10000
    mcuboot_pad:
        address: 0x10000
        end_address: 0x10200
        region: flash_primary
        size: 0x200
    mcuboot_primary:
        address: 0x10000
        end_address: 0xf0000
        orig_span: &id001
        - mcuboot_pad
        - app
        region: flash_primary
        size: 0xe0000
        span: *id001
    mcuboot_primary_1:
        address: 0x0
        device: flash_ctrl
        end_address: 0x40000
        region: ram_flash
        size: 0x40000
    mcuboot_primary_app:
        address: 0x10200
        end_address: 0xf0000
        orig_span: &id002
        - app
        region: flash_primary
        size: 0xdfe00
        span: *id002
    mcuboot_secondary:
        address: 0x0
        device: MX25R64
        end_address: 0xe0000
        region: external_flash
        size: 0xe0000
    mcuboot_secondary_1:
        address: 0xe0000
        device: MX25R64
        end_address: 0x120000
        region: external_flash
        size: 0x40000
    mcuboot_primary_2:
        address: 0x120000
        device: MX25R64
        end_address: 0x160000
        region: external_flash
        size: 0x40000
    mcuboot_secondary_2:
        address: 0x160000
        device: MX25R64
        end_address: 0x1a0000
        region: external_flash
        size: 0x40000
    otp:
        address: 0xff8100
        end_address: 0xff83fc
        region: otp
        size: 0x2fc
    pcd_sram:
        address: 0x20000000
        end_address: 0x20002000
        region: sram_primary
        size: 0x2000
    ram_flash:
        address: 0x40000
        end_address: 0x40000
        region: ram_flash
        size: 0x0
    rpmsg_nrf53_sram:
        address: 0x20070000
        end_address: 0x20080000
        placement:
            before:
            - end
        region: sram_primary
        size: 0x10000
    settings_storage:
        address: 0xf0000
        end_address: 0x100000
        region: flash_primary
        size: 0x10000
    sram_primary:
        address: 0x20002000
        end_address: 0x20070000
        region: sram_primary
        size: 0x6e000

Configuring linker script
*************************

To relocate code to the external flash, you need to configure a linker script.
The script needs to describe the ``EXTFLASH`` flash memory block to which the code will be linked.
The ``ORIGIN`` of the area can be calculated using following elements:

* The QSPI memory starting with the 0x10000000 internal memory address.
* The offset of an external application part image within the QSPI flash.
  The external application code partition is mapped by the ``mcuboot_primary_2`` PM partition.
* The image header size of the MCUboot image (0x200).

See the following example of the calculation:

.. code-block:: console

    #include <zephyr/linker/sections.h>
    #include <zephyr/devicetree.h>
    #include <zephyr/linker/linker-defs.h>
    #include <zephyr/linker/linker-tool.h>

    MEMORY
    {
        /* This maps in mcuboot_primary_2 partition defined in pm_static.yaml
        * components for ORIGIN calculation:
        *  - 0x10000000: offset of QSPI external memory in SoC memory mapping.
        *  - 0x120000: mcuboot_primary_2 offset in QSPI external memory
        *  - 0x200: image header size.
        * The size of this region is size of mcuboot_primary_2 reduced by the
        * image header size.
        */
        EXTFLASH (wx) : ORIGIN = 0x10120200, LENGTH = 0x3FE00
    }

    #include <zephyr/arch/arm/cortex_m/scripts/linker.ld>

Setting up code relocation
**************************

Relocating code to QSPI XIP is a part of the project's :file:`CMakeLists.txt` file.
You can set up the relocation on a file or library basis using the ``zephyr_code_relocate()`` function.
For example, to relocate a file in the application, use the following configuration:

.. code-block:: console

   zephyr_code_relocate(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/bluetooth.c LOCATION EXTFLASH_TEXT NOCOPY)
   zephyr_code_relocate(FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/bluetooth.c LOCATION RAM_DATA)

where the first line relocates the XIP code (.text) and the second line relocates the data initialization content section (.data).

Similarly, it is possible to relocate certain libraries, for example:

.. code-block:: console

   zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__mgmt LOCATION EXTFLASH_TEXT NOCOPY)
   zephyr_code_relocate(LIBRARY subsys__mgmt__mcumgr__mgmt LOCATION RAM_DATA)

Building the project
********************

You must use child image when building your project, as XIP QSPI does not currently support sysbuild.

Flashing the project
********************

For the nRF5340 DK and other boards equipped with flash working in the QSPI mode, use the ``west flash`` command.
For other cases, flashing needs to be done manually.

Flashing to external flash in SPI/DSPI mode
===========================================

Flashing an application with ``west`` triggers the ``nrfjprog`` runner.
The runner uses the default system settings that configure the application in the QSPI mode when flashing the external flash.
You can change this behavior by using a custom :file:`Qspi.ini` configuration file, however, it will prevent flashing through west.

.. note::
    The :file:`Qspi.ini` file is required to work on the Nordic Thingy:53.

If you wish to use the :file:`Qspi.ini` file, you will need to manually flash the HEX files in the repository.
For example, for the :ref:`smp_svr_ext_xip` sample, you need to flash the following files (paths are relative to the build directory):

* :file:`<cpunet_build_subdirectory>/zephyr/merged_CPUNET.hex`

  * For Bluetooth stack application the path is :file:`<cpunet_build_subdirectory> hci_ipc`.
* :file:`mcuboot/zephyr/zephyr.hex`
* :file:`zephyr/internal_flash_signed.hex`
* :file:`zephyr/qspi_flash_signed.hex`

Use the following commands to flash and verify the Simple Management Protocol (SMP) server sample:

.. code-block:: console

    nrfjprog -f NRF53 --coprocessor CP_NETWORK --sectorerase --program hci_ipc/zephyr/merged_CPUNET.hex --verify
    nrfjprog -f NRF53 --sectorerase --program mcuboot/zephyr/zephyr.hex --verify
    nrfjprog -f NRF53 --sectorerase --program zephyr/internal_flash_signed.hex --verify
    nrfjprog -f NRF53 --qspisectorerase --program zephyr/qspi_flash_signed.hex --qspiini <path_to>/Qspi.ini --verify
    nrfjprog -f NRF53 --reset

.. note::
    The external flash chip must be connected to the dedicated QSPI peripheral port pins of the nRF5340 SoC.
    It is not possible to program an external flash chip that is connected to different pins using nrfjprog.

Troubleshooting
***************

Refer to the following sections for information on how to solve the most common issues.

Module does not appear to start
===============================

When using QSPI XIP, a frequent issue is the module not starting or crashing before the application runs.
This often results from a mismatch in ``init`` priorities between the code on QSPI flash and the QSPI flash device.

To debug this issue, you can use a debugger such as GNU Debugger (GDB) to single-step through the application code until a QSPI address is encountered.
The backtrace functionality can then show which part of the code is responsible for the issue, and you can adjust the ``init`` priority of that module accordingly.

Given that the QSPI flash ``init`` priority defaults to ``41`` at the ``POST_KERNEL`` level, take into account the following points:

* There should be no QSPI flash residing code that has an ``init`` priority value that is less than or equal to the ``POST_KERNEL`` level ``41``.
* No interrupt handlers in the QSPI flash should be enabled until the QSPI flash driver has been initialized.

Module does not boot after update
=================================

This issue can occur if there is a mismatch between the internal flash code and the QSPI XIP code.
Both slots must be running the same build to successfully boot.
The application will fail to boot in the following cases:

* If one of the updates is not loaded.
* If a different build is loaded to one of the slots.
* If one of the loaded updates is corrupt and deleted.

.. _ug_nrf5340_intro_xip_measurements:

Indication of XIP performance
*****************************

The XIP code execution performance measurement was conducted to evaluate the expected performance in different operating conditions.

The :ref:`nrf_machine_learning_app` application running on the nRF5340 DK was used for the testing.
This particular application was used because its application design allows to move the Edge Impulse library to external memory.
There is only one call to the library from the wrapper module, and therefore this call is used to measure the time of execution.
Additional measurements of the current allowed to compare total energy used.

The following table lists performance numbers that were measured under different operating conditions.

.. note::
   The numbers in the table refer to current consumed only by the nRF5340 SoC.
   For complete numbers, you must add the current used by external flash, which varies between manufacturers.

.. _ug_nrf5340_intro_xip_measurements_table:

+-----------------+-----------------+--------+--------------+--------+-----------+--------------------+--------------------+--------------------------+--------------------------+
| CPU frequency   | Memory          | Cache  | QSPI speed   | Mode   | Time [ms] | Current @3.0V [mA] | Current @1.8V [mA] | Total energy @3.0V [µJ]  | Total energy @1.8V [µJ]  |
+=================+=================+========+==============+========+===========+====================+====================+==========================+==========================+
| 64 MHz          | Internal flash  | Yes    | n/a          | n/a    | 63        | 3.2                | 5.1                | 605                      | 578                      |
+-----------------+-----------------+--------+--------------+--------+-----------+--------------------+--------------------+--------------------------+--------------------------+
| 64 MHz          | External flash  | Yes    | 48 MHz       | Quad   | 68.9      | 5.63               | 8.51               | 1164                     | 1055                     |
+-----------------+-----------------+--------+--------------+--------+-----------+--------------------+--------------------+--------------------------+--------------------------+
| 64 MHz          | External flash  | Yes    | 24 MHz       | Quad   | 73.7      | 5.58               | 8.44               | 1234                     | 1120                     |
+-----------------+-----------------+--------+--------------+--------+-----------+--------------------+--------------------+--------------------------+--------------------------+
| 128 MHz         | Internal flash  | Yes    | n/a          | n/a    | 31        | 7.65               | 12.24              | 711                      | 683                      |
+-----------------+-----------------+--------+--------------+--------+-----------+--------------------+--------------------+--------------------------+--------------------------+
| 128 MHz         | External flash  | Yes    | 96 MHz       | Quad   | 34.1      | 8.99               | 14.1               | 920                      | 865                      |
+-----------------+-----------------+--------+--------------+--------+-----------+--------------------+--------------------+--------------------------+--------------------------+
| 128 MHz         | External flash  | No     | 96 MHz       | Quad   | 88.5      | 9.15               | 12.95              | 2429                     | 2063                     |
+-----------------+-----------------+--------+--------------+--------+-----------+--------------------+--------------------+--------------------------+--------------------------+
| 128 MHz         | External flash  | Yes    | 48 MHz       | Quad   | 36.4      | 8.85               | 13.9               | 966                      | 911                      |
+-----------------+-----------------+--------+--------------+--------+-----------+--------------------+--------------------+--------------------------+--------------------------+
