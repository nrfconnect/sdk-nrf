.. _hpf_mspi_example:

High-Performance Framework MSPI
###############################

.. contents::
   :local:
   :depth: 2

.. caution::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>` and is limited to the nRF54L15 device.

This application demonstrates how to write a :ref:`High-Performance Framework (HPF) <hpf_index>` application and communicate with it.
The application implements a subset of the Zephyr MSPI API.

Application overview
********************

The MSPI HPF application is structured into the following main components:

* :ref:`The HPF application <hpf_mspi_example_api>` - Operates on the FLPR core and facilitates data transmission between the application core and the connected MSPI device.
* :ref:`The Hard Real Time (HRT) module <hpf_mspi_example_api>` - Runs on the FLPR core and facilitates data transmission between the FLPR core and the connected MSPI device.
  The module emulates the MSPI hardware peripheral on the FLPR core, managing real-time data transmission and ensuring precise timing and synchronization of data transfer operations.
* :ref:`The MSPI Zephyr driver <hpf_mspi_example_api>` - Operates on the application core and uses the Zephyr's scalable real-time operating system (RTOS) MSPI API for data and configuration transmission between the application and FLPR cores.

Scope
*****

The application must meet the following key requirements:

* Compatibility with the MSPI configuration including clock polarity, phase, and the number of data lines or transmission modes (single, quad).
* Low latency and high throughput.

Requirements
************

The application supports the following development kits:

.. table-from-sample-yaml::

Configuration and data transfer management
******************************************

Data transfer between the MSPI driver and the HPF application is done through the ICMsg.
Data can be configured to be passed either by copy or by reference.
By default, data is passed by reference.
To enable data passing by copy, you must disable the ``SDP_MSPI_IPC_NO_COPY`` Kconfig option.

Initialization phase
====================

The initialization of the MSPI HPF application is divided into two main phases: driver initialization and device initialization.

1. Driver initialization:

   a. A user application calls the :c:func:`mspi_config` function to initialize the MSPI driver.
   #. The MSPI driver passes the DTS pin configuration to the HPF application.
   #. The HPF application calls the :c:func:`config_pins` function to initialize GPIO settings based on the received configuration.
   #. All pins are assigned to specific VIO lines using a mapping array.

2. Device initialization

   a. A user application calls the :c:func:`mspi_dev_config` function to configure the MSPI device.
   #. The MSPI driver passes selected parts of the configuration to the HPF application.
   #. The HPF application adds the device configuration to the device list.

Data transmission phase
=======================

The data transmission process is structured into two parts: preparation and the transfer.

1. Transfer preparation

   a. A user application calls the :c:func:`mspi_transceive` function to initiate a data transfer.
   #. The MSPI driver passes transfer parameters to HPF application (``hrt_xfer_t``).
   #. The HPF application calls the :c:func:`configure_clock` function to set the clock polarity and phase (SPI modes) based on the ``cpp_mode`` parameter.
      It ensures compatibility with modes 0 and 3.
   #. For each packet, MSPI driver passes packet data to HPF application, awaiting completion.
   #. For each packet, HPF application calls the :c:func:`xfer_execute` function to prepare the transfer and adjust the last word.

   .. note::

      Due to hardware constraints, the length of the last word cannot be ``1``.
      The :c:func:`adjust_tail` function modifies the last two words to meet this requirement.

2. Packet element transfer

   a. Each packet transfer is initiated by the :c:func:`hrt_write` or :c:func:`hrt_read` functions on the FLPR core.
      Additionally, each packet is divided into four elements: ``command``, ``address``, ``dummy_cycles``, and ``data``.
   #. The :c:func:`hrt_write` and :c:func:`hrt_read` functions prepare the hardware for data transmission by setting GPIO direction masks and determining active packet elements.
   #. The chip select (CS) is enabled before the transfer and disabled afterward unless ``ce_hold`` is set to ``true``.
   #. Each TX packet element is processed using the :c:func:`hrt_tx` function.
   #. :c:func:`hrt_read` function treats the ``data`` element as RX and passes all other elements to the :c:func:`hrt_write` function.
   #. For each packet element, the shift controller is configured for the appropriate number of data lines and word size.

Sequence flow
=============

The following diagram illustrates the interactions between the user application and the FLPR HRT, mapping out each step from initialization to data transfer:

.. uml::

  @startuml
  skinparam sequence {
  DividerBackgroundColor #8DBEFF
  DividerBorderColor #8DBEFF
  LifeLineBackgroundColor #13B6FF
  LifeLineBorderColor #13B6FF
  ParticipantBackgroundColor #13B6FF
  ParticipantBorderColor #13B6FF
  BoxBackgroundColor #C1E8FF
  BoxBorderColor #C1E8FF
  GroupBackgroundColor #8DBEFF
  GropuBorderColor #8DBEFF
  }

  skinparam note {
  BackgroundColor #ABCFFF
  BorderColor #2149C2
  }

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
  d -> d : send_config(HPF_MSPI_CONFIG_PINS)
  note right d: Shared mem and/or IRQ num
  d -> f : HPF_MSPI_CONFIG_PINS
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
  d -> d : send_config(HPF_MSPI_CONFIG_DEV)
  note right d: Shared mem and/or IRQ num
  d -> f : HPF_MSPI_CONFIG_DEV
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
  d -> d : send_config(HPF_MSPI_CONFIG_XFER)
  note right d: Shared mem and/or IRQ num
  d -> f : HPF_MSPI_CONFIG_XFER
  activate f
  f -> f : configure_clock()
  return
  deactivate f
  loop for each packet
  d -> d ++ : start_next_packet()
  d -> d ++ : xfer_packet()
  note right d: Shared mem and/or IRQ num
  d -> f : HPF_MSPI_TX or HPF_MSPI_TXRX
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

Key functions
=============

Key functions, such as :c:func:`hrt_write`, :c:func:`hrt_tx`, and :c:func:`hrt_read` are integral to managing the data transfer process, handling tasks from setting GPIO directions to executing low-level data transfers.

.. list-table::
   :widths: 20 30 50
   :header-rows: 1

   * - Function
     - Purpose
     - Key Steps
   * - :c:func:`hrt_write`
     - The primary function for executing a TX transfer. It handles configuration, timing, and data transmission for all frame elements.
     - 1. Configures GPIO directions for TX using the transfer parameters.
       2. Determines the active frame element (command, address, dummy bits, or data).
       3. Transfers the frame elements sequentially via :c:func:`hrt_tx`.
       4. Manages clock timing and CE pin behavior.
   * - :c:func:`hrt_tx`
     - Handles the low-level data transfer for a specific frame element (for example, ``command``, ``address``, ``dummy_cycles``, or ``data``).
     - 1. Configures the shift controller (:c:func:`nrf_vpr_csr_vio_shift_ctrl_buffered_set`).
       2. Writes each word from the data buffer to the output register.
       3. Starts the hardware counter for generating clock signal.
   * - :c:func:`hrt_read`
     - The primary function for executing a RX transfer. It handles configuration, timing, and data transmission for all frame elements.
     - 1. Call :c:func:`hrt_write` for ``command``, ``address`` and ``dummy_cycles`` elements.
       2. Configures GPIO directions for RX using the transfer parameters.
       3. Receives ``data`` element.
       4. Manages clock timing and CE pin behavior.
   * - :c:func:`adjust_tail`
     - Manages the formatting of the last and penultimate words in the transfer buffer to comply with hardware limitations.
     - - Adjusts word boundaries when the last word requires only 1 clock cycle to be sent.
       - Handles cases where data needs to be split across multiple words.

Key data structures
===================

The following section provides an overview of the primary data structures used in managing MSPI transfers.
These structures are crucial for configuring and executing data transfers efficiently.

* Transfer elements

  * ``hrt_frame_element_t`` - Defines different frame elements (:c:enumerator:`HRT_FE_COMMAND`, :c:enumerator:`HRT_FE_ADDRESS`, :c:enumerator:`HRT_FE_DUMMY_CYCLES`, :c:enumerator:`HRT_FE_DATA`).

* Transfer configuration

  * ``hrt_xfer_t`` - Encapsulates the core configuration for an MSPI transfer.
  * ``xfer_data`` - An array of the ``hrt_xfer_data_t`` structures for the frame elements (:c:member:`command`, :c:member:`address`, :c:member:`dummy_cycles`, :c:member:`data`).
  * ``bus_widths`` - Defines the bit-width configuration for the :c:member:`command`, :c:member:`address`, :c:member:`dummy_cycles`, and :c:member:`data` sections of the transfer.
  * ``counter_value``: Sets the clock divider to achieve frequency specified by a user.
  * ``ce_vio`` - Index of the VIO pin used as CE.
  * ``ce_hold`` - Whether CE remains asserted after the transfer.
  * ``ce_polarity`` - Active polarity of the CE pin.
  * ``tx_direction_mask`` and ``rx_direction_mask`` - Configure GPIO directions for data transmission.
  * ``cpp_mode`` - SPI mode 0 or 3

* Transfer data

  * ``hrt_xfer_data_t`` - Represents transfer-specific properties for each frame element.
  * ``data`` - Points to the data being transmitted or received.
  * ``word_count`` - Number of 32-bit words in the data buffer.
  * ``last_word_clocks`` - Number of clock pulses for the final word.
  * ``penultimate_word_clocks`` - Clock pulses for the second-to-last word.
  * ``last_word`` - Holds the last word data (for transition or from reception).
  * ``fun_out`` - Selects which function in TX to use to access buffered output register.

Building and running
********************

.. |application path| replace:: :file:`applications/hpf/mspi`

.. include:: /includes/application_build_and_run.txt

To build and run the application, you must include code for both the application core and FLPR core.
The process involves building a test or user application that is using MSPI driver with the appropriate sysbuild configuration.

.. tabs::

   .. tab:: Building with tests

      You can use the tests listed in the :ref:`hpf_mspi_example_testing` section.
      To build them, execute the following command in their respective directories:

      .. code-block:: console

         west build -p -b nrf54l15dk_nrf54l15_cpuapp

   .. tab:: Building with User Application

      Follow these steps to build with a user application:

      1. Enable the following Kconfig options:

         * ``SB_CONFIG_HPF`` - Enables the High-Performance Framework (HPF).
         * ``SB_CONFIG_HPF_MSPI`` - Integrates the HPF application image with the user application image.

      #. Disable the following Kconfig options:

         * ``SB_CONFIG_VPR_LAUNCHER`` - Disables the default VPR launcher image for the application core.
         * ``SB_CONFIG_PARTITION_MANAGER`` - Disables the :ref:`Partition Manager <partition_manager>`.

      #. Implement the business logic using the MSPI API with the HPF application.

      #. Add device tree nodes that describe the chips connected via MSPI (hpf_mspi).

      #. Compile your code.

.. _hpf_mspi_example_testing:

Testing
*******

The following tests utilize the MSPI driver along with this application:

* ``nrf/tests/zephyr/drivers/mspi/api``
* ``nrf/tests/zephyr/drivers/flash/common``

These tests report results through serial port (the USB debug port on the nFR54L15 DK).

Dependencies
************

* :file:`zephyr/doc/services/ipc/ipc_service` - Used for transferring data between application core and the FLPR core.
* `nrf HAL <nrfx API documentation_>`_ - Enables access to the VPR CSR registers for direct hardware control.
* :ref:`hpf_assembly_management` - Used to optimize performance-critical sections.

.. _hpf_mspi_example_api:

API documentation
*****************

Application uses the following API elements:

Zephyr driver
=============

* Header file: :file:`include/drivers/mspi/hpf_mspi.h`
* Source file: :file:`drivers/mspi/mspi_hpf.c`

FLPR application
================

* Source file: :file:`applications/hpf/mspi/src/main.c`

FLPR application HRT
====================

* Header file: :file:`applications/hpf/mspi/src/hrt/hrt.h`
* Source file :file:`applications/hpf/mspi/src/hrt/hrt.c`
* Assembly: :file:`applications/hpf/mspi/src/hrt/hrt-nrf54l15.s`
