.. _ug_nrf93m1_interfaces:

Host interfaces
###############

.. contents::
   :local:
   :depth: 2

This page describes how a host processor connects to an nRF93M1 module and how to configure that connection in the |NCS|.

Choose your integration model before you design the interface, because the choice determines both your wiring and your host software footprint.
See :ref:`ug_nrf93m1_architecture` for the comparison.

Host interfaces and peripherals
*******************************

The following table lists the host interfaces and peripherals for the nRF93M1 module:

.. list-table::
   :header-rows: 1

   * - Interface
     - Details
   * - USB
     - USB 2.0 device. High speed 480 Mbps, full speed 12 Mbps. Supports RNDIS and CDC-ECM.
   * - UART
     - Three UARTs, up to 3 Mbps. UART1 carries the AT command interface and includes DTR, RI, RTS,
       and CTS.
   * - IO
     - Four always-on I/O pins, 1.8 V logic. **RFC1** (RF Control interface) through **RFC3**
       are assigned to RF tuner control.
   * - Coexistence
     - Two programmable pins, for arbitration with a co-located radio.
   * - Modem status
     - One pin. ``STATUS``
   * - Network status
     - One pin, ``NETWORK``.
   * - Power control
     - ``POWERKEY`` for power on and off, ``nRESET`` for reset.

.. _ug_nrf93m1_interfaces_signals:

Control and status signals
**************************

Beyond the data interface, a host design needs the following module signals:

.. list-table::
   :header-rows: 1

   * - Signal
     - Direction
     - Purpose
   * - ``POWERKEY``
     - Host to module
     - Powers the module on. Also powers it off, and is the only wake source from System Disabled.
   * - ``nRESET``
     - Host to module
     - Resets the module. Use only for recovery, not for normal power cycling.
   * - ``UART1_DTR``
     - Host to module
     - Wakes the module from power-saving sleep. The only host wake path independent of baud rate.
   * - ``UART1_RI``
     - Module to host
     - The module asserts it when it has data for the host, and de-asserts it when ready to receive.
       Can serve as the host MCU wake source.
   * - ``NETWORK``
     - Module to host
     - Network registration status, readable without an AT command.
   * - Modem status
     - Module to host
     - Modem activity state.

There are following timings for the power and reset pins:

* ``POWERKEY`` low for at least 10 ms turns the module on into System ON.
* ``POWERKEY`` low for at least 650 ms powers it down to System Disabled.
* ``nRESET`` low for at least 100 ms, then released, triggers a full module reset.
  It is active low with an internal 120 kΩ pull-up, so no external pull-up is needed, and it may be left floating in designs that do not use it.

.. important::
   Do not drive module pins while the module is unpowered, and observe the power sequencing in the `nRF93M1 Datasheet`_.
   Module GPIO logic is 1.8 V, driven from the module's internal LDO.
   If your host runs at a different voltage, level shifting is required.
   The nRF54L15 host on the nRF93M1 DK is configured for this already.

.. _ug_nrf93m1_interfaces_at:

AT command interface over UART
******************************

The simplest integration model.
Your host opens UART1 and exchanges AT commands and unsolicited result codes with the module.
The module's IP stack does the networking, so the host needs no networking libraries.

Use this model when:

* The host is small.
* You are porting from another cellular module.
* You want the lowest possible host flash and RAM footprint.

Configure the UART in devicetree on the host side as an ordinary UART peripheral, then use the Zephyr UART API or the modem chat helpers.
See :ref:`ug_nrf93m1_at_commands` for command handling patterns.

Default serial settings are given in the `nRF93M1 AT Commands Reference Guide`_.
Set the baud rate using the ``AT+IPR`` command.

.. note::
   If the module enters a deep sleep state, set the baud rate to 9600 bps or lower first.
   At higher rates the UART cannot wake the module, and recovery requires ``POWERKEY`` or a power cycle.
   See :ref:`ug_nrf93m1_power_optimization`.

Enable hardware flow control using ``RTS`` and ``CTS``.
At 3 Mbps without flow control, the host will drop bytes.

.. _ug_nrf93m1_interfaces_ppp:

PPP over CMUX
*************

Use this model when you want Zephyr sockets on the host, or when you have existing host networking code that expects a network interface rather than an AT command channel.

Zephyr modem cellular driver multiplexes the single UART into two CMUX channels:

* DLCI 1 carries the PPP data connection, which Zephyr's networking stack binds to as a network interface.
* DLCI 2 carries AT commands, used for signal quality and registration monitoring.

This gives you control and data over one physical UART, without a second serial port.

The nRF93M1 is supported through the ``nordic,nrf93m1`` compatible, implemented by the :file:`drivers/modem/vendor_modem_cellular/cellular_nordic_nrf93m1.c` file.
The nRF93M1 DK board files already include the modem node, so you do not need to write it yourself.
For a custom board, declare the module as a child of the host UART node:

.. code-block:: devicetree

   &uart21 {
           status = "okay";
           current-speed = <115200>;
           hw-flow-control;

           modem: modem {
                   compatible = "nordic,nrf93m1";
                   status = "okay";

                   /* Required */
                   mdm-power-gpios = <&gpio1 8 GPIO_ACTIVE_HIGH>;

                   /* Optional control and status signals */
                   mdm-reset-gpios = <&gpio1 9 GPIO_ACTIVE_HIGH>;
                   mdm-wake-gpios  = <&gpio1 10 GPIO_ACTIVE_HIGH>;
                   mdm-ring-gpios  = <&gpio1 11 GPIO_ACTIVE_HIGH>;
                   mdm-dtr-gpios   = <&gpio1 12 GPIO_ACTIVE_HIGH>;
           };
   };

