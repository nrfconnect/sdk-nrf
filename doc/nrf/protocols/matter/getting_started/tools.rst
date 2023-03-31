.. _ug_matter_tools:
.. _ug_matter_gs_tools:

Matter tools
############

.. contents::
   :local:
   :depth: 2

Use tools listed on this page to test :ref:`matter_samples` and develop Matter applications in the |NCS|.

GN tool
*******

To build and develop Matter applications, you need the `GN`_ meta-build system.
This system generates the Ninja files that the |NCS| uses.

The GN is automatically installed with the |NCS|'s Toolchain Manager when you :ref:`install the SDK automatically <gs_assistant>`.
If you are updating from the |NCS| version earlier than v1.5.0 or you are installing the |NCS| manually, see the :ref:`GN installation instructions <gs_installing_gn>`.

Matter controller tools
***********************

The following figure shows the available Matter controllers in the |NCS|.

.. figure:: ../overview/images/matter_setup_controllers_generic.png
   :width: 600
   :alt: Controllers used by Matter

   Controllers used by Matter

You can read more about the Matter controller on the :ref:`Matter network topologies <ug_matter_configuring_controller>` page.
For information about how to build and configure the Matter controller, see the pages in the :ref:`ug_matter_gs_testing` section.

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
