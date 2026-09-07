.. _peripheral_custom_drivers:

Peripheral custom drivers
#########################

Peripheral custom drivers combine one or more hardware peripherals to provide functionality beyond a single standardized peripheral interface.
They can use `nrfx`_ drivers and the `nrfx_gppi helper layer`_, while integrating with Zephyr's :ref:`zephyr:device_model_api`.
The following pages describe the peripheral custom drivers available in the |NCS|.

.. toctree::
   :maxdepth: 1
   :caption: Subpages:

   pulse_meas
   ppi_seq
   ppi_seq_i2c_spi
