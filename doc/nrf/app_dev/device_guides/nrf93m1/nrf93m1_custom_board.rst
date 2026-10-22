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
     - –35 to +75 °C ambient

The most common custom board issues are as follows:

Peak transmit current
   The module draws several hundred milliamps during transmit bursts.
   At 23 dBm, the typical average current is about 515 to 605 mA on bands B8, B28, B3, and B7, and about 225 mA on B40.
   Your supply must hold regulation through those bursts, and your decoupling must be sized for them.
   Place the bulk and high-frequency decoupling capacitors close to the VDD pins, following the nRF93M1 Hardware Design Guidelines.
   A supply that works at idle and browns out on transmit produces registration failures that look like RF problems.

Undervoltage
   If the VDD voltage falls below the minimum operating voltage, the module goes to minimum functionality.
   The host must send ``AT+CFUN=1`` to restore full functionality.

Level shifting
   Module GPIO logic is 1.8 V.
   If your host runs at 3.3 V, level shift every signal, including ``POWERKEY``, ``nRESET``, the UART lines, and the status pins.

Power sequencing
   Follow the sequencing in the `nRF93M1 Datasheet`_.
   Do not drive module pins while the module is unpowered.
   When VDD is applied, the module stays in System Disabled until you pull ``POWERKEY`` low:

   * ``POWERKEY`` has an internal pull-up.
     Pull it low for at least 10 ms to start the module, and for at least 650 ms to power it down.
   * ``nRESET`` has an internal 120 kΩ pull-up, so no external pull-up is needed, and you can leave it floating if you do not use it.
     Pull it low for at least 10 ms to reset the module.
     Use it only for emergency or production-line reset, and use the AT reset commands in normal operation.

Thermal design
   The ambient operating temperature range is –35 to +75 °C.
   To keep the module within limits at elevated ambient temperature, use a copper pour without thermal relief under the module footprint.
   Add thermal vias beneath the module if the host PCB has inner ground planes.
   Do not place heat-generating components, such as high-current regulators, RF power amplifiers, or the host CPU, directly next to the module.
   Read the on-chip temperature with ``AT%ADC="temp"``.

.. _ug_nrf93m1_custom_board_rf:

RF and antenna design
*********************

Cat 1 bis uses a single receive antenna, so the module has one 50 ohm antenna pin, **ANT**.
There is no diversity or MIMO routing to lay out, which is a meaningful simplification compared with full Cat 1.

* Design the antenna feed as a 50 ohm controlled-impedance trace, kept as short as practical.
* Include a matching network footprint, even if your antenna does not require tuning.
* Follow the reference layout and keep-out guidance in the `nRF93M1 Datasheet`_ and the DK design files.
* Use the 3-pin RF control interface (**RFC1** to **RFC3**) to control an external antenna tuner.
  It covers the 700 MHz to 2700 MHz range and can improve radiated efficiency in compact single-antenna designs.
* If a companion radio such as an external positioning or Bluetooth device shares the antenna area, use the two-pin coexistence interface.
  For both interfaces, see the `nRF93M1 AT Commands Reference Guide`_.
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

The variants differ in SIM support.
Only nRF93M1-LACA supports SoftSIM.
When you use two external SIMs, both must operate at the same voltage class.
If you do not use ``SIM1_DET``, leave it floating.

Design for this deliberately:

* Develop and qualify on nRF93M1-LABA, then place nRF93M1-LACA for markets that need the wider band set, with no PCB change.
* Keep the RF design suitable for the full frequency range, 617 MHz to 2.69 GHz, even if you ship the multi-regional variant first.
  The global variant uses bands the regional variant does not, and an antenna tuned only for the regional band set will underperform on them.

.. note::
   Regulatory approval applies to your product, not just to the module.
   Module pre-certification reduces your certification effort but does not remove it.
   Confirm the scope of the module certifications for your target markets and variant early, because this affects schedule more than it affects design.

.. _ug_nrf93m1_custom_board_assembly:

Assembly
********

Follow the assembly guidance in the `nRF93M1 Datasheet`_:

* Use a reflow profile compatible with IPC-7530, with a peak temperature of 240 °C ±5 °C, and do not exceed three reflow cycles.
* Connect the RESERVED pins thermally and mechanically to the host PCB, but leave them electrically unconnected.
* Route the **ANT** pin with a controlled-impedance 50 Ω transmission line that is short and isolated from digital signal sources.
* Observe the dry pack storage and floor life limits (MSL 3: 168 hours at 30 °C or lower and 60% RH or lower), and bake the modules according to JEDEC J-STD-033 if the limits are exceeded.

.. _ug_nrf93m1_custom_board_checklist:

Design review checklist
***********************

The following is the design review checklist to be considered:

* Supply holds regulation through peak transmit current.
* All host-to-module signals are level shifted to 1.8 V if required.
* Power sequencing matches the datasheet.
* ``POWERKEY`` is host controllable.
  ``nRESET`` is optional and can be left floating.
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
* SIM interface matches your SIM class, 3 V Class B or 1.8 V Class C.
  With two SIMs, both use the same class.
