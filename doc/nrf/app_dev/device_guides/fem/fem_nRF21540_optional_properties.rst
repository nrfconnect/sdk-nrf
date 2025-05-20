.. _ug_radio_fem_optional_properties:

Optional FEM properties for nRF21540 GPIO and GPIO+SPI
######################################################

.. contents::
   :local:
   :depth: 2

The following properties are optional for use with the :ref:`GPIO <ug_radio_fem_nrf21540_gpio>` and :ref:`GPIO+SPI <ug_radio_fem_nrf21540_spi_gpio>` modes:

* Properties that control the other pins:

  * ``ant-sel-gpios`` - GPIO characteristic of the device that controls the ``ANT_SEL`` signal of the nRF21540.
  * ``mode-gpios`` - GPIO characteristic of the device that controls the ``MODE`` signal of the nRF21540.

    The ``MODE`` signal of the nRF21540 switches between two values of PA gain.
    The pin can either be set to a fixed state on initialization, which results in a constant PA gain, or it can be switched in run-time by the protocol drivers to match the transmission power requested by the application.

    To enable run-time ``MODE`` pin switching, you must enable :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_RUNTIME_PA_GAIN_CONTROL`.
    See also :ref:`ug_radio_fem_sw_support_mpsl_fem_output` below.

    .. note::
       The state of the ``MODE`` pin is selected based on the available PA gains and the required transmission power.
       To achieve reliable performance, :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTA` and :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTB` must reflect the content of the nRF21540 registers.
       Their default values match chip production defaults.
       For details, see the `nRF21540 Product Specification`_.

    If the run-time ``MODE`` pin switching is disabled, the PA gain is constant and equal to :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB`.

* Properties that control the timing of interface signals:

  * ``tx-en-settle-time-us`` - Minimal time interval between asserting the ``TX_EN`` signal and starting the radio transmission, in microseconds.
  * ``rx-en-settle-time-us`` - Minimal time interval between asserting the ``RX_EN`` signal and starting the radio transmission, in microseconds.

    .. note::
        Values for these two properties cannot be higher than the Radio Ramp-Up time defined by :c:macro:`TX_RAMP_UP_TIME` and :c:macro:`RX_RAMP_UP_TIME`.
        If the value is too high, the radio driver will not work properly and will not control FEM.
        Moreover, setting a value that is lower than the default value can cause disturbances in the radio transmission, because FEM may be triggered too late.

  * ``pdn-settle-time-us`` - Time interval before the PA or LNA activation reserved for the FEM ramp-up, in microseconds.
  * ``trx-hold-time-us`` - Time interval for which the FEM is kept powered up after the event that triggers the PDN deactivation, in microseconds.

  The default values of these properties are appropriate for default hardware and most use cases.
  You can override them if you need additional capacitors, for example when using custom hardware.
  In such cases, add the property name under the required properties in the devicetree node and set a new custom value.

  .. note::
    These values have some constraints.
    For details, see `nRF21540 Product Specification`_.

.. _ug_radio_fem_sw_support_mpsl_fem_output:

Setting the FEM output power with nRF21540
******************************************

nRF21540 implementations have the gain set to ``10`` by default.
You can set a different gain value to use through the :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB`  option, but it has to match the value of one of the POUTA (:kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTA` ) or POUTB (:kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTB`) gains.

.. caution::
   :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTA` and :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_TX_GAIN_DB_POUTB` are by default set to ``20`` and ``10`` and these are factory-precalibrated gain values.
   Do not change these values, unless POUTA and POUTB were calibrated to different values on specific request.

To enable runtime control of the gain, set the :kconfig:option:`CONFIG_MPSL_FEM_NRF21540_RUNTIME_PA_GAIN_CONTROL` to ``y``.
This option makes the gain of the FEM to be adjusted dynamically during runtime, depending on the power requested by the protocol driver for each transmission.

The following differences apply for both nRF21540 implementations:

* For the nRF21540 GPIO implementation, you must enable the **MODE** pin in devicetree.
* For the nRF21540 GPIO+SPI implementation, no additional configuration is needed as the gain setting is transmitted over the SPI bus to the nRF21540.
