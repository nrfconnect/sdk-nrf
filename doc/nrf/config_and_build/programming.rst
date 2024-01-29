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

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      You need to set up a build configuration before you program a device in the |nRFVSC|.

      Complete the following steps:

      1. Connect the development kit to your PC using a USB cable.
      #. Power on the development kit.
      #. Make sure you are familiar with :ref:`programming_hw`.
      #. Open the nRF Connect extension in |VSC| by clicking its icon in the :guilabel:`Activity Bar`.
      #. In the extension's :guilabel:`Actions View`, click on :guilabel:`Flash`.

      For instructions about programming with the |nRFVSC|, see `How to flash an application`_.

      If you want to program with custom options or scripts, read about `Binding custom tasks to actions`_ in the extension documentation.

      For information about how to flash and erase in the |nRFVSC|, read about the `Application-specific flash options`_.

   .. group-tab:: Command line

      Complete the following steps:

      1. Connect the development kit to your PC using a USB cable.
      #. Power on the development kit.
      #. Make sure you are familiar with :ref:`programming_hw`.
      #. Program the application to the kit using the following command:

         .. code-block:: console

            west flash --erase

         This command erases the full flash memory before programming, which is the recommended approach.
         If the application depends on other flash memory areas (for example, if it uses the :ref:`zephyr:settings_api` partition where bonding information is stored), erasing the full kit before programming ensures that these areas are updated with the new content.
         You can also :ref:`program without erasing <programming_params_no_erase>`.

         The ``west flash`` command automatically resets the kit and starts the application.

      For more information on programming using the command line, see :ref:`Flashing <zephyr:west-flashing>` in the Zephyr documentation.


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

.. _programming_params_no_erase:

Programming without ``--erase``
  As an alternative to the recommended ``west flash --erase``, you can also clear only those flash memory pages that are to be overwritten with the new application.
  With such approach, the old data in other areas will be retained.

  To erase only the areas of flash memory that are required for programming the new application, use the following command:

  .. code-block:: console

     west flash

Programming with ``--recover``
  Several Nordic Semiconductor SoCs or SiPs supported in the |NCS| offer an implementation of the :term:`Access port protection mechanism (AP-Protect)`, a form of readback protection.
  To disable the AP-Protect, you must recover your device.
  This is particularly important for multi-core devices.

  Use the following command:

  .. code-block:: console

     west flash --recover

  This command uses ``nrfjprog --recover`` command in the background.
  It erases all user available non-volatile memory and disables the readback protection mechanism if enabled.
