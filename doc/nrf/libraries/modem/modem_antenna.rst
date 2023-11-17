.. _lib_modem_antenna:

Modem antenna
#############

.. contents::
   :local:
   :depth: 2

You can use this library to set the antenna configuration on an nRF9160 DK or a Thingy:91.

Overview
********

The library writes the MAGPIO and COEX0 configuration to the modem after the :ref:`nrfxlib:nrf_modem` has been initialized.
The configuration depends on the selected board and the used GNSS antenna type, onboard or external.

For an nRF9160 DK, the library configures the Low Noise Amplifier (LNA) depending on the GNSS antenna type.

For a Thingy:91, it configures the antenna matching network used with both LTE and GNSS.

Configuration
*************

Set the :kconfig:option:`CONFIG_MODEM_ANTENNA` Kconfig option to enable this library.

.. note::

   The library is enabled by default when building for targets ``nrf9160dk_nrf9160_ns`` and ``thingy91_nrf9160_ns``.

Use one of the following options to select the used antenna type:

* :kconfig:option:`CONFIG_MODEM_ANTENNA_GNSS_ONBOARD` - Selects the onboard GNSS antenna (default).
* :kconfig:option:`CONFIG_MODEM_ANTENNA_GNSS_EXTERNAL` - Selects an external GNSS antenna.

Use the following options to set custom AT commands for MAGPIO and COEX0 configuration:

* :kconfig:option:`CONFIG_MODEM_ANTENNA_AT_MAGPIO` - Sets custom MAGPIO configuration.
* :kconfig:option:`CONFIG_MODEM_ANTENNA_AT_COEX0` - Sets custom COEX0 configuration.

Dependencies
************

This library uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`
