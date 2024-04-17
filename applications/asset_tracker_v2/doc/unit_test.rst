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

To run the unit test, you must navigate to the test directory of the respective internal module.
For example, to run the unit test for :ref:`asset_tracker_v2_debug_module`, navigate to :file:`asset_tracker_v2/tests/debug_module`.

The unit tests can be executed using west or Twister.
For more information, see :ref:`running_unit_tests`, and in particular the :ref:`example for nRF9160 DK <running_unit_tests_example_nrf9160>`.
