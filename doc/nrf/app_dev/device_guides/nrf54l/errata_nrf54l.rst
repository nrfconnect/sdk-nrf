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
* `nRF54LM20A Errata`_
* `nRF54LM20B Errata`_

Some of the anomalies can be fixed by applying workarounds directly in the software components, requiring minimal or zero effort from the end user.
Refer to the following table for the current workaround list:

.. tabs::

   .. tab:: nRF54L05

      .. list-table:: Anomaly workaround status
         :header-rows: 1
         :align: left
         :widths: auto

         * - Anomaly ID
           - Status
         * - 1
           - Not implemented
         * - 6
           - Not implemented
         * - 7
           - **Enabled**
         * - 8
           - **Enabled**
         * - 16
           - **Enabled**
         * - 17
           - Not implemented
         * - 18
           - Not implemented
         * - 20
           - Not implemented
         * - 21
           - Not implemented
         * - 22
           - Not implemented
         * - 23
           - Not implemented
         * - 24
           - Not implemented
         * - 25
           - Not implemented
         * - 26
           - Not implemented
         * - 30
           - Available when ``CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION=y`` :sup:`1`
         * - 31
           - **Enabled**
         * - 32
           - **Enabled**
         * - 37
           - **Enabled**
         * - 39
           - **Enabled**
         * - 41
           - Not implemented
         * - 44
           - Not implemented
         * - 47
           - Not implemented
         * - 48
           - **Enabled**
         * - 49
           - Not implemented
         * - 50
           - Not applicable
         * - 59
           - Not applicable
         * - 63
           - Not applicable
         * - 69
           - Not applicable
         * - 102
           - Not applicable
         * - 104
           - Not applicable
         * - 105
           - Not applicable

   .. tab:: nRF54L10

      .. list-table:: Anomaly workaround status
         :header-rows: 1
         :align: left
         :widths: auto

         * - Anomaly ID
           - Status
         * - 1
           - Not implemented
         * - 6
           - Not implemented
         * - 7
           - **Enabled**
         * - 8
           - **Enabled**
         * - 16
           - **Enabled**
         * - 17
           - Not implemented
         * - 18
           - Not implemented
         * - 20
           - Not implemented
         * - 21
           - Not implemented
         * - 22
           - Not implemented
         * - 23
           - Not implemented
         * - 24
           - Not implemented
         * - 25
           - Not implemented
         * - 26
           - Not implemented
         * - 30
           - Available when ``CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION=y`` :sup:`1`
         * - 31
           - **Enabled**
         * - 32
           - **Enabled**
         * - 37
           - **Enabled**
         * - 39
           - **Enabled**
         * - 41
           - Not implemented
         * - 44
           - Not implemented
         * - 47
           - Not implemented
         * - 48
           - **Enabled**
         * - 49
           - Not implemented
         * - 50
           - Not applicable
         * - 59
           - Not applicable
         * - 63
           - Not applicable
         * - 69
           - Not applicable
         * - 102
           - Not applicable
         * - 104
           - Not applicable
         * - 105
           - Not applicable

   .. tab:: nRF54L15

      .. list-table:: Anomaly workaround status
         :header-rows: 1
         :align: left
         :widths: auto

         * - Anomaly ID
           - Status
         * - 1
           - Not implemented
         * - 6
           - Not implemented
         * - 7
           - **Enabled**
         * - 8
           - **Enabled**
         * - 16
           - **Enabled**
         * - 17
           - Not implemented
         * - 18
           - Not implemented
         * - 20
           - Not implemented
         * - 21
           - Not implemented
         * - 22
           - Not implemented
         * - 23
           - Not implemented
         * - 24
           - Not implemented
         * - 25
           - Not implemented
         * - 26
           - Not implemented
         * - 30
           - Available when ``CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION=y`` :sup:`1`
         * - 31
           - **Enabled**
         * - 32
           - **Enabled**
         * - 37
           - **Enabled**
         * - 39
           - **Enabled**
         * - 41
           - Not implemented
         * - 44
           - Not implemented
         * - 47
           - Not implemented
         * - 48
           - **Enabled**
         * - 49
           - Not implemented
         * - 50
           - Not applicable
         * - 59
           - Not applicable
         * - 63
           - Not applicable
         * - 69
           - Not applicable
         * - 102
           - Not applicable
         * - 104
           - Not applicable
         * - 105
           - Not applicable

   .. tab:: nRF54LM20A / nRF54LM20B

      .. list-table:: Anomaly workaround status
         :header-rows: 1
         :align: left
         :widths: auto

         * - Anomaly ID
           - nRF54LM20A
           - nRF54LM20B
         * - 1
           - Not applicable
           - Not applicable
         * - 6
           - Not applicable
           - Not applicable
         * - 7
           - **Enabled**
           - **Enabled**
         * - 8
           - **Enabled**
           - **Enabled**
         * - 16
           - **Enabled**
           - **Enabled**
         * - 17
           - Not applicable
           - Not applicable
         * - 18
           - Not applicable
           - Not applicable
         * - 20
           - Not applicable
           - Not applicable
         * - 21
           - Not applicable
           - Not applicable
         * - 22
           - Not applicable
           - Not applicable
         * - 23
           - Not applicable
           - Not applicable
         * - 24
           - Not applicable
           - Not applicable
         * - 25
           - Not applicable
           - Not applicable
         * - 26
           - Not implemented
           - Not implemented
         * - 30
           - Available when ``CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION=y`` :sup:`1`
           - Available when ``CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION=y`` :sup:`1`
         * - 31
           - **Enabled**
           - **Enabled**
         * - 32
           - **Enabled**
           - **Enabled**
         * - 37
           - **Enabled**
           - **Enabled**
         * - 39
           - **Enabled**
           - **Enabled**
         * - 41
           - Not applicable
           - Not applicable
         * - 44
           - Not implemented
           - Not implemented
         * - 47
           - Not implemented
           - Not implemented
         * - 48
           - Not applicable
           - Not applicable
         * - 49
           - Not implemented
           - Not implemented
         * - 50
           - Not implemented
           - Not implemented
         * - 59
           - Not implemented
           - Not implemented
         * - 63
           - Not implemented
           - Not implemented
         * - 69
           - **Enabled**
           - **Enabled**
         * - 102
           - Not implemented
           - Not implemented
         * - 104
           - Not implemented
           - Not implemented
         * - 105
           - Not implemented
           - Not implemented

:sup:`1` You can set the anomaly workaround appliance frequency using the :kconfig:option:`CONFIG_CLOCK_CONTROL_NRF_HFINT_CALIBRATION_PERIOD` Kconfig option.
Its value must be tuned in accordance to application-specific operating temperature and expected power consumption.
