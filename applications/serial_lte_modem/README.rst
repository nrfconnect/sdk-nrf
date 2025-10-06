.. _serial_lte_modem:

Serial LTE modem
################

.. note::

   The Serial LTE Modem application will be removed before the next |NCS| release.
   It will be replaced by the Serial Modem, which is an add-on released separately from the `serial-modem <ncs-serial-modem_>`_ repository.
   The Serial LTE Modem application has been copied as the base for the add-on repository.

The Serial LTE Modem (SLM) application can be used to emulate a stand-alone LTE modem on an nRF91 Series device.
The application accepts both the modem-specific AT commands and proprietary AT commands.
The AT commands are documented in the following guides:

* Modem-specific AT commands - `nRF91x1 AT Commands Reference Guide`_  and `nRF9160 AT Commands Reference Guide`_
* Proprietary AT commands - :ref:`SLM_AT_commands`

See the subpages for how to use the application, how to extend it, and information on the supported AT commands.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   doc/slm_description
   doc/nRF91_as_Zephyr_modem
   doc/PPP_linux
   doc/slm_testing
   doc/slm_extending
   doc/slm_data_mode
   doc/AT_commands
