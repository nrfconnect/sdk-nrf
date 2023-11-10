.. _ug_matter_tools:
.. _ug_matter_gs_tools:

Matter tools
############

.. contents::
   :local:
   :depth: 2

Use tools listed on this page to test :ref:`matter_samples` and develop Matter applications in the |NCS|.

.. _ug_matter_gs_tools_gn:
.. _gs_installing_gn:
.. _manual_installation_gn:

GN tool
*******

To build and develop Matter applications, you need the `GN`_ meta-build system.
This system generates the Ninja files that the |NCS| uses.

The GN is automatically installed with the |NCS|'s toolchain when you :ref:`install the |NCS| <install_ncs>`.
If you are updating from the |NCS| version earlier than v1.5.0, see the following GN installation instructions.

.. toggle:: GN installation instructions

   If you want to build Matter applications with the |NCS| version earlier than v1.5.0, install the `GN`_ meta-build system.
   This system generates the Ninja files that the |NCS| uses.

   .. tabs::

      .. group-tab:: Windows

         To install the GN tool, complete the following steps:

         1. Download the latest version of the GN binary archive for Windows from the `GN website`_.
         2. Extract the :file:`zip` archive.
         3. Ensure that the GN tool is added to your :envvar:`PATH` environment variable.
            For the exact instructions, see :ref:`zephyr:env_vars`.

      .. group-tab:: Linux

         To install the GN tool, complete the following steps:

         1. Create the directory for the GN tool:

            .. code-block:: console

               mkdir ${HOME}/gn && cd ${HOME}/gn

         #. Download the GN binary archive and extract it by using the following commands:

            .. code-block:: console

               wget -O gn.zip https:\ //chrome-infra-packages.appspot.com/dl/gn/gn/linux-amd64/+/latest
               unzip gn.zip
               rm gn.zip

            The wget tool is installed when installing the required tools on Linux.
         #. Add the location of the GN tool to the system :envvar:`PATH`.
            For example, if you are using ``bash``, run the following commands:

            .. code-block:: console

               echo 'export PATH=${HOME}/gn:"$PATH"' >> ${HOME}/.bashrc
               source ${HOME}/.bashrc

      .. group-tab:: macOS

         To install the GN tool, complete the following steps:

         1. Create the directory for the GN tool:

            .. code-block:: console

               mkdir ${HOME}/gn && cd ${HOME}/gn

         #. Install the wget tool:

            .. code-block:: console

               brew install wget

         #. Download the GN binary archive and extract it by using the following commands:

            * For 64-bit ARM (M1 and M2) host architecture:

            .. code-block:: console

               wget -O gn.zip https:\ //chrome-infra-packages.appspot.com/dl/gn/gn/mac-arm64/+/latest
               unzip gn.zip
               rm gn.zip

            * For 64-bit AMD (Intel) host architecture:

            .. code-block:: console

               wget -O gn.zip https:\ //chrome-infra-packages.appspot.com/dl/gn/gn/mac-amd64/+/latest
               unzip gn.zip
               rm gn.zip

         #. Add the location of the GN tool to the system :envvar:`PATH`.
            For example, if you are using ``bash``, run the following commands:

            a. Create the :file:`.bash_profile` file if you do not have it already:

               .. code-block:: console

                  touch ${HOME}/.bash_profile

            #. Add the location of the GN tool to :file:`.bash_profile`:

               .. code-block:: console

                  echo 'export PATH=${HOME}/gn:"$PATH"' >> ${HOME}/.bash_profile
                  source ${HOME}/.bash_profile

.. _ug_matter_gs_tools_controller:

Matter controller tools
***********************

The following figure shows the available Matter controllers in the |NCS|.

.. figure:: ../overview/images/matter_setup_controllers_generic.png
   :width: 600
   :alt: Controllers used by Matter

   Controllers used by Matter

You can read more about the Matter controller on the :ref:`Matter network topologies <ug_matter_configuring_controller>` page.
For information about how to build and configure the Matter controller, see the pages in the :ref:`ug_matter_gs_testing` section.

.. _ug_matter_gs_tools_chip:

CHIP Tool for Linux or macOS
============================

The CHIP Tool for Linux or macOS is the default implementation of the Matter controller role, recommended for the nRF Connect platform.
You can read more about it on the :doc:`matter:chip_tool_guide` page in the Matter documentation.

Depending on your system, you can install the CHIP Tool in one of the following ways:

* For Linux only - Use the prebuilt tool package from the `Matter nRF Connect releases`_ GitHub page.
  Make sure that the package is compatible with your |NCS| version.
