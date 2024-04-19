:orphan:

.. features_nrf54h20:

Features of the nRF54H20 DK
###########################


.. contents::
   :local:
   :depth: 2

The nRF54H20 System-on-Chip (SoC) is the first wireless SoC in the nRF54H Series from Nordic Semiconductor.
It is an ideal solution for 2.4 GHz wireless applications that have the highest requirements for performance, energy efficiency, security, and ease of use, enabling you to incorporate innovative features hard to productize today.
nRF54H20 includes a multiprotocol radio, supporting Bluetooth Low Energy and IEEE 802.15.4-based protocols.

High performance and low power
==============================

The nRF54H20 SoC has been designed to provide high compute performance in your application, while maintaining or improving the battery life compared to previous devices.
It achieves this by integrating two Arm® Cortex®-M33 processors clocked up to 320 MHz, 2 MB of MRAM as non-volatile memory, 1 MB of RAM, a 10 dBm TX power multiprotocol radio, and state-of-the-art security features, in an asymmetric multi-core system, in which the different CPUs are optimized for various types of workloads.
This configuration allows the SoC to achieve a lower power consumption and better performance compared to the usage of identical CPUs.
nRF54H20 uses MRAM as non-volatile memory.
Compared to flash memory, MRAM is more energy efficient and can be written to up to 86x faster.
Compared to earlier devices, external interfaces have also been improved to support significantly higher data rates (up to 40 times greater for USB and up to 8 times greater for external memory).
This does not come at the expense of power consumption, as both active and sleep power consumption have been improved as well.

Highly integrated solution
==========================

The nRF54H20 SoC can help you simplify hardware designs by replacing multiple components, like an application MCU, an external flash, and a wireless SoC with one integrated SoC.
The nRF54H20 SoC's excellent energy efficiency enables smaller batteries to be utilized, further reducing both the design size and cost.
It also provides a single easy-to-use development environment without having to use multiple tools from different vendors.

Best-in-class wireless support
==============================

The nRF54H20 SoC integrates a best-in-class 2.4 GHz radio that supports a wide range of wireless protocols.
It supports Bluetooth Low Energy, LE Audio, Direction Finding, Bluetooth mesh, Thread, Zigbee and Matter.
This includes all Bluetooth 5.3 features and the preparation to support upcoming Bluetooth specification versions.
Additionally, nRF54H20 has improved sensitivity and robustness against interference.

22 nm FD-SOI process node
=========================

Leveraging the latest advancements in semiconductor manufacturing, nRF54H20 is built using a 22 nm process from GlobalFoundries.
Smaller process nodes typically result in higher-leakage currents.
However, this is mitigated by leveraging Fully Depleted Silicon-on-Insulator (FD-SOI) process technology.
FD-SOI reduces the size of the transistor channel compared to bulk CMOS, resulting in lower leakage currents.

Additional security features
============================

The nRF54H20 SoC has been designed with security as a key feature.
The nRF54H20 SoC provides security functionality as services, implemented centrally in the Secure Domain.
It includes state-of-the-art security features and is designed to achieve PSA Certified Level 3.
Physically separated centralized security has several advantages.
It separates the most important security operations from the rest of the system.
Because of this separation, there is a clear isolation layer where you interact with the secure services via well-defined APIs.
It also simplifies the inclusion of tamper sensors and side-channel protection without significantly increasing the cost of the device.

Multi-core architecture
=======================

The nRF54H20 SoC is partitioned into functional blocks, called domains.
The domains containing the user-programmable main CPUs and associated functions are called cores.
Most memory and peripherals can be flexibly allocated to cores at compile time.
To make this possible, the memory and peripherals are located in a separate area called the Global Domain.
Security functions are centralized into the Secure Domain.

Asymmetric multiprocessing
==========================

One of the key methods used to optimize the nRF54H20 SoC for very efficient processing is to include several CPUs and optimize them for different workloads.
In the nRF54H20 SoC, there are two different types of CPUs: Arm Cortex-M33, and a RISC-V-compatible CPU designed by Nordic Semiconductor called VPR (pronounced “Viper”).
However, we also apply optimizations to each instance of each processor, so that CPUs are tailored to their range of tasks.
This way, you can freely select the CPU that is best suited to the various workloads that make up an application.
Each CPU runs its own instance of the operating system.
There is no centralized scheduler that assigns threads to various cores.
This means that you have a full control over which CPU is used for a given task.

System services
===============

One key feature of the nRF54H20 SoC is that a lot of functionality is provided as services, implemented by Nordic-provided firmware.
This means that you can access functionality through a software API rather than through a set of registers.
Using a firmware layer means that functionality can be more advanced and handled at a higher level of abstraction, which increases ease-of-use.

