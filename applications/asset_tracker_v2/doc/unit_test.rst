.. _asset_tracker_unit_test:

Unit tests
###########

.. contents::
   :local:
   :depth: 2

The internal modules of the :ref:`asset_tracker_v2` application have a set of unit tests in the :file:`asset_tracker_v2/tests` folder.
Following are the modules that have unit tests:

* :ref:`asset_tracker_v2_debug_module` - :file:`asset_tracker_v2/src/modules/debug_module.c`
* :ref:`asset_tracker_v2_ui_module` - :file:`asset_tracker_v2/src/modules/ui_module.c`
* :ref:`asset_tracker_v2_location_module` - :file:`asset_tracker_v2/src/modules/location_module.c`
* JSON common library - :file:`asset_tracker_v2/src/cloud/cloud_codec/json_common.c`
* LwM2M codec helpers - :file:`asset_tracker_v2/src/cloud/cloud_codec/lwm2m/lwm2m_codec_helpers.c`
* LwM2M integration layer - :file:`asset_tracker_v2/src/cloud/lwm2m_integration/lwm2m_integration.c`
* nRF Cloud codec backend - :file:`asset_tracker_v2/src/cloud/cloud_codec/nrf_cloud/nrf_cloud_codec.c`

Running the unit test
*********************

To run the unit test, you must navigate to the test directory of the respective internal module.
For example, to run the unit test for :ref:`asset_tracker_v2_debug_module`, navigate to :file:`asset_tracker_v2/tests/debug_module`.
The unit tests can be executed using West or Twister.

Running unit tests using West
=============================

Enter the following west commands to execute the tests on different board targets:

* :ref:`zephyr:native_sim` board target:

.. code-block:: console

   west build -b native_sim -t run

* ``qemu_cortex_m3`` board target:

.. code-block:: console

   west build -b qemu_cortex_m3 -t run

Running unit tests using Twister
================================

Enter the following twister commands to execute the tests on different board targets:

* On both :ref:`zephyr:native_sim` and ``qemu_cortex_m3`` board targets:

.. code-block:: console

   twister -T .

* ``qemu_cortex_m3`` board target:

.. code-block:: console

   twister -T . -p qemu_cortex_m3

Running the unit tests on the nRF9160 DK
----------------------------------------

Enter the following command to execute the unit tests on nRF9160 DK:

.. code-block:: console

   twister -T . -p nrf9160dk_nrf9160_ns --device-testing --device-serial <serial port>

In this console snippet, ``serial port`` must be the port where you receive logs from the DK, normally the first port listed by ``nrfjprog --com``, for example ``/dev/ttyACM0``.

The :file:`testcase.yaml` file for that unit test must have the entry ``platform_allow: nrf9160dk_nrf9160_ns``.
See :file:`nrf/applications/asset_tracker_v2/tests/location_module/testcase.yaml` for an example.

Twister can also be used to see code coverage reports.
For more information about Twister, see the :ref:`zephyr:twister_script` documentation.

.. note::
   The Twister commands only work on Linux operating system.
