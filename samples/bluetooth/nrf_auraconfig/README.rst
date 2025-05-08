.. _nrf_auraconfig:

nRF Auraconfig
##############

.. contents::
   :local:
   :depth: 2

The nRF Auraconfig sample implements the :ref:`BIS gateway mode <nrf53_audio_app_overview>` and may act as an `Auracast™`_ broadcaster if you are using a preset compatible with Auracast.
The sample features a shell interface that allows you to configure the broadcast source in many different ways.

In the BIS gateway mode, transmitting audio from the broadcast source happens using Broadcast Isochronous Stream (BIS) and Broadcast Isochronous Group (BIG).

.. note::
     This sample is meant to be used with maximum two BIG with four BIS streams each.

.. _nrf_auraconfig_requirements:

Requirements
************

The sample supports only and exclusively the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340_audio_dk_nrf5340

.. note::
   The sample supports PCA10121 revisions 1.0.0 or above.
   The sample is also compatible with the following pre-launch revisions:

   * Revisions 0.8.0 and above.

.. _nrf_auraconfig_ui:

Shell commands list
*******************

The nRF Auraconfig uses a shell interface for all user interactions.
This section lists the supported commands.

Description convention
======================

The command argument description uses the following convention:

* Angle brackets mean that an argument is mandatory:

  .. parsed-literal::
     :class: highlight

     nac <*arg*>

* Square brackets mean that an argument is optional:

  .. parsed-literal::
     :class: highlight

     nac [*arg*]

----

start
=====

Start the broadcaster with the current configuration.
If no configuration is set, the broadcaster will not start.

To view the current configuration, use the ``show`` command.
The optional argument sets the index of the BIG to start.

Usage:

.. code-block:: console

   nac start [BIG_index]

Examples:

* The following commands starts any BIG that is configured:

  .. code-block:: console

     nac start

* The following command starts only the BIG 0:

  .. code-block:: console

     nac start 0

----

stop
====

Stop the broadcaster.
The optional argument sets the index of the BIG to stop.

Usage:

.. code-block:: console

   nac stop [BIG_index]

Examples:

* The following commands stops any BIG that is running:

  .. code-block:: console

     nac stop

* The following command stops only the BIG 0:

  .. code-block:: console

     nac stop 0

----

show
====

Shows the configuration for the different BIGs that are currently configured.

Usage:

.. code-block:: console

   nac show

Example output:

.. code-block:: console

   BIG 0:
       Streaming: false
       Advertising name: Lecture hall
       Broadcast name: Lecture
       Packing: interleaved
       Encryption: false
       Broadcast code:
       Subgroup 0:
               Preset: 24_2_1
                       PHY: 2M
                       Framing: unframed
                       RTN: 2
                       SDU size: 60
                       Max Transport Latency: 10 ms
                       Frame Interval: 10000 us
                       Presentation Delay: 40000 us
               Sampling rate: 24000 Hz
               Bitrate: 48000 bps
               Frame duration: 10000 us
               Octets per frame: 60
               Language: eng
               Context(s):
                       Live
               Program info: Mathematics 101
               Immediate rendering flag: set
               Number of BIS: 1
               Location:
                       BIS 0: Mono Audio

----

file list
=========

Lists the files and directories in the given directory on the SD card.
If no directory is given, contents of the root directory is listed.

Usage:

.. code-block:: console

      nac file list [directory]

Example output:

.. code-block:: console

   nac file list

   [DIR ]  16000hz
   [DIR ]  24000hz
   [DIR ]  32000hz
   [FILE]  left-channel-announcement.wav
   [FILE]  right-channel-announcement.wav

----

file select
===========

Selects a file from the SD card to be used as the audio source for the given stream.
The file must be in the LC3 format, and one file may be used for multiple streams at the same time.

Usage:

.. code-block:: console

   nac file select <file> <BIG index> <subgroup index> <BIS index>

Example:

  .. code-block:: console

     nac file select 16000hz/24_kbps/left-channel-announcement_16kHz_left_24kbps.lc3 1 2 0

This command selects the file :file:`16000hz/24_kbps/left-channel-announcement_16kHz_left_24kbps.lc3` for the BIS 0 in the subgroup 2 in the BIG 1.

----

packing
=======

Set the type of packing for the BIS streams, either sequential or interleaved.

Usage:

.. code-block:: console

   nac packing <seq/int> <BIG index>

Example:

.. code-block:: console

   nac packing int 0

This command sets the packing for the BIG 0 to interleaved.

----

preset
======

Set the BAP preset for a BIG or subgroup.
The presets are defined in the `Basic Audio Profile specification`_.
The supported presets can be listed with the ``nac preset print`` command.

Usage:

.. code-block:: console

   nac preset <preset> <BIG index> [subgroup index]

Examples:

* The following command sets the preset for the BIG 0 to ``24_2_1``:

  .. code-block:: console

     nac preset 24_2_1 0

* The following command sets the preset for the subgroup 0 in the BIG 0 to ``24_2_1``:

  .. code-block:: console

     nac preset 24_2_1 0 0

----

lang
====

