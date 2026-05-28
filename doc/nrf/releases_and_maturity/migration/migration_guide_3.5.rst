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

Clock control nrf deprecation
=============================

.. toggle::

   The :ref:`clock_control_api` driver has been updated for the following clocks on nRF52, nRF53, nRF91 and nRF54L Series devices:

   * HFCLK
   * LFCLK
   * XO
   * XO24M
   * HFCLK192M
   * HFCLKAUDIO

   To restore the legacy driver implementation set :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF` to ``y``.

   The following steps are required to migrate code from |NCS| v3.4.0 to |NCS| v3.5.0:

   1. Individual clocks controlled by the application must now be explicitly enabled in the application-specific DTS overlay file or board-specific overlay file so that the corresponding driver is enabled.
      See the following example:

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

   2. The following Kconfig option names have changed and must be updated in the application configuration file:
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC_CALIBRATION`->:kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_K32SRC_RC_CALIBRATION`
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_DRIVER_CALIBRATION`->:kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_DRIVER_CALIBRATION`
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_LF_ALWAYS_ON`->:kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_LF_ALWAYS_ON`
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_PERIOD`->:kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_PERIOD`
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_MAX_SKIP`->:kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_MAX_SKIP`
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_TEMP_DIFF`->:kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_TEMP_DIFF`
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_CALIBRATION_DEBUG`->:kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_CALIBRATION_DEBUG`
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_USES_TEMP_SENSOR`->:kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_USES_TEMP_SENSOR`
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION`->:kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_HFINT_CALIBRATION`
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION_PERIOD`->:kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_HFINT_CALIBRATION_PERIOD`
      :kconfig:option:`CONFIG_NRFX_CLOCK_USE_LFRC_CALIBRATION`->:kconfig:option:`CONFIG_NRFX_CLOCK_LFCLK_USE_LFRC_CALIBRATION`
      :kconfig:option:`CONFIG_NRFX_CLOCK_LF_CAL_ENABLED`->:kconfig:option:`CONFIG_CLOCK_CONTROL_NRFX_K32SRC_RC_CALIBRATION`

   3. The following Kconfig options must be moved to DTS:
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_K32SRC_FREQUENCY` -> DTS: ``nordic,nrfx-clock-lfclk``: ``k32src-frequency``
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_SOURCE` -> DTS: ``nordic,nrfx-clock-lfclk``: ``k32src``
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_ACCURACY_PPM` -> DTS: ``nordic,nrfx-clock-lfclk``: ``k32src-accuracy-ppm``
      :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_ACCURACY` -> DTS: ``nordic,nrfx-clock-lfclk``: ``k32src-accuracy-ppm``
      :kconfig:option:`CONFIG_NRFX_CLOCK_LFXO_TWO_STAGE_ENABLED` -> DTS: ``nordic,nrfx-clock-lfclk``: ``k32src``

   4. The following API usage changes must be made:

     * ``mgr`` - Onoff manager created for ``nordic,nrf-clock``, obtained using ``z_nrf_clock_control_get_onoff``
     * ``dev`` - ``nordic,nrf-clock`` compatible device.
     * ``sys`` - Subsystem for ``nordic,nrf-clock``. It is no longer used in new clocks implementation.
     * ``new_dev`` - Device corresponding to the ``sys`` used before the change. It must becompatible with one of the following nodes:
        * ``nordic,nrfx-clock-lfclk``
        * ``nordic,nrfx-clock-hfclk``
        * ``nordic,nrfx-clock-xo``
        * ``nordic,nrfx-clock-hfclk192m``
        * ``nordic,nrfx-clock-xo24m``
        * ``nordic,nrfx-clock-hfclkaudio``

     .. code-block:: c

         // Old api use (deprecated)
         z_nrf_clock_calibration_init(&mgrs);    //1
         onoff_release(mgr)                      //2
         onoff_request(mgr, &cli);               //3
         onoff_cancel_or_release(mgr, &cli);     //4
         clock_control_on(dev,sys)               //5
         clock_control_off(dev,sys)              //6
         clock_control_async_on(dev,sys)         //7
         clock_control_get_status(dev,sys)       //8
         z_nrf_clock_control_get_onoff(sys)      //9

         //New api use
         z_nrf_clock_calibration_init();                             //1
         nrf_clock_control_release(new_dev, NULL);                   //2
         nrf_clock_control_request(new_dev, NULL, &cli);             //3
         nrf_clock_control_cancel_or_release(new_dev, NULL, &cli);   //4
         clock_control_on(new_dev, NULL)                             //5
         clock_control_off(new_dev, NULL)                            //6
         clock_control_async_on(new_dev, NULL)                       //7
         clock_control_get_status(new_dev, NULL)                     //8
         // remove all uses of z_nrf_clock_control_get_onoff         //9

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
