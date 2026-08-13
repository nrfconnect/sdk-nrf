.. _ug_nrf93m1_custom_board:

Developing with a custom nRF93M1 board
######################################

.. contents::
   :local:
   :depth: 2

This page covers moving from the nRF93M1 DK to your own hardware.

When you replace the DK with your custom board you must select the host processor, and you are responsible for the RF and power design.
The module interface itself does not change.

.. _ug_nrf93m1_custom_board_host:

Choosing a host
***************

The module talks to the host over UART or USB, so the host does not have to be a Nordic device.
The following are the options:

.. list-table::
   :header-rows: 1

   * - Host
     - Integration model
     - Notes
   * - nRF54L15, as on the DK
     - AT commands or PPP over CMUX
     - Best supported path. Board files and samples exist.
   * - Another Nordic SoC
     - AT commands or PPP over CMUX
     - Supported through the generic Zephyr cellular modem driver.
   * - A third-party MCU
     - AT commands
     - No |NCS| dependency. Implement the AT interface on your own stack.
   * - A Linux or Windows system
     - USB RNDIS or CDC-ECM
     - The OS owns the network stack. No driver development.

If you are building a small battery-powered device, the AT command model on a modest host is the most power-efficient and lowest-footprint combination.
See :ref:`ug_nrf93m1_architecture`.

.. _ug_nrf93m1_custom_board_boardfiles:

Creating board files
********************

Follow :ref:`defining_custom_board` for the general process, then add the module.

For the AT command model, the module needs no devicetree node.
Configure the UART your host uses, and drive the control signals as ordinary GPIOs.

For the PPP model, declare the module as a child of the host UART node using the ``nordic,nrf93m1`` compatible.
Start from the nRF93M1 DK board files rather than writing them from scratch.
The modem node is declared in the :file:`nrf93m1dk_nrf54l15_common_0_3_0.dtsi` file, and copying it preserves the control signal mapping and the CMUX settings that are known to work.

For the full property list, see :ref:`ug_nrf93m1_interfaces_ppp`.
Only ``mdm-power-gpios`` is required, but a production design should also connect reset, DTR, and ring.

Two properties deserve a decision rather than a default:

``autostarts``
   Set it only if your hardware brings the module up without a power or reset pulse.
   If it is set wrongly, the driver waits for a ready indication that never arrives.

``cmux-close-pipe-on-power-save``
   Saves host power by closing the UART during power save, but then the module has to wake the host over RING.
   Do not enable it unless you wired ``mdm-ring-gpios``.

.. _ug_nrf93m1_custom_board_electrical:

Electrical design
*****************

.. list-table::
   :header-rows: 1

   * - Parameter
     - Requirement
   * - Supply voltage
     - Single VBAT rail to both VDD pins, 3.3 V to 4.5 V, 3.8 V typical
   * - GPIO logic level
     - 1.8 V, driven from the module's internal LDO. No external I/O supply required.
   * - ``LDO_OUT``
     - 1.8 V nominal, up to 120 mA, available to the host
   * - Operating temperature
     - -35 to +75 °C

The most common custom board issues are as follows:

Peak transmit current
   The module draws several hundred milliamps during transmit bursts.
   Your supply must hold regulation through those bursts, and your decoupling must be sized for them.
   A supply that works at idle and browns out on transmit produces registration failures that look like RF problems.

Level shifting
   Module GPIO logic is 1.8 V.
   If your host runs at 3.3 V, level shift every signal, including ``POWERKEY``, ``nRESET``, the UART lines, and the status pins.

Power sequencing
   Follow the sequencing in the `nRF93M1 Datasheet`_.
   Do not drive module pins while the module is unpowered.

.. _ug_nrf93m1_custom_board_rf:

RF and antenna design
*********************

Cat 1 bis uses a single receive antenna, so the module has one 50 ohm antenna pin, **ANT**.
There is no diversity or MIMO routing to lay out, which is a meaningful simplification compared with full Cat 1.

* Design the antenna feed as a 50 ohm controlled-impedance trace, kept as short as practical.
* Include a matching network footprint, even if your antenna does not require tuning.
* Follow the reference layout and keep-out guidance in the `nRF93M1 Datasheet`_ and the DK design files.
* Validate the antenna with ``AT%RFTEST``, which measures RSSI without a network connection.
  See :ref:`ug_nrf93m1_testing_rf`.

.. important::
   Antenna performance dominates real-world connectivity.
   A design that registers on a bench next to a base station might fail at the cell edge.
   Test at the edge of coverage, not only in good conditions.

.. _ug_nrf93m1_custom_board_variants:

Designing for both variants
***************************

The two variants in :ref:`ug_nrf93m1_variants` are pin, size, and software compatible.
Each variant requires its own board design and modem firmware image.
You select the variant by which part you place.

Design for this deliberately:

* Develop and qualify on nRF93M1-LABA, then place nRF93M1-LACA for markets that need the wider band set, with no PCB change.
* Keep the RF design suitable for the full frequency range, 617 MHz to 2.69 GHz, even if you ship the multi-regional variant first.
  The global variant uses bands the regional variant does not, and an antenna tuned only for the regional band set will underperform on them.

.. note::
   Regulatory approval applies to your product, not just to the module.
   Module pre-certification reduces your certification effort but does not remove it.
   Confirm the scope of the module certifications for your target markets and variant early, because this affects schedule more than it affects design.

.. _ug_nrf93m1_custom_board_checklist:

Design review checklist
***********************

The following is the design review checklist to be considered:

* Supply holds regulation through peak transmit current.
* All host-to-module signals are level shifted to 1.8 V if required.
* Power sequencing matches the datasheet.
* ``POWERKEY`` and ``nRESET`` are both host controllable.
* ``UART1_DTR`` is connected.
  It is the only baud-rate-independent wake path.
* ``UART1_RI`` is connected if the host will sleep and needs the module to wake it.
* Host ``CTS`` has a pull-up, required when using hardware flow control.
* **RFC1** through **RFC3** are left for RF tuner control, not repurposed as GPIO.
* UART hardware flow control lines are connected.
* Antenna feed is 50 Ω with a matching network footprint.
* Current measurement points are available for bring-up.
* A serial path exists for modem firmware recovery.
  See :ref:`ug_nrf93m1_updating_modem_fw_serial`.
* SIM interface matches your SIM class, 1.8 V Class B or 3 V Class C.
