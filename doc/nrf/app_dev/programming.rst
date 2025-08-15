.. _gs_programming:
.. _programming:
.. _programming_vsc:
.. _programming_cmd:

Programming an application
##########################

.. contents::
   :local:
   :depth: 2

|application_sample_definition|

To program the :ref:`output build files <app_build_output_files>` to your device, which in most of the cases will be :file:`zephyr.hex` or :file:`merged.hex`, use the steps for the development environment.

.. |exceptions_step| replace:: Make sure you are familiar with :ref:`programming_hw`.

.. include:: ../includes/vsc_build_and_run_series.txt

The flash command programs all cores by default, both in |nRFVSC| and on the command line.
If you want to program only one selected core, use ``west flash`` on the command line and :ref:`specify the domain <zephyr:west-multi-domain-flashing>`.

.. _programming_selecting_runner:

Selecting west runner
*********************

Starting with the |NCS| v3.0.0, all Nordic Semiconductor :ref:`boards <app_boards>` are using the `nRF Util`_ as the default runner for the ``west flash`` command.
This change ensures that the programming process is consistent across all boards.
See the :ref:`build system section in the v3.0.0 migration guide <migration_3.0_recommended>` for more information.

.. note::

   The ``west debug`` command is not affected by this change of the default runner.

Runners are Zephyr-specific Python classes that wrap Zephyr's :ref:`flash and debug host tools <zephyr:flash-debug-host-tools>` and integrate them with west and the Zephyr build system to support ``west flash`` and related commands.
Runners supported on a board are determined by the :file:`board.cmake` file in the board folder, for example `this one <nRF52840 DK board.cmake_>`_.
In particular, the following include lines enable them, with the first ``<runner>`` listed becoming the default runner:

.. code-block:: cmake

   include(${ZEPHYR_BASE}/boards/common/<runner>.board.cmake)

For more information about runners, see the Zephyr documentation: :ref:`zephyr:flash-and-debug-support` and :ref:`zephyr:west-flashing`.

If you want to change the runner used by ``west flash``, you can use one of the following options:

* Specify the runner permanently in your application's :file:`CMakeLists.txt` file:

  .. code-block::

     set(BOARD_FLASH_RUNNER nrfjprog)

* Specify the runner for a single command:

  .. code-block::

     west flash -r nrfjprog

In either case, make sure you have the required tools installed on your system.
`nRF Util`_ is part of the :ref:`nRF Connect SDK toolchain bundle <requirements_toolchain>`.
For ``-r nrfjprog``, you need the archived `nRF Command Line Tools`_.

To see which runners are available for your board, run the following command:

.. code-block::

   west flash --context

The output will include lines like the following:

.. code-block::

   available runners in runners.yaml:
     nrfutil, nrfjprog, jlink, pyocd, openocd
   default runner in runners.yaml:
     nrfutil

.. _programming_hw:

Hardware-specific programming steps
***********************************

Some hardware has platform-specific requirements that must be met before you program the device.
For additional information, check the :ref:`user guide for the hardware platform <device_guides>` that you are using.

For example, if you are working with an nRF9160 DK, you need to select the correct position of the :ref:`board controller <nrf9160_ug_intro>` switch **SW10** before you program the application to your development kit.
Programming to Thingy:91 also requires a :ref:`similar step <building_pgming>`, but using a different switch (**SW2**).

Programming the nRF52840 Dongle
  To program the nRF52840 Dongle instead of a development kit, follow the programming instructions in :zephyr:board:`nrf52840dongle` or use the `Programmer app <Programming the nRF52840 Dongle_>`_.

Programming the nRF54H20 DK
   To program the nRF54H20 DK, follow the programming instructions in the :ref:`nRF54H20 device guide <ug_nrf54h20_gs_sample>`.
   Programming the nRF54H20 DK with the |NCS| version earlier than v3.0.0 requires installing the `nrfutil device command <Installing and upgrading nRF Util commands_>`_ for the ``west flash`` command to work with this device.
   Starting with the |NCS| v3.1.0, the ``nrfutil device`` command is part of the :ref:`nRF Connect SDK toolchain bundle <requirements_toolchain>` and you get it when you :ref:`gs_installing_toolchain`.

.. _programming_params:

Optional programming parameters
*******************************

You can customize the basic ``west flash`` command in a variety of ways.
The following are most common in the |NCS|.
For more options, see Zephyr's :ref:`zephyr:west-flashing`.

.. _programming_params_no_erase:

Programming without ``--erase``
  As an alternative to the recommended ``west flash --erase``, you can also clear only those flash memory pages that are to be overwritten with the new application.
  With such approach, the old data in other areas will be retained.

  To erase only the areas of flash memory that are required for programming the new application, use the following command:

  .. code-block:: console

     west flash

  ..

.. _readback_protection_error:

Programming with ``--recover``
  Recovering a device typically refers to the process of resetting it to a known, working state, often by erasing and reprogramming its memory.
  This is usually done in the following cases:

  * The device is not functioning correctly.
  * The device is using a readback protection, such as the :term:`Access port protection mechanism (AP-Protect)`, offered by several Nordic Semiconductor SoCs or SiPs supported in the |NCS|.
    In such a case, you might get an error similar to the following message:

    .. code-block:: console

       ERROR: The operation attempted is unavailable due to readback protection in
       ERROR: your device. Please use --recover to unlock the device.

  Use the following command to recover your device:

  .. code-block:: console

     west flash --recover

  This command erases all user-available non-volatile memory and disables the readback protection mechanism, if enabled.

  .. toggle:: Background recovery commands

     The ``west flash --recover`` command uses one of the following background commands:

     * ``nrfjprog --recover`` for devices from nRF52, nRF53, and nRF91 Series.
     * ``nrfutil device recover`` for devices from nRF54H and nRF54L Series.
       For more information about how ``nrfutil device recover`` works, see the `nRF Util documentation <Recovering the device_>`_.