Power management service
========================

Rather than manually setting a power mode through a register or calling a sleep instruction, power management is performed through a service where the application describes the functionality it needs and how much latency it can accept.
Based on this information, the Power Management Service automatically handles the details such as: deciding how registers will be retained, which parts of the device are put into what mode, and what clock signals are running.

Security architecture
=====================

The nRF54H20 SoC security architecture is based on the conclusions from extensive threat modelling.
nRF54H20 is designed to protect against logical attacks and firmware-oriented attacks, as well as physical security at the PCB or package level (for example, against voltage glitching and EMI attacks).
However, robustness against physical die probing or alteration of the die itself is not provided by the design.
Security in software is implemented as a set of services, provided by the Secure Domain.
These services manage the following:

* Secure Boot
* Cryptographic operations
* Key management
* Device Firmware Updates (DFU) through the Software Internet of Things (SUIT) procedure

The main application and other parts of the system access security services through an API.
This API is translated internally to messages sent to the Secure Domain through a mailbox-based interface.
The security services handle these requests, utilizing hardware accelerators and other resources, and as security is centralized in the Secure Domain, keys can be kept inside this physical area and remain well-protected against attacks or breaches due to firmware exploits.
The nRF54H20 SoC also includes tamper sensors that can detect an attack in progress and take appropriate action.
The cryptographic accelerators have been hardened against side-channel attacks.

CPUs overview and features
==========================

The CPU cores in the nRF54H20 SoC are based on two types of CPU architectures:

* Arm Cortex M33
* RISC-V (VPR cores)

Arm Cortex M33
--------------

The Arm Cortex-M33 is a powerful yet power-efficient CPU developed by Arm.
It supports TrustZone, allowing secure and non-secure code to be separated and to have differentiated access to memory and peripherals.
In the nRF54H20 SoC, both Cortex-M33 instances are configured with a floating point unit, TrustZone, and DSP extensions enabled.
In addition to the CPU itself, each Arm Cortex-M33-based core includes also the following local features:

* Local level-1 (L1) cache, split into separate caches for data and instructions, with 16 kB each[2].
* Local RAM, serving as Tightly Coupled Memory (TCM), accessible without any wait states or latency.
* A Memory Vector DMA (MVDMA) controller, which allows high-throughput DMA transfers between local and global memory.
* Two watch-dog timers (WDT), controlled by the secure and non-secure firmware, respectively.
  WDTs produce a local reset, affecting only the core and not the rest of the system.
* An Interprocessor Communication Transceiver (IPCT) for communication with peripherals in other domains.

Cache memories
--------------

In the nRF54H20 SoC, each Arm Cortex-M33 core contains a local level 1 (L1) cache with separate data and instruction caches.
Their size is 16 kB each.
These caches are 2-way set associative.
There is also a global level 2 (L2) cache.
The main purpose of this cache is to accelerate accesses to slower global memories, such as MRAM and EXMIF.
The size of the L2 cache is 128 kB.

VPR core
--------

The VPR (pronounced “Viper”) is a small, efficient CPU developed by Nordic Semiconductor.
It is compatible with the RISC-V instruction set.
This means that industry-standard RISC-V development tools can be used.
It implements the following extensions:

* E: Integer instruction set with 16 registers
* M: Multiply and divide extension
* C: Compressed extension (compressed instructions)

The VPR implements the Machine mode CPU mode as well as the RISC-V CLIC specification for the interrupt controller.
The VPR implementation has been optimized for low-power, real-time use cases.

There are two user-accessible instances of VPRs in the nRF54H20 SoC: the Peripheral Processor (PPR, pronounced “Pepper”) and the Fast Lightweight Processor (FLPR, pronounced “Flipper”).
The PPR is located in the low-leakage portion of the Global Domain and is primarily intended to do the following:

* Handle peripherals in low-power states while the main processors are in Sleep mode.
* Coordinate peripherals.
* Implement low-level protocols for communicating with sensors and actuators.

The FLPR is located in the high-speed portion of the Global Domain and is primarily intended to do the following:

* Implement software-defined peripheral functions.
* Offload timing-critical tasks from the main processors.

Dynamic Voltage and Frequency Scaling support
---------------------------------------------

Dynamic Voltage and Frequency Scaling (DVFS) is a power management technique that allows adjusting the operating voltage and operating frequency of a CPU, depending on the workload.
This functionality was common in high-end processors but is now viable also for MCUs.
In the nRF54H20 SoC, the Application Core is DVFS-enabled.
If the maximum clock speed is not required, both the clock speed and voltage can be reduced, improving power efficiency.
