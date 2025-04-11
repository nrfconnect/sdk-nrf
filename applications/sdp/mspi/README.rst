################
HPF MSPI Example
################

Introduction
************

The MSPI (Multi-bit SPI Bus) HPF Application is divided into three parts: the MSPI driver, the HPF application, and the HRT (Hard Real Time) module.
The MSPI HPF driver is a firmware module runs on APP core that based on ZephyrRTOS MSPI API provides data and configuration transmission between the APP core and the FLPR core.
The HPF application is a firmware module runs on FLPR core that provides data transmission between the FLPR core and the MSPI device connected to the bus.
The HRT (Hard Real Time part of the HPF application) handles the emulation of the MSPI hardware peripheral on the FLPR core.
It enables real-time data transmission between the FLPR core and the peripheral hardware connected to MSPI bus.
It also provides precise time management and synchronization of data transfer operations in real-time.
The module handles pin configuration, clocking, and low-level data transfer.

Scope
=====

- Configure and manage MSPI data transfers.
- Provide real-time responses to requests from the application core.
- Control of clock, pins, and data flow in real-time.

Requirements
============

- Compatibility with MSPI configuration (clock polarity and phase, number of data lines/transmission mode - single, quad).
- Low latency in operations.
- High troughput.

Technical Context
=================

The system runs in ZephyrRTOS build environment (FLPR core runs without ZephyrRTOS kernel),
with the HRT module directly interacting with real-time peripherals (VPR RT) through hardware registers (VPR CSR)
to manage clocking, GPIO pins, and data transmission.

Data Flow and Logic
*******************

No Copy
=======

Data between MSPI driver and HPF application is passed through IPC.
Data can be passed either by copy (NRF54H20 platform) or by reference (NRF54L15 platform).
By default data is passed by reference, to change that Kconfig ``SDP_MSPI_IPC_NO_COPY`` flag has to be disabled.

Configuration Flow
==================

There are two configuration "phases" (driver configuration and device configuration).

1. **Driver configuration**:

   - The application calls :c:func:`mspi_config` to initialize the MSPI driver.
   - MSPI driver passes DTS pin configuration to HPF application.
   - HPF application calls :c:func:`config_pins` function to initialize GPIO settings based on the received configuration.
   - All pins are assigned to specific VIO lines using a mapping array.

2. **Device configuration**:

   - The application calls :c:func:`mspi_dev_config` to configure the MSPI device.
   - MSPI Driver passes selected parts of the configuration to the HPF application.
   - HPF appllication adds device configuration to device list.

Data Transmission Flow
======================

Transmission flow is divided into two parts: transfer preparation and the transfer itself.

1. **Transfer Preparation**:

   - The application calls :c:func:`mspi_transceive` to initiate a data transfer.
   - MSPI Driver passes transfer parameters to HPF application (``hrt_xfer_t``).
   - HPF application calls :c:func:`configure_clock` to set the clock polarity and phase (SPI modes) based on the ``cpp_mode`` parameter. Ensures compatibility with modes 0 and 3.
   - For each packet, MSPI driver passes packet data to HPF application and waits for its completion.
   - For each packet, HPF application calls :c:func:`xfer_execute` to prepare the transfer and adjust the last word.
   - Due to hardware issue last word length cannot be equal to 1. |br|
     :c:func:`adjust_tail` ensures that it is not the case and divides bits of last two words between each other to achieve that.

2. **Packet Element Transfer**:

   - Each packet transfer is initiated by either :c:func:`hrt_write` or :c:func:`hrt_read` on the FLPR side.
   - Each packet is divided into four elements: ``command``, ``address``, ``dummy_cycles``, and ``data``.
   - :c:func:`hrt_write` and :c:func:`hrt_read` prepare the hardware for data transmission by: |br|
     - Setting GPIO direction masks. |br|
     - Determining active packet elements. |br|
   - CS is enabled before the transfer and disabled afterward unless ``ce_hold`` is true.
   - Each TX packet element is processed using :c:func:`hrt_tx`.
   - :c:func:`hrt_read` function treats ``data`` element as RX and passes all other elements to :c:func:`hrt_write`
   - For each paacket element the shift controller is configured for the appropriate number of data lines and word size.

Sequence flow
=============

