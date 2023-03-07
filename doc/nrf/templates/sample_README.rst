.. _sample:

Template: Sample
################

.. contents::
   :local:
   :depth: 2

.. note::
   * Give the sample a concise name that corresponds to the folder name.
     If the sample targets a specific protocol or device, add it in the title before the sample name (for example, "NFC:" or "nRF9160:").
     Do not include the word "sample" in the title.
   * Put the documentation inside the sample folder and use the file name :file:`README.rst`.
   * Use the provided stock phrases and includes when possible.
   * Sections with * are optional and can be left out.
     All other sections are required for all samples.
     Do not add new sections (unless in the sections that allow for further subsections) without discussion with the tech writer team.

The Sample sample demonstrates how to blink LEDs in the rhythm of the music that is played.

.. tip::
   Explain what this sample demonstrates in one, max two sentences (full sentences, not sentence fragments).
   This introduction should give users a clear idea of what the sample can be used for.

   Think about use cases:
   "The XYZ sample shows how to use the XYZ library" is not a good introduction, because customers do not want to use the XYZ library, but they want to get a task done.
   Instead, write something like "The XYZ sample shows how to do this awesome task. It uses the XYZ library".


Requirements
************

.. note::
   * Supported kits are listed in a table, which is composed of rows from the :file:`doc/nrf/includes/sample_board_rows.txt` file.
     Select the required rows in the ``:rows:`` configuration, or use the ``.. table-from-sample-yaml::`` directive to include all build targets specified in the :file:`sample.yaml` file.
   * If only one kit is supported, replace the introduction text with "The sample supports the following development kit:".
   * If several kits are required to test the sample, state it after the table (for example, "You can use one or more of the development kits listed above and mix different development kits.").
   * Mention additional requirements after the table.
   * If TFM is included in the sample, add ``.. include:: /includes/tfm.txt`` to include the standard text that states this.

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52dk_nrf52832, nrf52840dk_nrf52840

The sample also requires ...

.. include:: /includes/tfm.txt


Overview
********

You can use this sample as a starting point to implement a disco light.

The sample uses the ``:ref:RST link`` library to control the LEDs.
In addition, it uses the ``:ref:RST link`` sound sensor and hooks up to some Internet service to download cool blinking sequences.

.. tip::
   Continue the explanation on what this sample is about.
   What does it show, and why should users try it out?
   What is the practical use?
   How can users extend this sample?
   What libraries are used (link to them)?


Some title*
===========
.. note::
   Add subsections for technical details.
   Give them a suitable title (sentence style capitalization, thus only the first word capitalized).
   If there is nothing important to point out, do not include any subsections.


The sample repeatedly calls function ABC, which ...

.. tip::
   Do not just list the functions that are called, but clarify general concepts or explain parts of the implementation that might be unintuitive for some reason.


Wiring*
*******

Connect PIN1 to PIN2, then cut the connection again.

User interface*
***************
.. note::
   Add button and LED assignments here, plus other information related to the user interface (for example, supported commands).

Button 1:
   Does something.

Button 2:
   Toggles something.

LED 1:
   On when connected.

LED 2:
   Indicates something.


Configuration*
**************

.. note::
   Always include this section if your sample can be configured by the user.
   Start with the stock phrase that is included with ``|config|``.

|config|

Setup*
======

.. note::
   Add information about the initial setup here, for example, that the user must install or enable some library before they can compile this sample, or set up and select a specific backend.
   Most samples do not need this section.

Configuration options*
======================

.. note::
   * You do not need to list all configuration options of the sample, but only the ones that are important for the sample and make sense for the user to know about.
   * The syntax below allows sample configuration options to link to the option descriptions in the same way as the library configuration options link to the corresponding Kconfig descriptions (``:kconfig:option:`SAMPLE_CONFIG```, which results in :kconfig:option:`SAMPLE_CONFIG`).
   * For each configuration option, list the symbol name and the string describing it.
   * For the |nRFVSC| instructions, list the configuration options as they are stated on the Generate Configuration screen.

Check and configure the following Kconfig options:

.. _SAMPLE_CONFIG:

SAMPLE_CONFIG
   The sample configuration defines ...

.. _ANOTHER_CONFIG:

