:orphan:

.. _migration_bicr_nrf54h:

Migrating nRF54H20 SoC BICR from DTS to JSON
############################################

.. contents::
   :local:
   :depth: 2

The nRF54H20 SoC requires a working Board Information Configuration Registers (BICR) configuration to function properly.

In the |NCS| v3.0.0, the format of the BICR configuration file was changed from DTS to JSON files.

Sources
*******

The following files contain the DTS and JSON format definitions for configuring BICR:

* `DTS format definition`_: ``nordic,nrf-bicr.yaml``.
* `JSON format definition`_: ``bicr-schema.json`` (from the |NCS| v3.0.0).

For examples of BICR configurations using both formats, see the following nRF54H20 DK board files:

* `BICR configuration example in DTS format`_, ``nrf54h20dk_bicr.dtsi`` (v2.9.0).
* `BICR configuration example in JSON format`_, ``bicr.json`` (v3.0.0).

Matching DTS configs to JSON
****************************

This chapter dissects the ``nrf54h20dk_bicr.dtsi`` DTS BICR configuration for the nRF54H20 DK and explains how to port the different parts to JSON.

compatible and reg
==================

The following is an example of the ``compatible`` and ``reg`` DTS options:

.. code-block:: dts

   compatible = "nordic,nrf-bicr";
   reg = <0xfff87b0 0x48>;

.. note::
   ``compatible`` and ``reg`` are DTS-specific.
   They are not needed for configuring BICR using JSON.

Supply configuration
====================

This DTS configuration controls the ``BICR`` → ``POWER.CONFIG`` register:

.. code-block:: dts

   power-vddao5v0 = "external";
   power-vddao1v8 = "internal";
   power-vdd1v0 = "internal";
   power-vddrf1v0 = "shorted";
   power-vddao0v8 = "internal";
   power-vddvs0v8 = "internal";

However, the nRF54H20 SoC currently supports only two hardware layout options.

The first layout (Config 1) is configured in DTS as follows:

.. code-block:: dts

   power-vddao5v0 = "external";
   power-vddao1v8 = "internal";
   power-vdd1v0 = "internal";
   power-vddrf1v0 = "shorted";
   power-vddao0v8 = "internal";
   power-vddvs0v8 = "internal";

The second layout (Config 2) is configured in DTS as follows:

.. code-block:: dts

   power-vddao5v0 = "shorted";
   power-vddao1v8 = "external";
   power-vdd1v0 = "internal";
   power-vddrf1v0 = "shorted";
   power-vddao0v8 = "internal";
   power-vddvs0v8 = "internal";

Using JSON, these supply options are configured in the ``power->scheme`` property.

The first layout (Config 1) is now configured in JSON as follows:

.. code-block:: json

   {
     "power": {
       "scheme": "VDDH_2V1_5V5"
     }
   }

The second layout (Config 2) is configured in JSON as follows:

.. code-block:: json

   {
     "power": {
       "scheme": "VDD_VDDH_1V8"
     }
   }

Inductor configuration
======================

This DTS option configures the ``INDUCTOR`` part of the register ``BICR`` → ``POWER.CONFIG``.

.. code-block:: dts

   inductor-present;

Since both supply configurations require the inductor, each JSON ``power->scheme`` option includes this component.
As such, if you have already set the ``power->scheme`` property, no additional configuration for the inductor is necessary.

GPIO power configuration
========================

This DTS option configures the ``BICR`` → ``IOPORT.POWER0`` and ``IOPORT.POWER1`` registers:

.. code-block:: dts

   ioport-power-rails = <&gpio1 2>, <&gpio2 2>, <&gpio6 2>, <&gpio7 2>, <&gpio9 4>;

In DTS, this configures GPIO ports.
For example ``<&gpio1 2>`` configures **P1**.

The number after the GPIO instance, from **P1** to **P7**, can be one of the following values:

