.. _gs_assistant:

Installing automatically
########################

.. contents::
   :local:
   :depth: 2

`nRF Connect for Desktop`_, a cross-platform tool available for Windows, Linux, and macOS, provides different applications that simplify installing the |NCS|.
Use the :ref:`gs_app_tcm` application to install the |NCS| automatically.

Before you start setting up the toolchain, install available updates for your operating system.
See :ref:`gs_recommended_versions` for information on the supported operating systems and Zephyr features.

.. _gs_app_tcm:

Toolchain Manager
*****************

The Toolchain Manager application is available for Windows, Linux, and macOS.
It installs the full toolchain that you need to work with the |NCS|, including the |nRFVSC| and the |NCS| source code.

.. _tcm_setup:

Installing the Toolchain Manager
================================

To install the Toolchain Manager app, complete the following steps:

1. `Download nRF Connect for Desktop`_ for your operating system.
#. Install and run the tool on your machine.
#. In the **APPS** section, click :guilabel:`Install` next to Toolchain Manager.

The app is installed on your machine, and the :guilabel:`Install` button changes to :guilabel:`Open`.

.. _gs_app_installing-ncs-tcm:

Installing the |NCS|
====================

Once you have installed the Toolchain Manager, open it in nRF Connect for Desktop.

.. figure:: images/gs-assistant_tm.png
   :alt: The Toolchain Manager window

   The Toolchain Manager window

Click :guilabel:`SETTINGS` in the navigation bar to specify where you want to install the |NCS|.
Then, in :guilabel:`SDK ENVIRONMENTS`, click the :guilabel:`Install` button next to the |NCS| version that you want to install.
The |NCS| version of your choice is installed on your machine.

There are two ways you can build an application:

* To build with |VSC|, click on the :guilabel:`Open VS Code` button.

* To build on the command line, use the following steps:

   1. With admin permissions enabled, download and install the `nRF Command Line Tools`_.
   #. Restart the Toolchain Manager application.
   #. Follow the instructions in :ref:`gs_programming_cmd`.

.. figure:: images/gs-assistant_tm_dropdown.png
   :alt: The Toolchain Manager dropdown menu for the installed nRF Connect SDK version, cropped

   The Toolchain Manager dropdown menu options
