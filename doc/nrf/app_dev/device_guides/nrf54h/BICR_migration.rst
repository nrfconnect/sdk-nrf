:orphan:

Migrating BICR configuration from DTS to JSON
=============================================

The Board Information Configuration Registers (BICR) must be set correctly for the nRF54H20 device to operate as intended.

Starting with nRF Connect SDK v3.0.0, the BICR configuration file format changed from Device Tree Source (DTS) to JSON.

References
----------

- DTS format definition: ``nordic,nrf-bicr.yaml``
- JSON format definition: ``bicr-schema.json`` (v3.0.0)
- Example files in the nRF54H20 DK board directory:
  - DTS: ``nrf54h20dk_bicr.dtsi`` (v2.9.0)
  - JSON: ``bicr.json`` (v3.0.0)
- See the nRF54H20 Preliminary Product Specification, section 3.8.5.1 "BICR — Board Information Configuration Registers"

Field mapping: DTS to JSON
--------------------------

This section describes how to convert fields from ``nrf54h20dk_bicr.dtsi`` to the JSON format.

compatible and reg
~~~~~~~~~~~~~~~~~~

The ``compatible`` and ``reg`` properties are specific to DTS and are not used in the JSON format.

Supply configuration
~~~~~~~~~~~~~~~~~~~~

.. code-block:: dts

   power-vddao5v0 = "external";
   power-vddao1v8 = "internal";
   power-vdd1v0 = "internal";
   power-vddrf1v0 = "shorted";
   power-vddao0v8 = "internal";
   power-vddvs0v8 = "internal";

These properties configure the BICR → POWER.CONFIG register. The nRF54H20 supports two hardware layout options. For details, see section 9.3 "Reference circuitry" in the product specification.

.. image:: image-20250625-164812.png

DTS Config 1:

.. code-block:: dts

   power-vddao5v0 = "external";
   power-vddao1v8 = "internal";
   power-vdd1v0 = "internal";
   power-vddrf1v0 = "shorted";
   power-vddao0v8 = "internal";
   power-vddvs0v8 = "internal";

DTS Config 2:

.. code-block:: dts

   power-vddao5v0 = "shorted";
   power-vddao1v8 = "external";
   power-vdd1v0 = "internal";
   power-vddrf1v0 = "shorted";
   power-vddao0v8 = "internal";
   power-vddvs0v8 = "internal";

In JSON, use the ``power.scheme`` property to select the configuration:

.. code-block:: json

   {
     "power": {
       "scheme": "CONFIG_1"
     }
   }

or

.. code-block:: json

   {
     "power": {
       "scheme": "CONFIG_2"
     }
   }

Inductor Configuration
----------------------

This configures the INDUCTOR part of the register BICR → POWER.CONFIG.

Both supply configurations include the inductor, so both options for ``scheme`` in the JSON configuration include the inductor. No extra configuration is needed for the inductor if ``power->scheme`` is set.

GPIO Power Configuration
------------------------

.. code-block:: dts

   ioport-power-rails = <&gpio1 2>, <&gpio2 2>, <&gpio6 2>, <&gpio7 2>, <&gpio9 4>;

This configures the BICR → IOPORT.POWER0 and IOPORT.POWER1 registers.

For DTS, this configures GPIO ports. For example, ``<&gpio1 2>`` configures P1.
The number after the GPIO instance is as follows (for P1 to P7):

- 0xF: Unconfigured
- 0x0: Disconnected
- 0x1: Shorted
- 0x2: External1v8

See datasheet section 3.8.5.1.2.2 IOPORT.POWER0 for more information.

For P9, the values differ:

- 0xF: Unconfigured
- 0x0: Disconnected
- 0x1: Shorted
- 0x2: External1v8
- 0x4: ExternalFull

See datasheet section 3.8.5.1.2.3 IOPORT.POWER1 for more information.

The DTS conversion from the DK example is:

.. code-block:: dts

   ioport-power-rails = <&gpio1 2>, <&gpio2 2>, <&gpio6 2>, <&gpio7 2>, <&gpio9 4>;

The numbers are in hex form, even though they do not have ``0x`` in DTS.

- P1: External1v8
- P2: External1v8
- P6: External1v8
- P7: External1v8
- P9: ExternalFull

For the JSON format, GPIO ports are listed under the ``ioPortPower`` object, and modes are listed in full.
Available port configurations: ``p1Supply``, ``p2Supply``, ``p6Supply``, ``p7Supply``, ``p9Supply``.
Available modes: ``DISCONNECTED``, ``SHORTED``, ``EXTERNAL_1V8``, and ``EXTERNAL_FULL`` (only for P9).

Therefore, the example for the DK becomes:

.. code-block:: json

   {
     "ioPortPower": {
       "p1Supply": "EXTERNAL_1V8",
       "p2Supply": "EXTERNAL_1V8",
       "p6Supply": "EXTERNAL_1V8",
       "p7Supply": "EXTERNAL_1V8",
       "p9Supply": "EXTERNAL_FULL"
     }
   }

GPIO Power Drive/Impedance Configuration
----------------------------------------

This configures the BICR → IOPORT.DRIVECTRL0 register.

This configuration controls IO port impedance for P6 and P7.

As with the GPIO power configuration, the GPIO reference is for GPIO port numbers.
The second number is the port impedance configuration in Ohms.
For example, both P6 and P7 can be configured for 50 Ohms.
The nRF54H20 supports a limited set of resistances: 33, 40, 50, 66, and 100 Ohms.

