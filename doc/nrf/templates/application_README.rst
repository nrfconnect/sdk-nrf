.. _application:

Template: Application
#####################

.. contents::
   :local:
   :depth: 2

.. note::
   * Provide a concise name to the application, which corresponds to the folder name.
     If the application targets a specific device, add it in the title before the application name (for example, "nRF9160:").
     Do not include the word "application" in the title.
   * Place the documentation inside the :file:`applications` folder and use the file name :file:`README.rst`.
   * Use the provided stock phrases and includes when possible.
   * Sections with * are optional and can be left out.
     All other sections are required.
     Do not add new sections (unless in the sections that allow for further subsections) without discussion with the tech writer team.

The XYZ application demonstrates some functionality for the Nordic XX hardware.

.. tip::
   Explain what this application demonstrates in one or two sentences maximum (full sentences, not sentence fragments).
   This introduction must be concise and clear, highlighting the features of the application.

Application overview
********************

You can use this application as a USB composite device.

The application adds the functionality of a USB mass storage device, which contains several utility files such as a ``:ref:RST link`` file.
In addition, it uses the ``:ref:RST link`` to provide an option for adding some functionality.

.. tip::
   Continue the explanation on what this application is about.
   What does it show, and why should users try it?
   What is the practical use?
   What libraries are used? (provide links to the used libraries)

.. note::
   If the application uses a third-party integration, provide an explanation of how the integration works with the application. Provide a link to the integration user guide for the user to get more information.

Some title*
===========
.. note::
   Add optional subsections for technical details.
   Provide suitable titles (sentence style capitalization, thus only the first word capitalized).
   If there is nothing important to point out, do not include any subsections.

Requirements
************

.. note::
   * Supported kits are listed in a table, which is composed of rows from the :file:`doc/nrf/includes/sample_board_rows.txt` file.
     Select the required rows in the ``:rows:`` configuration, or use the ``.. table-from-sample-yaml::`` directive to include all build targets specified in the :file:`sample.yaml` file.
   * If only one kit is supported, replace the introduction text with "The application supports the following development kit:".
   * If several kits are required to test the application, state it after the table (for example, "You can use one or more of the development kits listed above and mix different development kits.").
   * Mention additional requirements after the table.
   * If TFM is included in the application, add ``.. include:: /includes/tfm.txt`` to include the standard text that states this.

The application supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf52dk_nrf52832, nrf52840dk_nrf52840

The application also requires ...

User interface*
***************
.. note::
   Add button and LED assignments here, with other information related to the user interface (for example, supported commands).

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
   Always include this section if the user can configure the application.
   Start with the stock phrase that is included with ``|config|``.

|config|

Setup*
======

.. note::
   Add information about the initial setup here, for example, that the user must install or enable some library before they can compile this application, or set up and select a specific backend.
   Most applications do not need this section.

Some title*
===========

.. note::
   If required, add subsections for additional configuration scenarios that require a different building procedure.

Configuration options*
======================

.. note::
   * List only important configuration options of the application in this section.
     Make sure all other configuration options are listed in the section at the bottom of the page.
   * The syntax below allows application configuration options to link to the option descriptions in the same way as the library configuration options link to the corresponding Kconfig descriptions (``:kconfig:option:`CONFIG_APPLICATION```, which results in :kconfig:option:`CONFIG_APPLICATION`).
   * For each configuration option, list the symbol name and the string describing it.
   * For the |nRFVSC| instructions, list the configuration options as they are stated on the **Generate Configuration** screen.

Check and configure the following Kconfig options:

.. _CONFIG_APPLICATION:

CONFIG_APPLICATION
   The application configuration specifies ...

Additional configuration*
=========================

.. note::
   * Add this section to describe and link to any library configuration options that might be important to run this application.
     You can link to options with ``:kconfig:option:`CONFIG_XXX```.
   * You need not list all possible configuration options, but only the ones that are relevant.

Check and configure the following library options that are used by the application:

