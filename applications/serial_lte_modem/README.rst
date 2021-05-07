.. _serial_lte_modem:

nRF9160: Serial LTE modem
#########################

The Serial LTE Modem (SLM) application can be used to emulate a stand-alone LTE modem on the nRF9160.

The SLM application adds proprietary AT commands running on the application processor so they are 
complementary to the AT Commands found in nRF91 `AT Commands Reference Guide`_ which are modem specific.

See the subpages for how to use the application, how to extend it, and information on the supported AT commands.

.. toctree::
   :maxdepth: 2
   :caption: Subpages:

   doc/slm_description
   doc/slm_testing
   doc/slm_extending
   doc/AT_commands_intro