For the JSON format, this will be:

.. code-block:: json

   {
     "ioPortImpedance": {
       "p6ImpedanceOhm": 50,
       "p7ImpedanceOhm": 50
     }
   }

Low Frequency Oscillator (LFOSC) Configuration
----------------------------------------------

These configure parts of the BICR → LFOSC.LFXOCONFIG register.

All parts of LFOSC.LFXOCONFIG are defined in the DTS format file: ACCURACY (``lfosc-accuracy``), MODE (``lfosc-mode``), LOADCAP (``lfosc-loadcap``), and TIME (``lfosc-startup``). Options for these are also defined in the datasheet, DTS, and JSON format files.

The JSON format is inside ``lfosc: { lfxo: { ... } }``.

**ACCURACY**

- DTS (``lfosc-accuracy``): 20, 30, 50, 75, 100, 150, 250, 500
- JSON (``accuracyPPM``): 20, 30, 50, 75, 100, 150, 250, 500

**MODE**

- DTS (``lfosc-mode``): crystal, external-sine, external-square, disabled
- JSON (``mode``): CRYSTAL, EXT_SINE

**LOADCAP**

- DTS (``lfosc-loadcap``): Integer [pF]
- JSON (``builtInLoadCapacitancePf``): Integer [pF], min 1, max 25

Note: The JSON loadcap depends on ``builtInLoadCapacitors`` being set to ``true``.

**TIME (LFXO Startup Time)**

- DTS (``lfosc-startup``): Integer [ms]
- JSON (``startupTimeMs``): Integer [ms], min 1, max 25

While the DTS DK configuration only configures MODE and LOADCAP, the JSON configuration configures all:

.. code-block:: json

   {
     "lfosc": {
       "source": "LFXO",
       "lfxo": {
         "mode": "CRYSTAL",
         "accuracyPPM": 20,
         "startupTimeMs": 600,
         "builtInLoadCapacitancePf": 15,
         "builtInLoadCapacitors": true
       }
     }
   }

**Source**

The ``source`` field in the JSON configuration can be either ``LFXO`` or ``LFRC``.
TODO: What does the LFRC do here?

LFRC Autocalibration Configuration
----------------------------------

Configures the BICR → LFOSC.LFRCAUTOCALCONFIG register.

The three values, as described in the BICR DTS format:

- A list of values pertaining to LFRC autocalibration settings. The property is encoded as ``<temp-interval temp-delta interval-max-count>``, where:
  - ``temp-interval``: temperature measurement interval in 0.25s steps
  - ``temp-delta``: temperature delta that should trigger a calibration in 0.25 degree steps
  - ``interval-max-count``: max number of temp-interval periods between calibrations, independent of temperature changes

To convert from the example values (``<A B C>``):

- TEMPINTERVAL (A): ``tempMeasIntervalSeconds`` (e.g., 20)
- TEMPDELTA (B): ``tempDeltaCalibrationTriggerCelsius`` (e.g., 31.75, max in JSON)
- INTERVALMAXNO (C): ``maxMeasIntervalBetweenCalibrations`` (e.g., 3)
- ENABLE: ``calibrationEnabled``

If ``lfrc-autocalibration`` is set in DTS, the ENABLE field in BICR → LFOSC.LFRCAUTOCALCONFIG will be set automatically.

LFRC Autocalibration is not configured in the JSON configuration files for the DK, as the default values are sufficient for most use cases. However, the DTS example above would translate to JSON as:

.. code-block:: json

   {
     "lfrccal": {
       "calibrationEnabled": true,
       "tempMeasIntervalSeconds": 20,
       "tempDeltaCalibrationTriggerCelsius": 31.75,
       "maxMeasIntervalBetweenCalibrations": 3
     }
   }

*Note*: The field ``tempDeltaCalibrationTriggerCelsius`` has a max of 31.75 in the JSON BICR format.

HFXO Configuration & Startup
----------------------------

Configures the BICR → HFXO.CONFIG register. Not used in the DTS example, but ``hfxo-startup`` is also configurable, which would configure the BICR → HFXO.STARTUPTIME register.

Here is a table to match between DTS and JSON modes:

+---------------------+-------------------+-----------------------------+
| DTS                | JSON              | Description                 |
+=====================+===================+=============================+
| hfxo-mode           | mode              | HFXO.CONFIG: MODE           |
| hfxo-loadcap        | builtInLoadCapacitancePf | HFXO.CONFIG: LOADCAP  |
| hfxo-startup        | startupTimeUs     | HFXO.STARTUPTIME: TIME      |
+---------------------+-------------------+-----------------------------+

HFXO MODE options:

- Datasheet (MODE): Crystal, ExtSquare, Unconfigured
- DTS (hfxo-mode): crystal, external-square
- JSON (mode): CRYSTAL

Following this table, the example in JSON form becomes:

.. code-block:: json

   {
     "hfxo": {
       "mode": "CRYSTAL",
       "builtInLoadCapacitors": true,
       "builtInLoadCapacitancePf": 56
     }
   }

However, as seen from the v3.0.0 DK files, the values have been tuned since v2.9.0, so the default values for the DK are now:

.. code-block:: json

   {
     "hfxo": {
       "mode": "CRYSTAL",
       "startupTimeUs": 850,
       "builtInLoadCapacitors": true,
       "builtInLoadCapacitancePf": 14
     }
   }