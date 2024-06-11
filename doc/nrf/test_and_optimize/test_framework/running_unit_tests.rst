.. _running_unit_tests:

Running unit tests
##################

Both Zephyr and the |NCS| support running unit tests using the following methods:

* :ref:`Twister <zephyr:twister_script>` - A test runner tool that is part of Zephyr, used for automating the execution of test cases.
  It can be used for both continuous integration and local testing, and it supports running tests on multiple platforms, including actual hardware devices, emulated platforms, and simulated environments.
  It can also be used to generate code coverage reports.
* :ref:`zephyr:west` - Which lets you build and run one specific unit test.
  This feature of west uses Twister under the hood.

In either case, to run the unit test, you must navigate to the test directory that includes the :file:`testcase.yaml` file.
This file includes information about available tests, their parameters, hardware requirements or dependencies, platform subsets, among others.
For more information about these files, read the Test Cases section in :ref:`Zephyr's documentation about Twister <zephyr:twister_script>`.

Because Twister is part of Zephyr, you need to provide the ``<Zephyr_path>`` to Zephyr SDK repository that was included in your |NCS| installation (in the step :ref:`cloning_the_repositories`).

To generate the test project and run the unit tests, locate the directory with the :file:`testcase.yaml` file and run the following command:

.. tabs::

   .. group-tab:: Twister (Windows)

      .. code-block:: console

         <Zephyr_path>/scripts/twister -T .

      This command will generate the test project and run it for :ref:`zephyr:native_sim` and ``qemu_cortex_m3`` boards.
      If you want to specify a board target, use the ``-p`` parameter and specify the *board_target*.
      For example, to run the unit test on ``qemu_cortex_m3``, use the following command:

      .. code-block:: console

         <Zephyr_path>/scripts/twister -T . -p qemu_cortex_m3

   .. group-tab:: Twister (Linux)

      .. code-block:: console

         <Zephyr_path>/scripts/twister -T .

      This command will generate the test project and run it for :ref:`zephyr:native_sim` and ``qemu_cortex_m3`` boards.
      If you want to specify a board target, use the ``-p`` parameter and specify the *board_target*.
      For example, to run the unit test on ``qemu_cortex_m3``, use the following command:

      .. code-block:: console

         <Zephyr_path>/scripts/twister -T . -p qemu_cortex_m3

   .. group-tab:: west

      .. parsed-literal::
         :class: highlight

         west build -b *board_target* -t run

      The ``-t run`` parameter tells west to run the default test target.
      For example, to run the unit test on :ref:`zephyr:native_sim` board, use the following command:

      .. code-block:: console

         west build -b native_sim -t run

.. _running_unit_tests_example_nrf9160:

Example: Running the unit tests on the nRF9160 DK
*************************************************

The :ref:`asset_tracker_v2` application provides :ref:`asset_tracker_unit_test` for several of its modules.
To run the unit test for the :ref:`asset_tracker_v2_debug_module`, complete the following steps:

1. |connect_kit|
   Take note of the serial port where you receive logs from the DK (this will be ``serial_port`` in the following command).
#. Navigate to :file:`asset_tracker_v2/tests/debug_module`, where the :file:`testcase.yaml` is located.
   If you check this file, it includes ``nrf9160dk/nrf9160/ns`` in the ``platform_allow:`` entry.
#. Enter the following command to execute the unit tests on nRF9160 DK:

   .. tabs::

      .. group-tab:: Twister (Windows)

         .. code-block:: console

            <Zephyr_path>/scripts/twister -T . -p nrf9160dk/nrf9160/ns --device-testing --device-serial <serial_port>

      .. group-tab:: Twister (Linux)

         .. code-block:: console

            <Zephyr_path>/scripts/twister -T . -p nrf9160dk/nrf9160/ns --device-testing --device-serial <serial_port>

      .. group-tab:: west

         .. code-block:: console

            west build -b nrf9160dk/nrf9160/ns -t run
