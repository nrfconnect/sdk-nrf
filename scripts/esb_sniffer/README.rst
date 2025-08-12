.. _esb_sniffer_scripts:

|ESB| Sniffer
#############

.. contents::
   :local:
   :depth: 2

The Python scripts introduced in this document are used for the :ref:`esb_monitor` sample configured as |ESB| Sniffer.

Overview
********

There are two separate scripts you can use with the DK configured as an |ESB| sniffer:

* ``main.py`` provides integration with the Wireshark extcap interface and the UART shell for real-time updates of sniffer parameters such as:

   * Bitrate
   * Channel
   * Radio addresses
   * Pipe prefixes
   * Enabled pipes

* ``capture_to_pcap.py`` is a simple CLI utility to read packets from the DK and save them into pcap formatted file for further analysis.

Requirements
************

The script source files are located in the :file:`scripts/esb_sniffer` directory.
Complete the following steps to install scripts requirements:

1. Install the Python requirements:

   .. code-block:: console

         pip3 install -r nrf/scripts/esb_sniffer/requirements.txt

#. Install `Wireshark`_.

Set up Wireshark
****************

Complete the following steps to set up Wireshark:

1. Enter `nrf/scripts/esb_sniffer` directory.

#. Add a custom plugins to Wireshark:

   .. tabs::

      .. group-tab:: Linux

         .. code-block:: console

            mkdir -p $HOME/.local/lib/wireshark/{extcap,plugins}
            cp esb_dissector.lua $HOME/.local/lib/wireshark/plugins
            cp extcap/esb_extcap.py $HOME/.local/lib/wireshark/extcap

      .. group-tab:: Windows

         Copy the :file:`esb_dissector.lua` file into the :file:`%APPDATA%\\Wireshark\\plugins` directory.

#. Enable the dissector for |ESB|:

   a. Open Wireshark.
   #. Go to :guilabel:`Edit` -> :guilabel:`Preferences` -> :guilabel:`Protocols` -> :guilabel:`DLT_USER` -> :guilabel:`Edit`.
   #. Click the :guilabel:`Create new entry` icon (bottom left).
   #. Select ``DLT=147`` for **DLT** column and ``esb`` for **Payload dissector** column.
   #. Click :guilabel:`Ok`.
   #. Restart Wireshark.

After completing these steps, a new |ESB| sniffer interface appears in Wireshark.

main.py
*******

This script works on Linux only.

Complete the following steps to use this script:

1. Start the script:

   .. code-block:: console

      python3 main.py

#. Start Wireshark and select the |ESB| sniffer interface.
#. Observe the packets being received in Wireshark in real time.
#. Type ``q`` or ``quit`` to stop the application.

capture_to_pcap.py
******************

This script is not designed to work with a live Wireshark capture.
You can capture packets into a file and open it in Wireshark later.

Complete the following steps to use this script:

1. Start the script with the output filename as an argument:

   .. code-block:: console

      python3 capture_to_pcap.py output.pcap

#. Type ``q`` or ``quit`` to stop the application.

Dependencies
************

The scripts use the ``pynrfjprog`` and ``pyserial`` libraries to communicate with the DK, and `Wireshark`_ as tool for visualizing |ESB| packets.
