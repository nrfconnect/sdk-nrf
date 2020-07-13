.. _ug_radio_fem:

Working with radio front-end modules
####################################

The |NCS| provides support for using a front-end module (FEM) for radio on a target board.
Front-end modules usually consist of a power amplifier (PA) used for transmission and
low-noise amplifier (LNA) used for reception. Front-end modules allow extension of radio
range.

Methods of using FEM described by this chapter cover protocol drivers using FEM support
provided by the MPSL library only. This is true also when an application uses just one
protocol but benefits from features provided by the MPSL. Please refer to a protocol driver
to check if it uses FEM support provided by the MPSL.

The MPLS library provides an API to configure FEM (TODO: link here). However |NCS| provides
a friendly wrapper which configures FEM based on devicetree and Kconfig information.
To enable FEM support there must be a node labeled `nrf_radio_fem` within device tree.
It can be provided by target board device tree file or by an overlay.

.. _ug_radio_fem_nrf21540_gpio:

Support for nRF21540 in gpio mode
*********************************

To use nRF21540 in gpio mode device tree need to contain node as shown below.

/ {
    nrf_radio_fem: my_fem {
        compatible = "nordic,nrf21540_gpio";
        tx-en-pin  = < 13 >;
        rx-en-pin  = < 14 >;
        pdn-pin    = < 15 >;
    };
};

Provided pin numbers and node name `my_fem` are just exemplary. Software FEM
support controls `TX_EN`, `RX_EN` and `PDN` pins of the nRF21540. State of other
control pins should be setby other means according to nRF21540 Product Specification
what is out of scope of this user guide.

Required properties:
`tx-en-pin` Pin number of a SOC controlling `TX_EN` signal of the nRF21540
`rx-en-pin` Pin number a SOC controlling `RX_EN` signal of the nRF21540
`pdn-pin` Pin number of a SOC controlling `PDN` signal of the nRF21540

Optional properties:

`tx-en-settle-time-us`, `rx-en-settle-time-us`, `pdn-settle-time-us`,
`trx-hold-time-us` control timing of interface signals. Provided default values are
appropriate for most cases. If overriding them is necessary please put custom value
just below pin mapping properties. For constraints of these values please refer to
nRF21540 Product Specification.

`tx-gain-db`, `rx-gain-db` inform protocol drivers about gain provided by the nRF21540.
The way of writing gain information into nRF21540 is currently out of scope.

Kconfig parameters:

`MPSL_FEM_NRF21540_GPIO_GPIOTE_TX_EN`, `MPSL_FEM_NRF21540_GPIO_GPIOTE_RX_EN`,
`MPSL_FEM_NRF21540_GPIO_GPIOTE_PDN` serve to assign unique GPIOTE channel numbers
to be used exclusively by the FEM driver.

`MPSL_FEM_NRF21540_GPIO_PPI_CHANNEL_0`, `MPSL_FEM_NRF21540_GPIO_PPI_CHANNEL_1`,
`MPSL_FEM_NRF21540_GPIO_PPI_CHANNEL_2` serve to assign unique PPI channel numbers
to be used exclusively by the FEM driver.

Support for SKY66112-11
***********************

To use SKY66112-11 device tree need to contain node as shown below.

/ {
    nrf_radio_fem: skyworks_shield {
        compatible = "skyworks,sky66112-11";
        ctx-pin = < 13 >;
        crx-pin = < 14 >;
    };
};

Name `my_fem` is just an example, there is no requirement for certain node name.

Provided pin numbers and node name `my_fem` are just exemplary. Software FEM
support controls `CTX` and `CRX` pins of the SKY66112-11. State of other
control pins should be set by other means according to SKY66112-11 Product Specification
what is out of scope of this user guide.

Required properties:
`ctx-en-pin` Pin number of a SOC controlling `CTX` signal of the SKY66112-11
`crx-en-pin` Pin number a SOC controlling `CRX` signal of the SKY66112-11

Optional properties:

`ctx-settle-time-us`, `crx-settle-time-us` control timing of interface signals.
Provided default values are appropriate for most cases. If overriding them is necessary
please put custom value just below pin mapping properties. For constraints
of these values please refer to SKY66112-11 Product Specification.

`tx-gain-db`, `rx-gain-db` inform protocol drivers about gain provided by the
SKY66112-11. Default values are accurate for SKY66112-11, but can be overridden
when using simillar device with different gain.

Kconfig parameters:

`MPSL_FEM_SKY66112_11_GPIOTE_CTX`, `MPSL_FEM_SKY66112_11_GPIOTE_CRX`,
serve to assign unique GPIOTE channel numbers to be used exclusively by the FEM driver.

`MPSL_FEM_NRF21540_GPIO_PPI_CHANNEL_0`, `MPSL_FEM_NRF21540_GPIO_PPI_CHANNEL_1`,
serve to assign unique PPI channel numbers to be used exclusively by the FEM driver.