Flow between user application and FLPR HRT is presented below:

  .. uml::

    @startuml
    box "Application"
    participant "User\nApplication" as t
    participant "Zephyr RTOS\nMSPI API" as a
    participant "HPF eMSPI driver" as d
    end box
    box "FLPR"
    participant "HPF FLPR APP" as f
    participant "HRT" as h
    end box

    == Initialization ==

    activate t
    t -> a : mspi_config()
    activate a
    a -> d : api_config()
    activate d
    d -> d : send_config(NRFE_MSPI_CONFIG_PINS)
    note right d: Shared mem and/or IRQ num
    d -> f : NRFE_MSPI_CONFIG_PINS
    activate f
    f -> f : config_pins()
    return
    deactivate f
    d --> a
    deactivate d
    a --> t
    deactivate a

    ...

    loop for each device
    t -> a : mspi_dev_config()
    activate a
    a -> d : api_dev_config()
    activate d
    d -> d : send_config(NRFE_MSPI_CONFIG_DEV)
    note right d: Shared mem and/or IRQ num
    d -> f : NRFE_MSPI_CONFIG_DEV
    activate f
    note right f: Fill structures\nwith data from APP\nand configure CE pins
    return
    deactivate f
    d --> a
    deactivate d
    a --> t
    deactivate a
    end

    ...

    == TX-RX-TX requests ==

    t -> a : mspi_transceive()
    activate a
    a -> d : api_transceive()
    activate d
    d -> d : send_config(NRFE_MSPI_CONFIG_XFER)
    note right d: Shared mem and/or IRQ num
    d -> f : NRFE_MSPI_CONFIG_XFER
    activate f
    f -> f : configure_clock()
    return
    deactivate f
    loop for each packet
    d -> d ++ : start_next_packet()
    d -> d ++ : xfer_packet()
    note right d: Shared mem and/or IRQ num
    d -> f : NRFE_MSPI_TX or NRFE_MSPI_TXRX
    activate f
    f -> f ++ : xfer_execute()
    f -> f ++ : adjust_tail(command)
    deactivate f
    f -> f ++ : adjust_tail(address)
    deactivate f
    f -> f ++ : adjust_tail(dummy_cycles)
    deactivate f
    f -> f ++ : adjust_tail(data)
    deactivate f
    deactivate f
    note right f: IRQ num
    f -> h : Dispatch
    activate h
    
    alt transfer type TX
    h -> h ++ : hrt_write()
    h -> h ++ : hrt_tx(command)
    deactivate h
    h -> h ++ : hrt_tx(address)
    deactivate h
    h -> h ++ : hrt_tx(dummy_cycles)
    deactivate h
    h -> h ++ : hrt_tx(data)
    deactivate h
    deactivate h
    
    else trasnfer type TXRX
    note right h: Temp. set data length to 0
    h -> h ++ : hrt_write()
    h -> h ++ : hrt_tx(command)
    deactivate h
    h -> h ++ : hrt_tx(address)
    deactivate h
    h -> h ++ : hrt_tx(dummy_cycles)
    deactivate h
    deactivate h
    note right h: Temp. set data length to original value
    note right h: Receive data
    end
    return
    return
    deactivate d
    deactivate d
    d --> a
    deactivate d
    a --> t
    deactivate a
    deactivate t
    @enduml


Main Components
***************

Key Functions
=============

:c:func:`hrt_write`

- **Purpose**:

  - The primary function for executing a TX transfer. It handles configuration, timing, and data transmission for all frame elements.

- **Key steps**:

  1. Configures GPIO directions for TX using the transfer parameters.
  2. Determines the active frame element (command, address, dummy bits, or data).
  3. Transfers the frame elements sequentially via :c:func:`hrt_tx`.
  4. Manages clock timing and CE pin behavior.

:c:func:`hrt_tx`

- **Purpose**:

  - Handles the low-level data transfer for a specific frame element (e.g., ``command``, ``address``, ``dummy_cycles``, or ``data``).

- **Key steps**:

  1. Configures the shift controller (:c:func:`nrf_vpr_csr_vio_shift_ctrl_buffered_set`).
  2. Writes each word from the data buffer to the output register.
  3. Starts the hardware counter for generating clock signal.

:c:func:`hrt_read`

- **Purpose**:

  - The primary function for executing a RX transfer. It handles configuration, timing, and data transmission for all frame elements.

- **Key steps**:

  1. Call :c:func:`hrt_write` for ``command``, ``address`` and ``dummy_cycles`` elements.
  2. Configures GPIO directions for RX using the transfer parameters.
  3. Receives ``data`` element.
  4. Manages clock timing and CE pin behavior.


:c:func:`adjust_tail`

- **Purpose**:

  - Manages the formatting of the last and penultimate words in the transfer buffer to comply with hardware limitations.

- **Key logic**:

  - Adjusts word boundaries when the last word requires only 1 clock cycle to be sent.
  - Handles cases where data needs to be split across multiple words.

Key Data Structures
===================

- **Transfer elements:**

  - ``hrt_frame_element_t``: Defines different frame elements (:c:enumerator:`HRT_FE_COMMAND`, :c:enumerator:`HRT_FE_ADDRESS`, :c:enumerator:`HRT_FE_DUMMY_CYCLES`, :c:enumerator:`HRT_FE_DATA`).

- **Transfer configuration:** ``hrt_xfer_t``: Encapsulates the core configuration for an MSPI transfer.

  - ``xfer_data``: An array of ``hrt_xfer_data_t`` structures for the frame elements (:c:member:`command`, :c:member:`address`, :c:member:`dummy_cycles`, :c:member:`data`).
  - ``bus_widths``: Defines the bit-width configuration for the :c:member:`command`, :c:member:`address`, :c:member:`dummy_cycles`, and :c:member:`data` sections of the transfer.
  - ``counter_value``: Sets the clock divider to achieve frequency specified by a user.
  - ``ce_vio``: Index of the VIO pin used as CE.
  - ``ce_hold``: Whether CE remains asserted after the transfer.
  - ``ce_polarity``: Active polarity of the CE pin.
  - ``tx_direction_mask`` and ``rx_direction_mask``: Configures GPIO directions for data transmission.
  - ``cpp_mode``: SPI mode 0 or 3

- **Transfer data:** ``hrt_xfer_data_t``: Represents transfer-specific properties for each frame element.

  - ``data``: Points to the data being transmitted or received.
  - ``word_count``: Number of 32-bit words in the data buffer.
  - ``last_word_clocks``: Number of clock pulses for the final word.
  - ``penultimate_word_clocks``: Clock pulses for the second-to-last word.
  - ``last_word``: Holds the last word data (for transition or from reception).
  - ``fun_out``: Selects which function in TX to use to access buffered output register.

Technologies and Tools
**********************

- **nrfx HAL (Hardware Access Layer)**: Access to VPR CSR registers for direct hardware control.

- :ref:`assembly_management.rst``: For optimizing performance-critical sections.
