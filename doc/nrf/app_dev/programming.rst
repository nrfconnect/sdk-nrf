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

The flash command programs all cores by default, both in the |nRFVSC| and on the command line.
If you want to program only one selected core, use ``west flash`` on the command line and :ref:`specify the domain <zephyr:west-multi-domain-flashing>`.

.. _programming_hw:

Hardware-specific programming steps
***********************************

Some hardware has platform-specific requirements that must be met before you program the device.
For additional information, check the :ref:`user guide for the hardware platform <device_guides>` that you are using.

For example, if you are working with an nRF9160 DK, you need to select the correct position of the :ref:`board controller <nrf9160_ug_intro>` switch **SW10** before you program the application to your development kit.
Programming to Thingy:91 also requires a :ref:`similar step <building_pgming>`, but using a different switch (**SW2**).

Programming the nRF52840 Dongle
  To program the nRF52840 Dongle instead of a development kit, follow the programming instructions in :ref:`zephyr:nrf52840dongle_nrf52840` or use the `nRF Connect Programmer app <Programming the nRF52840 Dongle_>`_.

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