ANOTHER_CONFIG
   This configuration option specifies ...

.. note::

   Use :ref:`SAMPLE_CONFIG <SAMPLE_CONFIG>` to link to sample specific option.

Additional configuration*
=========================

.. note::
   * Add this section to describe and link to any library configuration options that might be important to run this sample.
     You can link to options with ``:kconfig:option:`CONFIG_XXX```.
   * You do not need to list all possible configuration options, but only the ones that are relevant.

Check and configure the following library options that are used by the sample:

* :kconfig:option:`CONFIG_BT_DEVICE_NAME` - Defines the Bluetooth® device name.
* :kconfig:option:`CONFIG_BT_LBS` - Enables the :ref:`lbs_readme`.

Configuration files*
====================

.. note::
   Add this section if the sample provides predefined configuration files.

The sample provides predefined configuration files for typical use cases.
You can find the configuration files in the :file:`XXX` directory.

The following files are available:

* :file:`filename.conf` - Specific scenario.
  This configuration file ...

Building and running
********************

.. note::
   * Include the standard text for building - either ``.. include:: /includes/build_and_run.txt`` or ``.. include:: /includes/build_and_run_ns.txt`` for the build targets that use :ref:`Cortex-M Security Extensions <app_boards_spe_nspe>`.
   * The main supported IDE for |NCS| is |VSC|, with the |nRFVSC| installed.
     Therefore, build instructions for the |nRFVSC| are required.
     Build instructions for the command line are optional.
   * See the link to the `nRF Connect for Visual Studio Code`_ documentation site for basic instructions on building with the extension.
   * If the sample uses a non-standard setup, point it out and link to more information, if possible.

.. |sample path| replace:: :file:`samples/XXX`

.. include:: /includes/build_and_run.txt

Note that this sample enables the :ref:`XXX <nrfxlib:nrfxlib>` stack.
See REF for more information.

Some title*
===========

.. note::
   If required, add subsections for additional build instructions.
   Use these subsections sparingly and only if the content does not fit into other sections (mainly Configuration).
   If the additional build instructions are valid for other samples as well, consider adding them to the :ref:`getting_started` section instead and linking to them.


Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Press a button on the development kit.
#. Look at the flashing lights.
#. And so on ...

.. note::
   * Use the shortcuts provided in :file:`doc/nrf/shortcuts.txt` to keep the wording consistent.
   * If there are different ways of testing, introduce them in this section (for example, "After programming the sample to your development kit, you can test it either by ...") and add subsections for the different scenarios.


Sample output*
==============

.. note::
   Add the full output of the sample in this section, or include parts of the output in the testing steps in the previous section.

The following output is logged on RTT::

   whatever


Troubleshooting*
================

If the LEDs do not start blinking, check if the music is loud enough.

References*
***********

* Music chapter in the Bluetooth® Spec (-> always link)
* Disco ball datasheet

.. tip::
   * Do not duplicate links that have been mentioned in other sections before.
   * Do not include links to documents that are common to all or many of our samples.
     For example, the Bluetooth Spec or the DK user guides are always important, but should not be listed.
   * Include specific links, like a chapter in the Bluetooth Spec if the sample demonstrates the respective feature, or a link to the hardware pictures in the DK user guide if there is a lot of wiring required, or specific information about the feature that is presented in the sample.

Dependencies*
*************

.. note::
   * List all relevant libraries.
     Standard libraries (for example, :file:`types.h`, :file:`errno.h`, or :file:`printk.h`) do not need to be listed.
   * Delete the parts that are not relevant.
   * Drivers can be listed under libraries, no need for a separate part.
   * If possible, link to the respective library.
     If there is no documentation for the library, include the path.
   * Add the appropriate secure firmware component that the sample supports.

This sample uses the following |NCS| libraries:

* :ref:`customservice_readme`

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:softdevice_controller`

It uses the following Zephyr libraries:

* ``include/console.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

In addition, it uses the following secure firmware components:

* :ref:`Trusted Firmware-M <ug_tfm>`

The sample also uses drivers from `nrfx`_.

Internal modules*
*****************

.. note::
   Include this section only for applications (not samples), and only if there are internal modules that must be documented.
