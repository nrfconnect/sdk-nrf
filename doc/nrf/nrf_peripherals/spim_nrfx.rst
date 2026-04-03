.. _spim_nrfx:

SPIM nrfx driver
################

Complete driver API reference can be found under following link: `nrfx SPIM driver`_

Purpose of this page is to present peripheral usage scenarios covered by the driver API.

Configuration
*************

Before the hardware peripheral instance can be configured, associated software driver instance must be created.
This can be achieved by initializing a variable of type `nrfx_spim_t` with static storage duration, using `NRFX_SPIM_INSTANCE()` macro.
Subsequently, `nrfx_spim_config_t` structure must be defined and its members initialized in accordance to desired driver and peripheral operating mode.
Lastly, define an event handler function of `nrfx_spim_event_handler_t` type if interrupts are to be used and driver user notified of hardware events occurrence.
In order to perform run-time initialization of the hadware peripheral instance, call `nrfx_spim_init()` function.
Check returned error code to make sure initialization passed successfully.
At this point the software driver instance is operable.

  .. code-block:: c

    #define NRF_SPIM_HW_INST NRF_SPIM20

    #define NRFX_SPIM20_PIN_SCK  NRF_SPIM_PIN_NOT_CONNECTED
    #define NRFX_SPIM20_PIN_MOSI NRF_SPIM_PIN_NOT_CONNECTED
    #define NRFX_SPIM20_PIN_MISO NRF_SPIM_PIN_NOT_CONNECTED
    #define NRFX_SPIM20_PIN_CS   NRF_SPIM_PIN_NOT_CONNECTED

    static nrfx_spim_t spim = NRFX_SPIM_INSTANCE(NRF_SPIM_HW_INST);
    static const spim_cfg = NRFX_SPIM_DEFAULT_CONFIG(NRFX_SPIM20_PIN_SCK,
                                                     NRFX_SPIM20_PIN_MOSI,
                                                     NRFX_SPIM20_PIN_MISO,
                                                     NRFX_SPIM20_PIN_CS)

    void spim_event_handler(nrfx_spim_event_t const * p_event, void * p_context) {
        ...
    }

    int main(void) {
        int err;
        void * p_context = NULL;

        err = nrfx_spim_init(&spim, &spim_cfg, spim_event_handler, NULL);
        if (err < 0) {
            ...
        }
    }

Interrupts
==========

When a hardware interrupt is trigger, the driver interrupt handler must be notified so that the driver can update its software state.
To achieve that use `IRQ_CONNECT` macro using generic `nrfx_spim_irq_handler` function and `&spim` argument before the driver is initialized:

  .. code-block:: c

    int main(void) {
        ...

        IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_SPIM_HW_INST), IRQ_PRIO_LOWEST, nrfx_spim_irq_handler, &spim, 0);
    }

Alternatively, `IRQ_DIRECT_CONNECT` macro can be used. It reduces interrupt latency at the cost of Zephyr's kernel API usage prohibition in interrupt context, which can be limiting when it comes to thread signalling.
Because `IRQ_DIRECT_CONNECT` does not accept additional parameter, veneer function must be used in order to provide correct argument.

  .. code-block:: c

    void spim_irq_handler(void) {
        nrfx_spim_irq_handler(&spim);
    }

    int main(void) {
        ...

        IRQ_DIRECT_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_SPIM_HW_INST), IRQ_PRIO_LOWEST, spim_irq_handler, 0);
    }

Pin management
==============

Standardized approach for managing pins in `Zephyr`_ is the `pinctrl` subsystem.
This applies to both `NRF_Px` registers, as well as `NRF_SPIMx->PSEL` registers.


Explicit - pinctrl
------------------

If `pinctrl` subsystem is to be used:

* define `pinctrl` pin groups in the application overlay, per `Zephyr's pinctrl pin configuration`_,
* set `skip_psel_cfg` and `skip_gpio_cfg` members of `nrfx_spim_config_t` structure to `true`,
* use `pinctrl_apply_state()` runtime function. Refer to `Zephyr's pinctrl API`_ for more information.

Implicit
--------

If `nrfx SPIM driver`_ is to manage pins implicitly, during `nrfx_spim_init()` function call:

* set `nrfx_spim_config_t` structure members: `sck_pin`, `mosi_pin`, `miso_pin`, `ss_pin` and optionally `dcx_pin`, if used, to valid pin numbers,
* set `skip_psel_cfg` and `skip_gpio_cfg` members of `nrfx_spim_config_t` structure to `false`.

Chip Select management
======================

Lorem ipsum

Software-controlled
-------------------

Lorem ipsum

Hardware-controlled
-------------------

Lorem ipsum

Data transfer
*************

After performing the initialization, the driver is ready to perform transfers with `nrfx_spim_xfer()` function.
Fundamentally, each individual transfer is described by:

* instance of `nrfx_spim_xfer_desc_t` structure, containing buffer pointers and data lengths for transmission and reception,
* integer value, composed of OR'ed bitmasks denoting individual transfer flags, altering the transfer properties.

Caution!

Depending on the specific SoC, buffers provided to the driver via `p_tx_buffer` and `p_rx_buffer` members of `nrfx_spim_xfer_desc_t` structure might need to comply with specific alignment requirement or RAM location requirement.
Refer to respective Product Specification for more details.

Basic TX-RX transfer
====================

To perform simple transfer composed of data transmission and reception provide `nrfx_spim_xfer_desc_t` structure filled with desired buffers as an argument to the `nrfx_spim_xfer()` function.
`flags` argument shall remain `0`.
Once done, the transfer status will be reported via `nrfx_spim_event_t` structure as a parameter to the event handler function provided during driver initialization.

Data/command transfer
=====================

Some external, peripheral devices (like displays) may need to utilize an additional GPIO signal to denote whether the transferred bytes should be treated as payload data or control commands.
To facilitate this use-case `nrfx SPIM driver`_ exposes dedicated `nrfx_spim_xfer_dcx()` function.
Apart from parameters: `nrfx_spim_xfer_desc_t` structure and `flags` bitmask, this function also accepts `cmd_length` parameter which is used to specify number of bytes to transfer before setting the DCX pin high.

Caution!

This feature is not available on all SoCs and may not be available for all SPIM hardware instances.
Refer to respective Product Specification for more details.
