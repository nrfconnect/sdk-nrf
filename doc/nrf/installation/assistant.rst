.. _gs_app_tcm:
.. _gs_assistant:

Installing automatically
########################

.. contents::
   :local:
   :depth: 2

Complete the steps below to install the |NCS| automatically using the Toolchain Manager application.
The application installs the full toolchain for the |NCS|, including the |nRFVSC| and the |NCS| source code.

.. rst-class:: numbered-step

Install prerequisites
*********************

Before you start setting up the toolchain, install available updates for your operating system.
See :ref:`gs_recommended_versions` for information on the supported operating systems and Zephyr features.

Additionally, make sure you install the Universal version of SEGGER J-Link.
This is required for SEGGER J-Link to work correctly with both Intel and ARM assemblies.

.. _tcm_setup:

.. rst-class:: numbered-step

Install Toolchain Manager
*************************

Toolchain Manager is available from `nRF Connect for Desktop`_, a cross-platform tool that provides different applications that simplify installing the |NCS|.
Both the tool and the application are available for Windows, Linux, and macOS.

To install the Toolchain Manager app, complete the following steps:

1. `Download nRF Connect for Desktop`_ for your operating system.
#. Install and run the tool on your machine.
#. In the **APPS** section, click :guilabel:`Install` next to Toolchain Manager.

The app is installed on your machine, and the :guilabel:`Install` button changes to :guilabel:`Open`.

.. _gs_app_installing-ncs-tcm:

.. rst-class:: numbered-step

Install the |NCS|
*****************

Complete the following steps to install the |NCS| source code:

1. Open Toolchain Manager in nRF Connect for Desktop.

   .. figure:: images/gs-assistant_tm.png
      :alt: The Toolchain Manager window

      The Toolchain Manager window

#. Click :guilabel:`SETTINGS` in the navigation bar to specify where you want to install the |NCS|.
#. In :guilabel:`SDK ENVIRONMENTS`, click the :guilabel:`Install` button next to the |NCS| version that you want to install.

   The |NCS| version of your choice is installed on your machine.
   The :guilabel:`Install` button changes to :guilabel:`Open VS Code`.

.. _gs_app_installing_choose_building_method:

.. rst-class:: numbered-step

Set up the preferred building method
************************************

There are two ways you can build an application:

* Using |VSC| and the |nRFVSC|
* Using command line

|vsc_extension_description|

.. _gs_app_installing_choose_building_method_vsc:

Installing |nRFVSC|
===================

To build on the |nRFVSC|, complete the following steps:

#. In Toolchain Manager, click the :guilabel:`Open VS Code` button.

   A notification appears with a list of missing extensions that you need to install, including those from the `nRF Connect for Visual Studio Code`_ extension pack.
#. Click **Install missing extensions**.
#. Once the extensions are installed, click **Open VS Code** button again.

You can then follow the instructions in :ref:`gs_programming_vsc`.

.. _gs_app_installing_choose_building_method_cl:

Installing command line requirements
====================================

To build on the command line, complete the following steps:

1. With admin permissions enabled, download and install the `nRF Command Line Tools`_.
#. Restart the Toolchain Manager application.
#. Click the dropdown menu for the installed nRF Connect SDK version.

   .. figure:: images/gs-assistant_tm_dropdown.png
      :alt: The Toolchain Manager dropdown menu for the installed nRF Connect SDK version, cropped

      The Toolchain Manager dropdown menu options

#. Select :guilabel:`Open command prompt`.

You can then follow the instructions in :ref:`gs_programming_cmd`.
