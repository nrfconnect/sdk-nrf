.. _esb_sniffer_scripts_docs:

ESB Sniffer
###########

.. contents::
   :local:
   :depth: 2

The Python scripts introduced in this document are used for the :ref:`esb_monitor` sample configured as ESB Sniffer.

Set up Linux
************

Complete the following steps to set up your Linux environment:

1. Install the Python requirements:

   .. parsed-literal::
         :class: highlight

         pip3 install -r requirements.txt

#. Add a custom dissector and extcap to Wireshark:

   .. parsed-literal::
         :class: highlight

         mkdir -p $HOME/.local/lib/wireshark/{extcap,plugins}
         cp esb_dissector.lua $HOME/.local/lib/wireshark/plugins
         cp extcap/esb_extcap.py $HOME/.local/lib/wireshark/extcap
         chmod +x $HOME/.local/lib/wireshark/extcap/esb_extcap.py

#. Enable the dissector for ESB:


   a. Open Wireshark.
   #. Go to :guilabel:`Edit` -> :guilabel:`Preferences` -> :guilabel:`Protocols` -> :guilabel:`DLT_USER` -> Edit
   #. Click the :guilabel:`Create new entry` icon (bottom left).
   #. Select ``DLT=147`` for **DLT** column and ``esb`` for **Payload dissector** column.
   #. Click :guilabel:`Ok`.
   #. Restart Wireshark.

After completing these steps, a new ESB sniffer interface appears in Wireshark.

Scripts
*******

There are two separate scripts you can use with the DK configured as an ESB sniffer:

main.py
=======

This script provides integration with the Wireshark extcap interface and the UART shell for real-time updates of sniffer parameters such as:

* Bitrate
* Channel
* Radio addresses
* Pipe prefixes
* Enabled pipes

Complete the following steps to use this script:

1. Start the script:

   .. parsed-literal::
         :class: highlight

         python3 main.py

#. Start Wireshark and select the ESB sniffer interface.
#. Observe the packets being received in Wireshark in real time.
#. Type ``q`` or ``quit`` to stop the application.

capture_to_pcap.py
==================

This script is a simple CLI utility to read packets from the DK and save them into pcap formatted file for further analysis.
It is not designed to work with the Wireshark extcap interface.

Complete the following steps to use this script:

1. Start the script with the output filename as an argument:

   .. parsed-literal::
         :class: highlight

         python3 capture_to_pcap.py output.pcap

#. Type ``q`` or ``quit`` to stop the application.
