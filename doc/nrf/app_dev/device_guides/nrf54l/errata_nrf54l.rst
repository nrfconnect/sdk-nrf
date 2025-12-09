.. _ug_errata_nrf54l:

nRF54L Errata support in |NCS|
##############################

.. contents::
   :local:
   :depth: 2

This page summarizes the nRF54L Series anomalies and their workarounds that are currently implemented in the |NCS|.

nRF54L Series Errata documents contain descriptions of identified hardware anomalies for the relevant SoCs:

* `nRF54L05 Errata`_
* `nRF54L10 Errata`_
* `nRF54L15 Errata`_

Some of the anomalies can be fixed by applying workarounds directly in the software components, requiring minimal or zero effort from the end user.

.. list-table:: Anomaly workaround status for nRF54L Series devices
    :header-rows: 1
    :align: center
    :widths: auto

    * - Anomaly ID
      - nRF54L05
      - nRF54L10
      - nRF54L15
    * - 1
      - \-
      - \-
      - \-
    * - 6
      - \-
      - \-
      - \-
    * - 7
      - Enabled
      - Enabled
      - Enabled
    * - 8
      - Enabled
      - Enabled
      - Enabled
    * - 16
      - Enabled
      - Enabled
      - Enabled
    * - 17
      - \-
      - \-
      - \-
    * - 18
      - \-
      - \-
      - \-
    * - 20
      - \-
      - \-
      - \-
    * - 21
      - \-
      - \-
      - \-
    * - 22
      - \-
      - \-
      - \-
    * - 23
      - \-
      - \-
      - \-
    * - 24
      - \-
      - \-
      - \-
    * - 25
      - \-
      - \-
      - \-
    * - 26
      - \-
      - \-
      - \-
    * - 30
      - Available when ``CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION=y`` :sup:`1`
      - Available when ``CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION=y`` :sup:`1`
      - Available when ``CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION=y`` :sup:`1`
    * - 31
      - Enabled
      - Enabled
      - Enabled
    * - 33
      - \-
      - \-
      - \-
    * - 37
      - Enabled
      - Enabled
      - Enabled
    * - 39
      - Enabled
      - Enabled
      - Enabled
    * - 41
      - \-
      - \-
      - \-
    * - 44
      - \-
      - \-
      - \-
    * - 47
      - \-
      - \-
      - \-
    * - 48
      - Enabled
      - Enabled
      - Enabled
    * - 49
      - \-
      - \-
      - \-

:sup:`1` You can set the anomaly workaround appliance frequency using the :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION_PERIOD` Kconfig option.
Its value must be tuned in accordance to application-specific operating temperature and expected power consumption.