* For both Linux and macOS - Build it manually from the source files available in the :file:`modules/lib/matter/examples/chip-tool` directory and using the building instructions from the :doc:`matter:chip_tool_guide` page in the Matter documentation.

.. note::
   To use CHIP Tool for macOS you need to have the appropriate Apple profile installed on your macOS machine.
   To learn how to install the profile and work with Apple devices see the `Testing with Apple Devices`_ user guide in the Matter documentation.

.. _ug_matter_gs_tools_zap:

ZAP tool
********

ZCL Advanced Platform, in short ZAP tool, is a third-party tool that is a generic node.js-based templating engine for applications and libraries based on Zigbee Cluster Library.

You can use the ZAP tool for the following Matter use cases:

* Enabling and disabling clusters, cluster commands, attributes, and events.
* Configuring attributes' default values.
* Configuring attributes' properties, such as storage type (RAM storage, non-volatile storage, application-managed).

All the relevant data for these use cases is stored in the ZAP file of your Matter application, which you can edit using the ZAP tool GUI.
A ZAP file is a JSON file that contains the data model configuration of clusters, commands, and attributes that are enabled for a given application.
It is not used directly by the application, but it is used to generate global and customized source files for handling requests enabled by the user.
In the |NCS|, the ZAP file is provided in the :file:`src` directory for each :ref:`Matter sample <matter_samples>`.

For an example of how to use the ZAP tool to edit a ZAP file, see the :ref:`ug_matter_creating_accessory_edit_zap` in the :ref:`ug_matter_creating_accessory` user guide.
For more information about the ZAP tool, see the official `ZCL Advanced Platform`_ documentation.

.. _ug_matter_tools_installing_zap:

Installing the ZAP tool
=======================

To install the ZAP tool using the recommended method, complete the following steps:

1. Download the ZAP package containing pre-compiled executables and libraries and extract it:

   a. Open your installation directory for the |NCS| in a command line.
   #. Navigate to :file:`modules/lib/matter`.
   #. Run the helper script to download and extract the ZAP package:

      .. parsed-literal::
         :class: highlight

         python scripts/setup/nrfconnect/get_zap.py -l *zap_location* -o

      In this command:

      *  *zap_location* corresponds to the directory where the ZAP tool package is to be extracted.
      *  The `-o` argument in the command is used to allow overwriting files, if they already exist in the given location.
         Otherwise the script will display prompt during download and ask for user consent to overwrite the files directly.

#. Verify the ZAP tool version by comparing the script output with the following possible cases:

   * Case 1: If your currently installed ZAP version matches the recommended one, you will see a message similar to the following one:

     .. code-block::

        Your currently installed ZAP tool version: 2022.12.20 matches the recommended one.

     This means that your ZAP version is correct and the tool executable can be accessed from the operating system environment, so you can skip the following step about adding the ZAP tool to the system :envvar:`PATH` environment variable.

   * Case 2: If your currently installed ZAP version does not match the recommended one or no ZAP version is installed on your device, you will see a message similar to the following one:

     .. code-block::

        Your currently installed ZAP tool version 2022.12.19 does not match the recommended one: 2022.12.20.

     Alternatively, this message can look like the following one:

     .. code-block::

        No ZAP tool version was found installed on this device.

     In this case, the package download process will start automatically:

     .. parsed-literal::
        :class: highlight

        Trying to download ZAP tool package matching your system and recommended version.
        100% [......................................................................] 150136551 / 150136551
        ZAP tool package was downloaded and extracted in the given location.

     Depending on your operating system, the download will conclude with the following message, with updated *zap_cli_location* and *zap_location* paths, which you will need in the following step:

     .. tabs::

        .. group-tab:: Windows

           .. parsed-literal::
              :class: highlight

              #######################################################################################
              # Please add the following location(s) to the system PATH:                            #
              # *zap_location*                                                                      #
              #######################################################################################

        .. group-tab:: Linux

           .. parsed-literal::
              :class: highlight

              #######################################################################################
              # Please add the following location(s) to the system PATH:                            #
              # *zap_location*                                                                      #
              ######################################################################################

        .. group-tab:: macOS

           .. parsed-literal::
              :class: highlight

              #######################################################################################
              # Please add the following location(s) to the system PATH:                            #
              # *zap_cli_location*                                                                  #
              # *zap_location*                                                                      #
              #######################################################################################
     ..

