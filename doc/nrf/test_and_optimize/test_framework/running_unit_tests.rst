.. _running_unit_tests:

Running unit tests
##################

.. contents::
   :local:
   :depth: 2

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

   .. group-tab:: twister

      .. code-block:: console

         <Zephyr_path>/scripts/twister -T .

      This command will generate the test project and run it for the board targets defined in :file:`testcase.yaml` file.
      If you want to specify a board target, use the ``-p`` parameter.
      For example, to run the unit test on ``qemu_cortex_m3``, use the following command:

      .. code-block:: console

         <Zephyr_path>/scripts/twister -T . -p qemu_cortex_m3

   .. group-tab:: west

      .. parsed-literal::
         :class: highlight

         west build -b *board_target*
         west flash

      The ``west flash`` command tells west to run the tests.
      For example, to run the unit test on :zephyr:board:`native_sim` board, use the following command:

      .. code-block:: console

         west build -b native_sim
         west flash