.. note::
   These GPIO assignments are placeholders.
   Copy the actual mapping from the nRF93M1 DK board files, where the modem node is declared in the :file:`nrf93m1dk_nrf54l15_common_0_3_0.dtsi` file.

Binding properties
------------------

.. list-table::
   :header-rows: 1

   * - Property
     - Type
     - Purpose
   * - ``mdm-power-gpios``
     - phandle-array
     - Modem power control. **Required.**
   * - ``mdm-reset-gpios``
     - phandle-array
     - Modem reset.
   * - ``mdm-wake-gpios``
     - phandle-array
     - Modem wake.
   * - ``mdm-ring-gpios``
     - phandle-array
     - Ring indicator from the module.
   * - ``mdm-dtr-gpios``
     - phandle-array
     - Data terminal ready. Asserted when the UART is active, de-asserted when it is inactive, powered down, or in low-power mode.
   * - ``zephyr,mdm-reset-behavior``
     - string-array
     - When the driver drives the reset GPIOs. Legal values are ``hold_on_suspend``, ``toggle_on_resume``, and ``toggle_on_recovery``.
       Multiple values may be given. Defaults to ``toggle_on_recovery``.
   * - ``cmux-enable-runtime-power-save``
     - boolean
     - Use CMUX PSC commands for runtime power saving while keeping the data connection active.
   * - ``cmux-close-pipe-on-power-save``
     - boolean
     - Close the modem pipe, and therefore the UART, when entering power save. Requires waking the
       UART using the RING signal.
   * - ``cmux-idle-timeout-ms``
     - int
     - Idle time before CMUX enters power save. Defaults to 10000.
   * - ``autostarts``
     - boolean
     - Set when the modem starts by itself at power-on without an external power or reset pulse.
       The host then waits for a ready indication before sending AT commands.
   * - ``zephyr,use-default-pdp-ctx``
     - int
     - Set to ``1`` when the modem configures and activates the default PDP context itself.
       The driver then skips APN configuration and ``AT+CGACT``.
   * - ``zephyr,use-default-apn``
     - int
     - Set to ``1`` to supply an empty APN string when configuring the PDP context.

.. note::
   ``cmux-close-pipe-on-power-save`` and the module's RI signal work together.
   Closing the UART saves host power, but the module must then wake the host over RING.
   See :ref:`ug_nrf93m1_power_optimization_wake`.

Connection lifecycle
--------------------

The driver runs a state machine.
Knowing the states helps when you debug a connection that does not come up:

#. The driver pulses the power and reset GPIOs and waits for the module to respond.
#. It runs an initialization chat script over plain AT, disabling echo and querying IMEI, model, and firmware version.
   CMUX is not active yet.
#. It sends ``AT+CMUX`` to switch the module into multiplexed mode, then opens DLCI 1 and DLCI 2.
#. It configures the APN with ``AT+CGDCONT`` on DLCI 1 and dials.
#. PPP attaches to DLCI 1, AT monitoring moves to DLCI 2, and the driver brings the network carrier up once ``+CEREG`` reports registration.

A failure at initialization step is a wiring, baud rate, or power sequencing problem.
A failure in PPP attachment step is a SIM, APN, or coverage problem.

Enable the driver and networking with the following Kconfig options:

.. code-block:: cfg

   CONFIG_MODEM=y
   CONFIG_MODEM_CELLULAR=y
   CONFIG_NETWORKING=y
   CONFIG_NET_L2_PPP=y
   CONFIG_NET_IPV4=y
   CONFIG_NET_SOCKETS=y

Once the interface is up, the host uses standard Zephyr sockets.
The module still handles the LTE connection itself, so you configure APN and power saving through the AT channel, not through Zephyr.

.. note::
   Running PPP on the host means IP, and optionally TLS, run on the host.
   This costs host flash and RAM, and it means the host cannot sleep as deeply while a connection is open.
   If you do not need host sockets, the AT command model is more power efficient.

.. _ug_nrf93m1_interfaces_usb:

USB
***

The module presents a USB 2.0 device interface supporting RNDIS or CDC-ECM.
A host running Linux, Windows, or another rich OS enumerates the module as a network interface with no driver development.

Use this model for gateways, single-board computers, and any host where the OS already owns the network stack.

High speed operation reaches 480 Mbps on the USB link, well above the LTE throughput, so USB is not the bottleneck.
Full speed at 12 Mbps is also supported.

.. note::
   USB draws more current than UART and generally prevents the module from reaching its deepest sleep states.
   Do not use USB as the data interface in a battery-powered design that depends on PSM.
   USB ``VBUS`` detect is a wake source from System OFF, which is useful for mains-powered or intermittently connected designs.

.. _ug_nrf93m1_interfaces_coex:

Coexistence
***********

Two programmable coexistence pins let the module arbitrate with a co-located radio, such as a 2.4 GHz transceiver on the same board.
Configure the pin behavior with AT commands.

.. _ug_nrf93m1_interfaces_sim:

SIM interfaces
**************

The module provides two (e)UICC interfaces.
Both support Class B 1.8 V and Class C 3 V SIMs.
``SIM1_VOUT`` and ``SIM2_VOUT`` supply the cards, so the host does not need its own SIM regulator.

SIM hot-swap detect is a wake source from System OFF, which lets a device react to a SIM change without staying awake to poll.
