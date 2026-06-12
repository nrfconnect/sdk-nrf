.. _migration_3.5:

Migration notes for |NCS| v3.5.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v3.4.0 to |NCS| v3.5.0.

.. HOWTO
   Add changes in the following format:
   Component (for example, application, sample or libraries)
   *********************************************************
   .. toggle::
      * Change1 and description
      * Change2 and description

.. _migration_3.5_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Samples and applications
========================

This section describes the changes related to samples and applications.

|no_changes_yet_note|

Libraries
=========

This section describes the changes related to libraries.

.. toggle::

   * :ref:`lib_location` library:

     * The library now always uses the chosen ``zephyr,wifi`` node to find the used Wi-Fi device.
       If your application uses the deprecated ``ncs,location-wifi`` node, you need to change it to use the ``zephyr,wifi`` node instead:

       .. code-block:: dts

          chosen {
                  zephyr,wifi = &mywifi;
          };

Drivers
=======

This section describes the changes related to drivers.

Clock control nrf deprecation
-----------------------------

.. toggle::

   The :ref:`clock_control_api` driver has been updated for the following clocks on nRF52, nRF53, nRF91, and nRF54L Series devices:

   * HFCLK
   * LFCLK
   * XO
   * XO24M
   * HFCLK192M
   * HFCLKAUDIO

   To restore the legacy driver implementation, set :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF` to ``y``.

   To migrate your code from |NCS| v3.4.0 to |NCS| v3.5.0, complete the following steps:

   1. Enable each application-controlled clock in the application-specific or board-specific devicetree overlay file.

      This enables the corresponding clock driver.
      For example:

      .. code-block:: dts

          /* if nRF54L XO is to be controlled */
          &xo {
              status = "okay";
          };

          /* if nRF52, nRF53 HFCLK is to be controlled */
          &hfclk {
              status = "okay";
          };

          /* if nRF52, nRF53, nRF91 or nRF54L LFCLK is to be controlled */
          &lfclk {
              status = "okay";
          }

          /* if HFCLK192M is to be controlled */
          &hfclk192m {
              status = "okay";
          }

          /* if XO24M is to be controlled */
          &xo24m {
              status = "okay";
          }

          /* if HFCLKAUDIO is to be controlled */
          &hfclkaudio {
              status = "okay";
          }

   #. Update the renamed Kconfig options in your application configuration file:

      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION` with :kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_K32SRC_RC_CALIBRATION`.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_DRIVER_CALIBRATION` with :kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_DRIVER_CALIBRATION`.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_LF_ALWAYS_ON` with :kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_LF_ALWAYS_ON`.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD` with :kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_PERIOD`.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP` with :kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_MAX_SKIP`.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_TEMP_DIFF` with :kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_TEMP_DIFF`.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_DEBUG` with :kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_DEBUG`.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_USES_TEMP_SENSOR` with :kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_USES_TEMP_SENSOR`.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION` with :kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_HFINT_CALIBRATION`.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION_PERIOD` with :kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_HFINT_CALIBRATION_PERIOD`.
      * Replace :kconfig:option:`CONFIG_NRFX_CLOCK_USE_LFRC_CALIBRATION` with :kconfig:option:`CONFIG_NRFX_CLOCK_LFCLK_USE_LFRC_CALIBRATION`.
      * Replace :kconfig:option:`CONFIG_NRFX_CLOCK_LF_CAL_ENABLED` with :kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_K32SRC_RC_CALIBRATION`.

   #. Move the following Kconfig options to the ``nordic,nrfx-clock-lfclk`` Devicetree node:

      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_FREQUENCY` with the ``k32src-frequency`` property.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_SOURCE` with the ``k32src`` property.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_ACCURACY_PPM` with the ``k32src-accuracy-ppm`` property.
      * Replace :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_ACCURACY` with the ``k32src-accuracy-ppm`` property.
      * Replace :kconfig:option:`CONFIG_NRFX_CLOCK_LFXO_TWO_STAGE_ENABLED` with the ``k32src`` property.

   #. Update your application to use the new clock control API.

      Use the following mapping when you update the API calls:

      * ``mgr`` is the on-off manager created for ``nordic,nrf-clock`` and obtained using ``z_nrf_clock_control_get_onoff``.
      * ``dev`` is the device compatible with ``nordic,nrf-clock``.
      * ``sys`` is the subsystem for ``nordic,nrf-clock``. The new clocks implementation does not use it.
      * ``new_dev`` is the device that corresponds to the previously used ``sys`` value.
        It must be compatible with one of the following nodes:

        * ``nordic,nrfx-clock-lfclk``
        * ``nordic,nrfx-clock-hfclk``
        * ``nordic,nrfx-clock-xo``
        * ``nordic,nrfx-clock-hfclk192m``
        * ``nordic,nrfx-clock-xo24m``
        * ``nordic,nrfx-clock-hfclkaudio``

      The following example shows the deprecated API usage and the corresponding new API usage:

      .. code-block:: c

         // Old API usage (deprecated)
         z_nrf_clock_calibration_init(&mgrs);    //1
         onoff_release(mgr)                      //2
         onoff_request(mgr, &cli);               //3
         onoff_cancel_or_release(mgr, &cli);     //4
         clock_control_on(dev,sys)               //5
         clock_control_off(dev,sys)              //6
         clock_control_async_on(dev,sys)         //7
         clock_control_get_status(dev,sys)       //8
         z_nrf_clock_control_get_onoff(sys)      //9

         // New API usage
         z_nrf_clock_calibration_init();                             //1
         nrf_clock_control_release(new_dev, NULL);                   //2
         nrf_clock_control_request(new_dev, NULL, &cli);             //3
         nrf_clock_control_cancel_or_release(new_dev, NULL, &cli);   //4
         clock_control_on(new_dev, NULL)                             //5
         clock_control_off(new_dev, NULL)                            //6
         clock_control_async_on(new_dev, NULL)                       //7
         clock_control_get_status(new_dev, NULL)                     //8
         // Remove all uses of z_nrf_clock_control_get_onoff         //9

.. _migration_3.5_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Build and configuration system
==============================

|no_changes_yet_note|

Samples and applications
========================

This section describes the changes related to samples and applications.

|no_changes_yet_note|

Libraries
=========

This section describes the changes related to libraries.

|no_changes_yet_note|
