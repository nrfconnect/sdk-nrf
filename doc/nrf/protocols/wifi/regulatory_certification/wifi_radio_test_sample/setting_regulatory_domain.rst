.. _ug_wifi_setting_regulatory_domain:

Setting regulatory domain
#########################

.. contents::
   :local:
   :depth: 2

In the Wi-Fi® Radio test sample, world regulatory domain regulations apply by default.
To change the regulatory domain to a specific country, use the ``reg_domain`` subcommand.

.. note::
   Set the ``reg_domain`` subcommand for each session and on reboot of the development board, not with every command set described in this section.

For more information, see :ref:`nRF70 Series operating with regulatory support <ug_nrf70_developing_regulatory_support>`.

To change the domain, use the following command:

.. code-block:: shell

   uart:~$ wifi_radio_test reg_domain <country code>

To remove regulatory domain restrictions, use the following command:

.. code-block:: shell

   uart:~$ wifi_radio_test bypass_reg_domain 1

To re-enable regulatory domain restrictions, use the following command:

.. code-block:: shell

   uart:~$ wifi_radio_test bypass_reg_domain 0

For more information, see :ref:`ug_wifi_radio_test_sample` and :ref:`Using the Radio test (short-range) sample <ug_using_short_range_sample>`.