+-------------------------+---------------------------+
| **P1** to **P7** GPIO   | **P1** to **P7** GPIO HEX |
| operating mode          | value in DTS              |
+=========================+===========================+
| Unconfigured            | ``0xF``                   |
+-------------------------+---------------------------+
| Disconnected            | ``0x0``                   |
+-------------------------+---------------------------+
| Shorted                 | ``0x1``                   |
+-------------------------+---------------------------+
| External1v8             | ``0x2``                   |
+-------------------------+---------------------------+

For P9, the number after the GPIO instance can be one of the following values:

+---------------------+-----------------------+
| **P9** GPIO         | **P9** GPIO HEX value |
| operating mode      | in DTS                |
+=====================+=======================+
| Unconfigured        | ``0xF``               |
+---------------------+-----------------------+
| Disconnected        | ``0x0``               |
+---------------------+-----------------------+
| Shorted             | ``0x1``               |
+---------------------+-----------------------+
| External1v8         | ``0x2``               |
+---------------------+-----------------------+
| ExternalFull        | ``0x4``               |
+---------------------+-----------------------+

The following is the DTS configuration from the nRF54H20 DK example:

.. code-block:: dts

   ioport-power-rails = <&gpio1 2>, <&gpio2 2>, <&gpio6 2>, <&gpio7 2>, <&gpio9 4>;

.. note::
   Configuration values use hexadecimal format, even if the ``0x`` prefix is not shown in the DTS syntax.

In the JSON configuration, GPIO port assignments are specified within the ``ioPortNumber`` object, with each mode explicitly indicated.

The available port configuration keys include:

+------+--------------------+
| Port | Port configuration |
|      | key                |
+======+====================+
| P1   | p1Supply           |
+------+--------------------+
| P2   | p2Supply           |
+------+--------------------+
| P6   | p6Supply           |
+------+--------------------+
| P7   | p7Supply           |
+------+--------------------+
| P9   | p9Supply           |
+------+--------------------+

The supported operating modes for these ports are:

+--------------------------+--------------------+
| Supported operating mode | JSON configuration |
+==========================+====================+
| Disconnected             | DISCONNECTED       |
+--------------------------+--------------------+
| Shorted                  | SHORTED            |
+--------------------------+--------------------+
| External1v8              | EXTERNAL_1V8       |
+--------------------------+--------------------+
| ExternalFull             | EXTERNAL_FULL      |
+--------------------------+--------------------+

Based on the DTS example, the corresponding JSON values are the following:

+----------------------+----------------------+----------------------+
| GPIO ports from the  | DTS configuration    | JSON configuration   |
| example              |                      |                      |
+======================+======================+======================+
| P1                   | ``<gpio1 2>``        | EXTERNAL_1V8         |
+----------------------+----------------------+----------------------+
| P2                   | ``<gpio2 2>``        | EXTERNAL_1V8         |
+----------------------+----------------------+----------------------+
| P6                   | ``<gpio6 2>``        | EXTERNAL_1V8         |
+----------------------+----------------------+----------------------+
| P7                   | ``<gpio7 2>``        | EXTERNAL_1V8         |
+----------------------+----------------------+----------------------+
| P9                   | ``<gpio9 4>``        | EXTERNAL_FULL        |
+----------------------+----------------------+----------------------+

The resulting JSON configuration is structured as follows:

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

GPIO power drive and impedance configuration
============================================

This DTS option configures the ``BICR`` → ``IOPORT.DRIVECTRL0`` register:

.. code-block:: dts

   ioport-drivectrls = <&gpio6 50>, <&gpio7 50>;

This section specifies the IO port impedance settings for **P6** and **P7**.

As with the GPIO power configuration, each GPIO reference indicates the corresponding port number, while the second value indicates the port's impedance in Ohms.

In this example, both **P6** and **P7** are configured to use a 50 Ohm impedance.

The nRF54H20 SoC allows you to select from these supported impedance values:

* 33 Ohms
* 40 Ohms
* 50 Ohms
* 66 Ohms
* 100 Ohms

The DTS configuration described in the nRF54H20 DK files can be represented in JSON as follows:

