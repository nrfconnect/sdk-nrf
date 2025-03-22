.. _ug_using_short_range_sample:

Using the Radio test (short-range) sample
#########################################

The Radio test sample is used to control the short-range device.
The Radio test firmware supports configuration of the short-range radio in specific modes and with various TX and RX parameters to test its performance.

For more information on the samples in |NCS|, see the following:

* Overall description: :ref:`radio_test` sample.
* Description of using PuTTY as the terminal application for controlling the :term:`Device Under Test (DUT)`: :ref:`Testing and debugging an application <test_and_optimize>`.
* Description of sub-commands for configuring the radio: :ref:`User interface for short-range Radio test <radio_test_ui>`.

.. note::

   The Radio test (short-range) runs on a different UART that is connected to the nRF5340 network core (VCOM0).
   For more information, see :ref:`ug_wifi_test_setup_vcom_settings`.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   ble_radio_test_firmware
   ble_radio_test_for_per_measurements
   thread_radio_testing
   thread_radio_test_for_per_measurements