* :kconfig:option:`CONFIG_PDN_DEFAULT_APN` - Used for manual configuration of the Access Point Name (APN).
* :kconfig:option:`CONFIG_MODEM_ANTENNA_GNSS_EXTERNAL` - Selects an external GNSS antenna.

Configuration files*
====================

.. note::
   Add this section if the application provides predefined configuration files.

The application provides predefined configuration files for typical use cases.
You can find the configuration files in the :file:`XXX` directory.

The following files are available:

* :file:`filename.conf` - Specific scenario.
  This configuration file ...

Building and running
********************

.. note::
   * Include the standard text for building - either ``.. include:: /includes/application_build_and_run.txt`` or ``.. include:: /includes/application_build_and_run_ns.txt`` for the build targets that use :ref:`Cortex-M Security Extensions <app_boards_spe_nspe>`.
   * The main supported IDE for |NCS| is |VSC|, with the |nRFVSC| installed.
     Therefore, build instructions for the |nRFVSC| are required.
     Build instructions for the command line are optional.
   * See the link to the `nRF Connect for Visual Studio Code`_ documentation site for basic instructions on building with the extension.
   * If the application uses a non-standard setup, point it out and link to more information, if possible.

.. |application path| replace:: :file:`applications/XXX`

.. include:: /includes/application_build_and_run.txt

Some title*
===========

.. note::
   If required, add subsections for additional build instructions.
   Use these subsections sparingly and only if the content does not fit into other sections (mainly `Configuration*`_).
   If the additional build instructions are valid for other applications as well, consider adding them to the :ref:`getting_started` section instead and linking to them.

Testing
=======

|test_application|

#. |connect_kit|
#. |connect_terminal|
#. Reset the development kit.
#. And so on ...

.. note::
   * Use the shortcuts provided in :file:`doc/nrf/shortcuts.txt` to keep the wording consistent.
   * If there are different ways of testing, introduce them in this section (for example, "After programming the application to your development kit, you can test it either by ...") and add subsections for the different scenarios.

Application output*
===================

.. note::
   Add the full output of the application in this section or include parts of the output in the testing steps in the previous section.

The application shows the following output:

.. code-block:: console

   [00:00:02.029,174] <inf> zigbee_app_utils: Zigbee stack initialized

References*
***********

.. note::
   Provide a link to other relevant documentation for the user to get more information.

.. tip::
   Do not include links to documents that are common to all or many of our applications. For example, :ref:`getting_started` section.

Related projects and applications*
==================================

.. note::
   Add links to projects and/or applications that demonstrate or implement some or all of the features of this application.
   For example, the :ref:`matter_weather_station_app` application is part of the `Matter`_ project, and so on.

Related samples*
================
.. note::
   Add links to the samples that show aspects of the application.
   A sample showcases one feature or component.
   An application uses several features or components.
   Include links to samples that showcase features included in the application.

Dependencies*
*************

.. note::
   * List all relevant dependencies, for example, libraries, other tool or service (third-party) references, certification requirements (if applicable).
   * Standard libraries (for example, :file:`types.h`, :file:`errno.h`, or :file:`printk.h`) need not be listed.
   * Delete the parts that are not relevant.
   * Drivers can be listed under libraries.
   * If possible, link to the respective dependency.
     If there is no documentation for the dependency, include the path.
   * Add the appropriate secure firmware component that the application supports.

This application uses the following |NCS| libraries:

* :ref:`app_event_manager`
* :ref:`lib_aws_iot`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr libraries:

* :ref:`zephyr:logging_api`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`

The application also uses drivers from `nrfx`_.

Internal modules*
*****************

.. note::
   Add this section if there are internal modules that must be documented.
   If there are complex modules that cannot fit on one page, add them on separate pages.

API documentation*
******************

.. note::
   Add the following section if the application uses API documentation.
   Add subsections if the application uses different components with their own APIs.

.. code-block::

   | Header file: :file:`*provide_the_path*`
   | Source files: :file:`*provide_the_path*`

   .. doxygengroup:: *doxygen_group_name*
      :project: *project_name*
      :members:
