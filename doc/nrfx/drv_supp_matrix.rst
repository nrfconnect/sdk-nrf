.. _nrfx_drv_supp_matrix:

Driver support matrix
=====================

The following matrix shows which drivers are supported by specific Nordic SoCs.

.. role:: green
.. role:: red

====================  ============  =================  ==========  ==========  ==========  ==========  ==========
Driver                nRF51 Series  nRF52810/nRF52811  nRF52832    nRF52833    nRF52840    nRF5340     nRF9160
====================  ============  =================  ==========  ==========  ==========  ==========  ==========
:ref:`nrf_aar`        :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_acl`        :red:`✖`      :red:`✖`           :red:`✖`    :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_adc`        :green:`✔`    :red:`✖`           :red:`✖`    :red:`✖`    :red:`✖`    :red:`✖`    :red:`✖`
:ref:`nrf_bprot`      :red:`✖`      :green:`✔`         :green:`✔`  :red:`✖`    :red:`✖`    :red:`✖`    :red:`✖`
:ref:`nrf_cache`      :red:`✖`      :red:`✖`           :red:`✖`    :red:`✖`    :red:`✖`    :green:`✔`  :red:`✖`
:ref:`nrf_ccm`        :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_clock`      :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_comp`       :red:`✖`      :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_systick`    :red:`✖`      :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_dcnf`       :red:`✖`      :red:`✖`           :red:`✖`    :red:`✖`    :red:`✖`    :green:`✔`  :red:`✖`
:ref:`nrf_dppi`       :red:`✖`      :red:`✖`           :red:`✖`    :red:`✖`    :red:`✖`    :green:`✔`  :green:`✔`
:ref:`nrf_ecb`        :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_egu`        :red:`✖`      :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_ficr`       :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_fpu`        :red:`✖`      :red:`✖`           :red:`✖`    :red:`✖`    :red:`✖`    :green:`✔`  :red:`✖`
:ref:`nrf_gpio`       :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_gpiote`     :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_i2s`        :red:`✖`      :red:`✖`           :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_ipc`        :red:`✖`      :red:`✖`           :red:`✖`    :red:`✖`    :red:`✖`    :green:`✔`  :green:`✔`
:ref:`nrf_kmu`        :red:`✖`      :red:`✖`           :red:`✖`    :red:`✖`    :red:`✖`    :green:`✔`  :green:`✔`
:ref:`nrf_lpcomp`     :green:`✔`    :red:`✖`           :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_mpu`        :green:`✔`    :red:`✖`           :red:`✖`    :red:`✖`    :red:`✖`    :red:`✖`    :red:`✖`
:ref:`nrf_mutex`      :red:`✖`      :red:`✖`           :red:`✖`    :red:`✖`    :red:`✖`    :green:`✔`  :red:`✖`
:ref:`nrf_mwu`        :red:`✖`      :red:`✖`           :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`    :red:`✖`
:ref:`nrf_nfct`       :red:`✖`      :red:`✖`           :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_nvmc`       :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_pdm`        :red:`✖`      :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_power`      :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_ppi`        :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`    :red:`✖`
:ref:`nrf_pwm`        :red:`✖`      :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_qdec`       :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_qspi`       :red:`✖`      :red:`✖`           :red:`✖`    :red:`✖`    :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_radio`      :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_reset`      :red:`✖`      :red:`✖`           :red:`✖`    :red:`✖`    :red:`✖`    :green:`✔`  :red:`✖`
:ref:`nrf_rng`        :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_rtc`        :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_saadc`      :red:`✖`      :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_spi`        :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`    :red:`✖`
:ref:`nrf_spim`       :red:`✖`      :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_spis`       :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_spu`        :red:`✖`      :red:`✖`           :red:`✖`    :red:`✖`    :red:`✖`    :green:`✔`  :green:`✔`
:ref:`nrf_temp`       :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_timer`      :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_twi`        :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`    :red:`✖`
:ref:`nrf_twim`       :red:`✖`      :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_twis`       :red:`✖`      :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_uart`       :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`    :red:`✖`
:ref:`nrf_uarte`      :red:`✖`      :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
:ref:`nrf_usbd`       :red:`✖`      :red:`✖`           :red:`✖`    :green:`✔`  :green:`✔`  :green:`✔`  :red:`✖`
:ref:`nrf_vmc`        :red:`✖`      :red:`✖`           :red:`✖`    :red:`✖`    :red:`✖`    :green:`✔`  :green:`✔`
:ref:`nrf_wdt`        :green:`✔`    :green:`✔`         :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`  :green:`✔`
====================  ============  =================  ==========  ==========  ==========  ==========  ==========