Set the language metadata for a subgroup.

Usage:

.. code-block:: console

   nac lang <language> <BIG index> <Subgroup index>

The ``language`` argument is a three-letter `ISO 639-2 code`_.

Example:

.. code-block:: console

   nac lang eng 0 0

This command sets the language for the subgroup 0 in the BIG 0 to English.

----

immediate
=========

Set the immediate rendering flag for a subgroup.

Usage:

.. code-block:: console

   nac immediate <0/1> <BIG index> <Subgroup index>

Example:

.. code-block:: console

   nac immediate 1 0 0

This command sets the immediate rendering flag for the subgroup 0 in the BIG 0 to ``true``.

----

num_subgroups
=============

Set the number of subgroups for a BIG.
The maximum number of subgroups for each BIG is set by :kconfig:option:`CONFIG_BT_BAP_BROADCAST_SRC_SUBGROUP_COUNT`.

Usage:

.. code-block:: console

   nac num_subgroups <number> <BIG index>

Example:

.. code-block:: console

   nac num_subgroups 2 0

This command sets the number of subgroups for the BIG 0 to 2.

----

num_bises
=========

Set the number of BISes for a given subgroup.
The maximum number of BISes for each subgroup is set by :kconfig:option:`CONFIG_BT_BAP_BROADCAST_SRC_STREAM_COUNT`.

Usage:

.. code-block:: console

   nac num_bises <number> <BIG index> <Subgroup index>

Example:

.. code-block:: console

   nac num_bises 2 0 0

This command sets the number of BISes for the subgroup 0 in the BIG 0 to 2.

----

context
=======

Sets the context metadata for a subgroup.
The supported contexts can be listed with the ``nac context print`` command.

Usage:

.. code-block:: console

   nac context <context> <BIG index> <Subgroup index>

Example:

.. code-block:: console

   nac context Media 0 0

This command sets the context for the subgroup 0 in the BIG 0 to Media.

----

location
========

Set the location metadata for a BIS.
The supported locations can be listed with the ``nac location print`` command.

Usage:

.. code-block:: console

   nac location <location> <BIG index> <Subgroup index> <BIS index>

Example:

.. code-block:: console

   nac location fl 0 0 0

This command sets the location for the BIS 0 in the subgroup 0 in the BIG 0 to Front Left.

----

broadcast_name
==============

Set the broadcast name metadata for a BIG.

Usage:

.. code-block:: console

   nac broadcast_name <name> <BIG index>

Examples:

* The following command sets the broadcast name for the BIG 0 to ``Lecture``:

  .. code-block:: console

     nac broadcast_name Lecture 0

* The following command uses quotation marks for a longer name with spaces:

  .. code-block:: console

     nac broadcast_name "Lecture hall" 0

----

encrypt
=======

Set the broadcast code by enabling or disabling encryption for a given BIG.
The broadcast code is a 16-character string that is used to encrypt the broadcast, but shorter codes are also possible.

Usage:

.. code-block:: console

   nac encrypt <0/1> <BIG index> [broadcast_code]

Examples:

* The following command enables encryption for the BIG 0 with the broadcast code "Auratest":

  .. code-block:: console

     nac encrypt 1 0 Auratest

* The following command disables encryption for the BIG 0 after setting it:

  .. code-block:: console

     nac encrypt 0 0

----

usecase
=======

Set a pre-defined use case.
A use case is a set of configurations that are commonly used together.
The command typically sets the number of subgroups, the number of BISes, the context, the location, and potentially metadata.

All pre-defined use cases can be listed with the ``nac usecase print`` command.

Usage:

.. code-block:: console

   nac usecase <use_case>

The ``use_case`` argument can either be an index or the name of a use case.

Example:

.. code-block:: console

   nac usecase 0

This command sets a unique configuration for the given use case and then calls ``show`` to display the configuration.

----

clear
=====

Clear any previous configuration.

Usage:

.. code-block:: console

   nac clear

----

adv_name
========

Set the advertising name metadata for a BIG.

.. note::
    Make sure each BIG has a unique advertising name.

Usage:

.. code-block:: console

   nac adv_name <name> <BIG index>

The maximum length of the advertising name is given by :kconfig:option:`CONFIG_BT_DEVICE_NAME_MAX`.

Example:

.. code-block:: console

   nac adv_name "Lecture hall" 0

This command sets the advertising name for the BIG 0 to ``Lecture hall``.

.. note::
    The name must be enclosed in quotation marks if it contains spaces.

----

program_info
============

Set the program info metadata for a subgroup.

Usage:

.. code-block:: console

   nac program_info <info> <BIG index> <Subgroup index>

Example:

.. code-block:: console

   nac program_info "Mathematics 101" 0 0

This command sets the program info for the subgroup 0 in the BIG 0 to ``Mathematics 101``.

----

phy
===

Set the PHY for a BIG.
The supported PHY values are:

* ``1`` - 1M
* ``2`` - 2M
* ``4`` - Coded

Usage:

.. code-block:: console

   nac phy <phy> <BIG index> <subgroup index>

Example:

.. code-block:: console

   nac phy 2 0 0

