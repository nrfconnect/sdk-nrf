.. _nrf_modem_lib_lte_net_if:

Network interface driver
########################

The LTE network interface in the :ref:`nrfxlib:nrf_modem` is a Zephyr network interface driver for the nRF91 Series cellular firmware.

The driver is enabled using the :kconfig:option:`CONFIG_NRF_MODEM_LIB_NET_IF` Kconfig option.
By enabling the LTE network interface in the :ref:`nrfxlib:nrf_modem`, you can control the nRF91 series modem as a Zephyr network interface, for example, :c:func:`net_if_up`, :c:func:`conn_mgr_if_connect`, and :c:func:`net_if_down`.

.. note::

   The LTE network interface supports both IPv4 and IPv6.
   The support is enabled automatically for whichever IP stack version is enabled in Zephyr.
