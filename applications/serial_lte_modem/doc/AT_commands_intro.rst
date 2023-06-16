.. _SLM_AT_intro:

SLM-specific AT commands
########################

The application sample uses a series of proprietary AT commands to let the nRF91 development kit operate as a Serial LTE Modem (SLM).

The AT Commands have standardized syntax rules.

Words enclosed in <angle brackets> are references to syntactical elements.
Words enclosed in [square brackets] represent optional items that can be left out of the command line at the specified point.
The brackets are not used when the words appear in the command line.

``<CR>``, ``<LF>`` and ``<CR><LF>`` are allowed in an AT command sent by an application.

A string type parameter input must be enclosed between quotation marks (``"string"``).

There are 3 types of AT commands:

* Set command ``<CMD>[=...]``.
  Set commands set values or perform actions.
* Read command ``<CMD>?``.
  Read commands check the current values of subparameters.
* Test command ``<CMD>=?``.
  Test commands test the existence of the command and provide information about the type of its subparameters.
  Some test commands can also have other functionality.

AT responds to all commands with a final response.

See the following subpages for documentation of the proprietary AT commands.
The modem-specific AT commands are documented in the `nRF91 AT Commands Reference Guide <AT Commands Reference Guide_>`_.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   Generic_AT_commands
   SOCKET_AT_commands
   TCPIP_AT_commands
   ICMP_AT_commands
   FOTA_AT_commands
   SMS_AT_commands
   FTP_AT_commands
   GNSS_AT_commands
   MQTT_AT_commands
   HTTPC_AT_commands
   TWI_AT_commands
   GPIO_AT_commands
   CARRIER_AT_commands
   NRFCLOUD_AT_commands