This command sets the PHY for the subgroup 0 in the BIG 0 to 2M.

----

framing
=======

Set the framing for a subgroup.

Usage:

.. code-block:: console

   nac framing <unframed/framed> <BIG index> <subgroup index>

Example:

.. code-block:: console

   nac framing unframed 0 0

This command sets the framing for the subgroup 0 in the BIG 0 to ``unframed``.

----

rtn
===

Set the number of retransmits for a subgroup.

Usage:

.. code-block:: console

   nac rtn <number> <BIG index> <subgroup index>

Example:

.. code-block:: console

   nac rtn 2 0 0

This command sets the number of retransmits for the subgroup 0 in the BIG 0 to 2.

----

sdu
===

Set the Service Data Unit (SDU) size in octets for a subgroup.

.. note::

   This command does not change the bitrate, only the size of the SDU.

Usage:

.. code-block:: console

   nac sdu <octets> <BIG index> <subgroup index>

Example:

.. code-block:: console

   nac sdu 60 0 0

This command sets the SDU size for the subgroup 0 in the BIG 0 to 60 octets.

----

mtl
===

Set the maximum transport latency for a subgroup, in milliseconds.

Usage:

.. code-block:: console

   nac mtl <ms> <BIG index> <subgroup index>

Example:

.. code-block:: console

   nac mtl 10 0 0

This command sets the maximum transport latency for the subgroup 0 in the BIG 0 to 10 ms.

----

frame_interval
==============

Set the frame interval for a subgroup, in microseconds.
The command supports the following values:

* 7500
* 10000

Usage:

.. code-block:: console

   nac frame_interval <us> <BIG index> <subgroup index>

Example:

.. code-block:: console

   nac frame_interval 10000 0 0

This command sets the frame interval for the subgroup 0 in the BIG 0 to 10 ms (10000 us).

----

pd
==

Set the presentation delay for a subgroup, in microseconds.

Usage:

.. code-block:: console

   nac pd <us> <BIG index> <subgroup index>

Example:

.. code-block:: console

   nac pd 40000 0 0

This command sets the presentation delay for the subgroup 0 in BIG0 to 40 ms (40000 us).

----

broadcast_id fixed
==================

Set a fixed broadcast ID for a BIG.
The broadcast ID is used to identify the broadcast.
Its value is three octets long.

Usage:

.. code-block:: console

   nac broadcast_id fixed <BIG index> <broadcast_id in hexadecimal (3 octets)>

Examples:

.. code-block:: console

   nac broadcast_id fixed 0 0xAA1234

This command sets a fixed broadcast ID for the BIG 0 to ``0xAA1234``.
This value will remain if the broadcast is stopped and started again.

----

broadcast_id random
===================

Set a random broadcast ID for a BIG.
The broadcast ID is used to identify the broadcast.
The broadcast ID will be generated anew every time the broadcaster is started.

Usage:

.. code-block:: console

   nac broadcast_id random <BIG index>

Examples:

.. code-block:: console

   nac broadcast_id random 0

This command sets a random broadcast ID for the BIG 0 each time it is started.

----

.. _nrf_auraconfig_configuration:

Configuration
*************

|config|

The sample is pre-configured with a generous default memory allocation, suitable for a wide range of use cases.
You can modify these default settings in the :file:`prj.conf` file.
Using aggressive configurations can reduce air time availability for all streams, depending on the combination of options selected (like high bitrates, increased re-transmits, specific PHY settings).

.. _nrf_auraconfig_configuration_sd:

SD card setup
*************

This sample can support pre-encoded LC3 data stored as LC3 files on an SD card.
You can use the `nRF Auracast configuration files`_ provided by Nordic Semiconductor for populating the SD card.

If you are not using an SD card, the system defaults to sending dummy data.
The purpose of the dummy data is to test that the broadcast source has been correctly configured.

Make sure you format the SD card with a FAT file system.

.. _nrf_auraconfig_building:

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/nrf_auraconfig`

.. include:: /includes/build_and_run.txt

.. |sample_or_app| replace:: sample
.. |ipc_radio_dir| replace:: :file:`sysbuild/ipc_radio`

.. include:: /includes/ipc_radio_conf.txt

The nRF5340 Audio DK comes pre-programmed with basic firmware that indicates if the kit is functional.
See :ref:`nrf53_audio_app_dk_testing_out_of_the_box` for more information.

.. _nrf_auraconfig_testing:

Testing
*******

In this testing procedure, the development kit is programmed with the nRF Auraconfig sample.

To test the nRF Auraconfig sample, complete the following steps:

1. If you are using an :ref:`SD card loaded with the pre-encoded LC3 data <nrf_auraconfig_configuration_sd>`, insert it into your development kit.
#. Turn on the development kit.
#. Set up the serial connection with the development kit.
#. Configure a BIG using use case 1:

   .. code-block:: console

      nac usecase 1

#. Start the broadcaster:

   .. code-block:: console

      nac start

You can now send other shell commands, as listed in the :ref:`nrf_auraconfig_ui`.

Dependencies
************

For the list of dependencies, check the application's source files.
