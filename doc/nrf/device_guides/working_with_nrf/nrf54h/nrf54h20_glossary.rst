:orphan:

.. _nrf54h20_glossary:

Glossary
########

.. contents::
   :local:
   :depth: 2

.. glossary::
   :sorted:

   Advanced eXtensible Interface (AXI) Communication
      A system that transfers data between components inside a computer or between computers.

   BELLBOARD
      A peripheral associated with a given core that provides support for the inter-domain SW signaling.
      It includes a set of tasks and events intended for signaling within an interprocessor communication (IPC) framework.

   Bill of materials (BoM)
      List of the raw materials, sub-assemblies, intermediate assemblies, sub-components, parts, and the quantities of each needed to manufacture an end product.

   Core
      Subsets of domains.
      Most memory and peripherals can be flexibly allocated to cores at compile time.

   CRACEN
      A hardware cryptographic engine within nRF54H devices.

   Easy Direct Memory Access (EasyDMA)
      A module that some peripherals implement to gain direct access to data RAM.

   Enhanced ShockBurst (ESB)
      A basic protocol supporting two-way data packet communication including packet buffering, packet acknowledgment, and automatic retransmission of lost packets.
      ESB provides radio communication with low power consumption.
      The implementation is small in code size and easy to use.

   External Memory Interface Encryption Engine (EXMEE)
      A peripheral that supports on-the-fly encryption and authentication for the memory connected to EXMIF.
      The AES tweaked mode is used to provide encryption, and one of single hash, granular hash, or Merkle tree is used to provide authentication, using the SHA3 algorithm.

   External Memory Interface (EXMIF)
      A bus protocol for communication from an integrated circuit, such as a microprocessor, to an external memory device located on a circuit board.

   Domain
      Functional blocks included in the system/SoC.
      Contains the user-programmable main CPU and associated functions.

   Dynamic Voltage and Frequency Scaling (DVFS)
      A power management technique that allows adjusting the operating voltage and operating frequency of a CPU, depending on the workload.

   Fully Depleted Silicon-on-Insulator (FD-SOI)
      A type of SOI technology that uses a thin layer of silicon that is fully depleted of electrons.

   Frequency-locked loop (FLL)
      An electronic control system that generates a signal that is locked to the frequency of an input or "reference" signal.

   Fast Lightweight Processor (FLPR, pronounced “Flipper”)
      A processor that is located in the high-speed portion of the Global Domain primarily intended to implement software-defined peripheral functions.

   Interprocessor Communication Transceiver (IPCT)
      A peripheral used for direct signalling between peripherals in other domains.

   Spin-Transfer Torque Magneto-Resistive Random Access Memory (MRAM (STT-MRAM))
      An alternative non-volatile memory (NVM) to flash memory.
      Compared to flash, MRAM does not have to be erased before writing and can simply be written, making it easier to use.

   Memory Privilege Controller (MPC)
      Performs security configuration, enforcement, and bus decoding.
      It implements security filtering, checking bus accesses against the configured access properties and blocking any unauthorized accesses.

   Memory-to-memory Vector Direct Memory Access (MVDMA)
      A peripheral capable of copying data from one memory address to another memory address.
      It is not a complement to the EasyDMA (:term:`Easy Direct Memory Access (EasyDMA)`).
      The scatter-gather property applies to MVDMA as well.

   Peripheral Processor (PPR, pronounced “Pepper”)
      A processor that is located in the low-leakage portion of the Global Domain and is primarily intended to:

         * Handle peripherals in low-power states while the main processors are in sleep mode.
         * Coordinate peripherals.
         * Implement low-level protocols for communicating with sensors and actuators

   Platform Security Architecture Certified (PSA Certified)
      A security certification scheme for Internet of Things (IoT) hardware, software and devices.

   Power Management Service
      A service that automatically handles the settings described by an application.
      It decides how registers will be retained, which parts of the device are put into what mode, and what clock signals are running.

   Physically Unclonable Function (PUF)
      A function device that exploits inherent randomness introduced during manufacturing to give a physical entity a unique "fingerprint" or a trust anchor.

   Quad Serial Peripheral Interface (QSPI)
      A peripheral that provides support for communicating with an external flash memory device using SPI.

   Secure domain (SecDom)
      A dedicated domain which executes a pre-compiled firmware component that is signed by Nordic Semiconductor.
      It exposes security services to the other cores through an IPC interface.

   Serial Peripheral Interface (SPI)
      An interface bus commonly used to send data between microcontrollers and small peripherals such as shift registers, sensors, and SD cards.

   Serial Peripheral Interface Master (SPIM)
      A peripheral that can communicate with multiple slaves using individual chip select signals for each of the slave devices attached to a bus.

   Serial Peripheral Interface Slave (SPIS)
      A peripheral used for ultra-low power serial communication from an external SPI master.

   System Controller
      A VPR that implements system startup and power management functionalities that in the past would have been implemented in hardware.

   Tightly Coupled Memory (TCM)
      Part of RAM which provides a low-latency memory access that the core can use with predictable access time.
      Unlike cached memories for which the access latency is unpredictable.

   TIMER
      A peripheral that runs on the high-frequency clock source (HFCLK) and includes a four-bit (1/2X) prescaler that can divide the timer input clock from the HFCLK controller.
      It can operate in two modes: timer and counter.

   TrustZone
      Provides a cost-effective methodology to isolate security-critical components in an ARM Cortex CPU by hardware separating a rich operating system from smaller, secure operating system.

   Trusted Third Party (TTP)
      An entity which facilitates interactions between two parties who both trust the third party.

   VPR Core (pronounced “Viper”)
      A core that is compatible with the RISC-V instruction set, meaning the industry-standard RISC-V development tools can be used.
      VPR implements the RV32E instruction set (Base Integer Instruction Set (embedded) - 32 bit, 16 registers) and the following extensions:

         * M: Multiply and divide extension
         * C: Compressed extension (compressed instructions)

      The nRF54H20 PDK uses several VPR cores: :term:`Fast Lightweight Processor (FLPR, pronounced “Flipper”)`, :term:`Peripheral Processor (PPR, pronounced “Pepper”)` and :term:`System Controller`.

   VPR Event Interface (VEVIF)
      A real-time peripheral that allows interaction with the VPR's interrupts and the PPI system in the domain where the VPR is instantiated.
