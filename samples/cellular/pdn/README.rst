.. _pdn_sample:

.. ncs-sample::
   :title: Cellular: PDN

   The PDN sample demonstrates how to create and configure a Packet Data Protocol (PDP) context, activate a Packet Data Network connection, and receive events on its state and connectivity using the PDN functionality in the :ref:`lte_lc_readme` library.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The sample first initializes the :ref:`nrfxlib:nrf_modem`.
Next, the sample registers a callback for PDN events using the :ref:`lte_lc_readme` library, including events pertaining to the default PDP context.
This is done before changing the function mode to 1 (``AT+CFUN=1``) to receive the activation event for the default PDP context.
The sample then creates a new PDP context and configures it to use the default APN, and activates the PDN connection.
Finally, the sample prints the PDP context IDs and PDN IDs of both the default PDP context and the new PDP context that it has created.
For both PDN connections, it also prints the dynamic parameters reported by the modem: the APN, and the IP address, DNS server addresses, and MTU of each IP family.
Because IPv6 comes up after the PDN connection has been activated, the sample waits up to three seconds for the IPv6 up event before printing the IPv6 parameters.
If IPv6 does not come up within that time, for example, because the network does not support IPv6, the sample prints that IPv6 is not up instead of the IPv6 parameters.

.. note::
   The sample uses the :ref:`lte_lc_readme` library to change the modem's functional mode.
   Hence, the :ref:`lte_lc_readme` library can automatically register to the necessary packet domain events notifications using the ``AT+CGEREP=1`` AT command, and notifications for unsolicited reporting of error codes sent by the network using the ``AT+CNEC=16`` AT command.
   See the `nRF91x1 AT+CGEREP set command`_  and the `nRF91x1 AT+CNEC set command`_ sections, respectively, in the nRF91x1 AT Commands Reference Guide.
   If your application does not use the :ref:`lte_lc_readme` library to change the modem's functional mode, you have to subscribe to these notifications manually before the functional mode is changed.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/pdn`

.. include:: /includes/build_and_run_ns.txt


Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Power on or reset your nRF91 Series DK.
#. Observe that the sample starts, creates and configures a PDP context, and then activates a PDN connection.

Sample output
=============

The sample shows the following output, which may vary based on the network provider:

.. code-block:: console

   *** Booting nRF Connect SDK v3.4.99-0fe0a33dbae1 ***
   *** Using Zephyr OS v4.4.99-1285448db445 ***
   PDN sample started
   Event: PDP context 0 activated
   Default APN is internet
   Created new PDP context 1
   PDP context 1 configured: APN internet, Family IPV4V6
   Event: PDP context 1 activated
   PDP Context 0, PDN ID 0
   PDP Context 1, PDN ID 0
   Event: PDP context 0 IPv6 up
   Event: PDP context 1 IPv6 up
   Dynamic info for cid 0:
   APN: internet.mnc001.mcc001.gprs
   IPv4 address: 10.22.233.44
   Primary IPv4 DNS address: 111.222.233.4
   Secondary IPv4 DNS address: 111.222.233.5
   IPv4 MTU: 1500
   IPv6 address: aaaa::bbbb:cccc
   Primary IPv6 DNS address: 1111:2222:3:fff::55
   Secondary IPv6 DNS address: 1111:2222:3:fff::56
   IPv6 MTU: 1500
   Dynamic info for cid 1:
   APN: internet.mnc001.mcc001.gprs
   IPv4 address: 10.22.233.44
   Primary IPv4 DNS address: 111.222.233.4
   Secondary IPv4 DNS address: 111.222.233.5
   IPv4 MTU: 1500
   IPv6 address: aaaa::bbbb:cccc
   Primary IPv6 DNS address: 1111:2222:3:fff::55
   Secondary IPv6 DNS address: 1111:2222:3:fff::56
   IPv6 MTU: 1500

   Interface addresses:
   l0: (AF_INET) 10.22.233.44
   l0: (AF_INET6) aaaa::bbbb:cccc
   l0: (AF_INET6) aaaa:bbbb:cccc:dddd::eeee:2cd2

   Event: PDP context 0 network detach
   Event: PDP context 1 network detach
   Event: PDP context 1 context destroyed
   Bye

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`at_monitor_readme`
* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