.. code-block:: json

   {
     "ioPortImpedance": {
       "p6ImpedanceOhms": 50,
       "p7ImpedanceOhms": 50
     }
   }

Low Frequency Oscillator (LFOSC) configuration
==============================================

The following DTS options configure parts of the ``BICR`` → ``LFOSC.LFXOCONFIG`` register:

.. code-block:: dts

   lfosc-mode = "crystal";
   lfosc-loadcap = <15>;

The following elements of the ``LFOSC.LFXOCONFIG`` register are defined in the BICR DTS format file:

* ACCURACY (set using ``lfosc-mode``)
* MODE (``lfosc-mode``)
* LOADCAP (``lfosc-loadcap``)
* TIME (``lfosc-startup``)

The available options for these settings are provided in the following tables.

The JSON format is inside ``lfosc: { lfxo: { ... } }``.

ACCURACY
--------

+----------------------+--------------------+
| DTS (lfosc-accuracy) | JSON (accuracyPPM) |
+======================+====================+
| 20                   | 20                 |
+----------------------+--------------------+
| 30                   | 30                 |
+----------------------+--------------------+
| 50                   | 50                 |
+----------------------+--------------------+
| 75                   | 75                 |
+----------------------+--------------------+
| 100                  | 100                |
+----------------------+--------------------+
| 150                  | 150                |
+----------------------+--------------------+
| 250                  | 250                |
+----------------------+--------------------+
| 500                  | 500                |
+----------------------+--------------------+

MODE
----

+------------------+-------------+
| DTS (lfosc-mode) | JSON (mode) |
+==================+=============+
| crystal          | CRYSTAL     |
+------------------+-------------+
| external-sine    | EXT_SINE    |
+------------------+-------------+
| external-square  | EXT_SQUARE  |
+------------------+-------------+
| disabled         |             |
+------------------+-------------+

LOADCAP
-------

+---------------------+---------------------------------+
| DTS (lfosc-loadcap) | JSON (builtInLoadCapacitancePf) |
+=====================+=================================+
| Integer [pF]        | Integer [pF], min 1, max 25     |
+---------------------+---------------------------------+

.. note::
   In the JSON configuration, the load capacitance parameter is only applied if the option ``builtInLoadCapacitors`` is explicitly set to true, as in the previous example.

TIME (LFXO startup time)
------------------------

+---------------------+-----------------------------+
| DTS (lfosc-startup) | JSON (startupTimeMs)        |
+=====================+=============================+
| Integer [ms]        | Integer [ms], min 1, max 25 |
+---------------------+-----------------------------+

The DTS configuration specifies only the ``MODE`` and ``LOADCAP`` parameters, relying on default values for all other settings.
The JSON configuration provides explicit control over all parameters.

See the following example:

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

Source
------

The ``source`` option in JSON can be one of the following:

* ``LFXO``, when an external crystal oscillator is in place.
* ``LFRC``, when an external Crystal Oscillator is not in place.

This means that the device can use either ``LFRC`` or ``SYNTH`` as clock sources.

LFRC autocalibration configuration
==================================

This DTS option configures the ``BICR`` → ``LFOSC.LFRCAUTOCALCONFIG`` register:

.. code-block:: dts

   lfrc-autocalibration = <20 40 3>;

The three values provided in the BICR DTS format correspond to the LFRC autocalibration configuration:

* ``temp-interval`` - Specifies the interval between temperature measurements, expressed in 0.25-second increments.
* ``temp-delta`` - Defines the temperature change, in 0.25-degree Celsius steps, that triggers a calibration event.
* ``interval-max-count`` - Indicates the maximum number of measurement intervals allowed between calibrations, regardless of temperature variations.

In the DTS property, each variable is mapped as follows:

* A: Temperature measurement interval (``temp-interval``)
* B: Temperature delta for calibration (``temp-delta``)
* C: Maximum intervals between calibrations (``interval-max-count``)

Use these parameters to precisely control the LFRC autocalibration behavior.

