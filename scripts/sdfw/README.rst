.. _sdfw_ctrlap_script:

Secure Domain Firmware (SDFW) scripts
#####################################

.. contents::
   :local:
   :depth: 2

These scripts enables communication with the SDFW using the CTRL-AP.

Overview
********

Several types of requests can be issued to the SDFW over the CTRL-AP interface.
These requests are based on the ADAC protocol.

There are two types of requests: ADAC commands and SSF services.

ADAC commands
-------------

ADAC commands are invoked through :file:`scripts/sdfw/main.py`.
These are vendor specific extensions to the ADAC protocol.

SSF Services
------------
SSF services are invoked by calling their respective script file directly.
For instance :file:`sdfw_update_service.py`.

Requirements
************

The script source files are located in the :file:`scripts/sdfw` directory.
There are no extra dependencies in addition to the ones in sdk-nrf.
These scripts also requires an the nRF54H Series development kit to run the SDFW.

Using the script
****************

To list all supported ADAC commands run the following command:

.. code-block:: console

   python3 main.py --help

For detailed information about a specific command (for instance ``version``) run the following command:

.. code-block:: console

   python3 main.py version --help


Reading the SDFW version with an ADAC command
---------------------------------------------

.. code-block:: console

   python3 main.py version --type 1 # Read the Firmware version
   python3 main.py version --type 0 # Read the ADAC protocol version

Purging a domain
----------------

.. code-block:: console

   python3 main.py purge --domain-id 2 # Purge the application domain
   python3 main.py purge --domain-id 3 # Purge the radio domain