#. Add the ZAP packages location to the system :envvar:`PATH` environment variables.
   This is not needed if your currently installed ZAP version matches the recommended one (case 1 from the previous step).

   .. tabs::

      .. group-tab:: Windows

         For the detailed instructions for adding :envvar:`PATH` environment variables on Windows, see :ref:`zephyr:env_vars`.

      .. group-tab:: Linux

         For example, if you are using bash, run the following commands, with *zap_location* updated with your path:

         .. parsed-literal::
            :class: highlight

            echo 'export PATH=zap_location:"$PATH"' >> ${HOME}/.bashrc
            source ${HOME}/.bashrc

      .. group-tab:: macOS

         For example, if you are using bash, run the following commands, with *zap_cli_location* and *zap_location* updated with your paths:

         .. parsed-literal::
            :class: highlight

            echo 'export PATH=zap_cli_location:"$PATH"' >> ${HOME}/.bash_profile
            echo 'export PATH=zap_location:"$PATH"' >> ${HOME}/.bash_profile
            source ${HOME}/.bash_profile
   ..

Alternative installation methods
================================

You can also install the ZAP tool using one of the following methods:

* Download the ZAP tool package in a compatible version manually from the Assets section in `ZCL Advanced Platform releases`_.
* Configure the tool and manually compile it using the instructions in the official `ZCL Advanced Platform`_ documentation.

Both these methods still require adding the ZAP tool location to the system :envvar:`PATH` environment variables, as detailed in the step 3 above.

.. _ug_matter_gs_tools_cert:

CHIP Certificate Tool
*********************

Matter's CHIP Certificate Tool, in short chip-cert, is a command-line utility tool for generating and editing Matter certificates such as Certificate Declarations (CD), Device Attestation Certificates (DAC), Product Attestation Intermediate (PAI) certificates, and Product Attestation Authority (PAA) certificate, alongside their related keys.
You can use it for integration testing purposes while working on a :ref:`Matter end product <ug_matter_intro_device>`.

For more information about the chip-cert tool, see how to :ref:`generate custom certification declarations <ug_matter_device_configuring_cd_generating_steps>` for integration testing in the |NCS|.
You can also take a look at the `CHIP Certificate Tool source files`_.

.. _ug_matter_gs_tools_cert_installation:

Installing CHIP Certificate Tool
================================

To install the chip-cert tool, complete the following steps:

1. Navigate to the :file:`connectedhomeip` root directory.
#. In a terminal, run the following command to build the tool executable file:

   .. code-block:: console

      cd src/tools/chip-cert && gn gen out && ninja -C out chip-cert

#. Add the chip-cert tool to the system :envvar:`PATH` environment variable when built.

Generating custom certificates in factory data
==============================================

Adding the chip-cert tool to the system :envvar:`PATH` allows you to build :ref:`matter_samples`, the :ref:`Matter weather station <matter_weather_station_app>`, and the :ref:`Matter bridge <matter_bridge_app>` applications with custom certificates included in the factory data.
This lets you for example change the test Vendor ID, Product ID, or other data.

To build a Matter application in the |NCS| with custom certification data, make sure to set the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_USE_DEFAULT_CERTS` to ``n`` when :doc:`building an example with factory data <matter:nrfconnect_factory_data_configuration>`.

.. _ug_matter_gs_tools_spake2:

SPAKE2+ Python tool
*******************

SPAKE2+ Python Tool is a Python script for generating SPAKE2+ protocol parameters.
The protocol is used during Matter commissioning to :ref:`establish a secure session <ug_matter_overview_commissioning_stages_case>` between the commissioner and the commissionee.

.. note::
   Currently, the tool only supports generating Verifier parameters.

For usage examples, see the `SPAKE2+ Python Tool page`_ in the Matter SDK official documentation.

.. _ug_matter_gs_tools_mot:

Matter over Thread tools
************************

You can use the following :ref:`ug_thread_tools` when working with Matter in the |NCS| using the Matter over Thread setup.

Thread Border Router
====================

.. include:: ../../thread/tools.rst
    :start-after: tbr_shortdesc_start
    :end-before: tbr_shortdesc_end

See the :ref:`ug_thread_tools_tbr` documentation for configuration instructions.

nRF Sniffer for 802.15.4
========================

.. include:: ../../thread/tools.rst
    :start-after: sniffer_shortdesc_start
    :end-before: sniffer_shortdesc_end

nRF Thread Topology Monitor
===========================

.. include:: ../../thread/tools.rst
    :start-after: ttm_shortdesc_start
    :end-before: ttm_shortdesc_end