Datasheet register field
========================

+---------------------+------------------------------------+-------------------------------+
| DTS value           | JSON variable                      | JSON value                    |
+=====================+====================================+===============================+
| TEMPINTERVAL (A)    | tempMeasIntervalSeconds            | 5                             |
+---------------------+------------------------------------+-------------------------------+
| TEMPDELTA (B)       | tempDeltaCalibrationTriggerCelsius | 10                            |
+---------------------+------------------------------------+-------------------------------+
| INTERVALMAXNO (C)   | maxMeasIntervalBetweenCalibrations | 3                             |
+---------------------+------------------------------------+-------------------------------+
| ENABLE              | calibrationEnabled                 | -                             |
+---------------------+------------------------------------+-------------------------------+

If ``lfrc-autocalibration`` is set in the DTS, the ``ENABLE`` field in ``BICR`` → ``LFOSC.LFRCAUTOCALCONFIG`` is set automatically.

LFRC Autocalibration is not configured in the JSON configuration files for the DK, as the default values (``4``, ``0.5`` and ``2``) will be good enough for most use-cases.
However, the DTS example above would translate to JSON as such:

.. code-block:: json

   {
     "lfrccal": {
       "calibrationEnabled": true,
       "tempMeasIntervalSeconds": 5,
       "tempDeltaCalibrationTriggerCelsius": 10,
       "maxMeasIntervalBetweenCalibrations": 3
     }
   }

.. note::

   * Use the new default values in place of the old values from the DTS version.
   * ``tempDeltaCalibrationTriggerCelsius``: In the JSON BICR format, the maximum allowable value for this field is 31.75 °C.
     Therefore, 31.75 is used in place of 40 to ensure compatibility.

HFXO configuration and startup
==============================

This DTS option configures the ``BICR`` → ``HFXO.CONFIG`` register:

.. code-block:: dts

   hfxo-mode = "crystal";
   hfxo-loadcap = <56>;

Even if not used in the DTS example, the ``hfxo-startup`` parameter can also be configured to set the value of the ``BICR`` → ``HFXO.STARTUPTIME`` register.

The following table maps DTS and JSON configuration options for HFXO modes:

+------------------------+-------------------+-------------------------------+
| Datasheet register     | DTS variable      | JSON variable                 |
| field                  |                   | (within ``hfxo``)             |
+========================+===================+===============================+
| HFXO.CONFIG: MODE      | hfxo-mode         | mode                          |
+------------------------+-------------------+-------------------------------+
| HFXO.CONFIG: LOADCAP   | hfxo-loadcap      | builtInLoadCapacitancePf      |
+------------------------+-------------------+-------------------------------+
| HFXO.STARTUPTIME: TIME | hfxo-startup      | startupTimeUs*                |
+------------------------+-------------------+-------------------------------+

(*) Depends on ``”builtInLoadCapacitors”: true``

hfxo-mode
=========

+-----------------+-------------------+-------------------+
| Datasheet (MODE)| DTS (hfxo-mode)   | JSON (mode)       |
+=================+===================+===================+
| Crystal         | crystal           | CRYSTAL           |
+-----------------+-------------------+-------------------+
| ExtSquare       | external-square   |                   |
+-----------------+-------------------+-------------------+
| Unconfigured    |                   |                   |
+-----------------+-------------------+-------------------+

The corresponding JSON configuration, based on the previous table, is as follows:

.. code-block:: json

   {
     "hfxo": {
       "mode": "CRYSTAL",
       "builtInLoadCapacitors": true,
       "builtInLoadCapacitancePf": 14
     }
   }

For reference, the |NCS| v3.0.0 DK files indicate that the default values have been updated since the |NCS| v2.9.0.
The current standard configuration for the DK is as follows:

.. code-block:: json

   {
     "hfxo": {
       "mode": "CRYSTAL",
       "startupTimeUs": 850,
       "builtInLoadCapacitors": true,
       "builtInLoadCapacitancePf": 14
     }
   }
