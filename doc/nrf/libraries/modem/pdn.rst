.. _pdn_readme:

PDN
###

.. contents::
   :local:
   :depth: 2

The PDN library can be used to manage Packet Data Protocol (PDP) contexts and PDN connections.
It provides an API for the following purposes:

* Creating and configuring PDP contexts
* Receiving events pertaining to the PDN connections
* Managing PDN connections

The library uses several AT commands, and it relies on the following two types of AT notifications to work:

* Packet domain events notifications (``+CGEV``) - Subscribed by using the ``AT+CGEREP=1`` command.
  See the `AT+CGEREP set command`_ section in the nRF9160 AT Commands Reference Guide or the `same section <nRF91x1 AT+CGEREP set command_>`_ in the nRF91x1 AT Commands Reference Guide, depending on the SiP you are using.
* Notifications for unsolicited reporting of error codes sent by the network (``+CNEC``) - Subscribed by using the ``AT+CNEC=16`` command.
  See the `AT+CNEC set command`_ section in the nRF9160 AT Commands Reference Guide or the `same section <nRF91x1 AT+CGEREP set command_>`_ in the nRF91x1 AT Commands Reference Guide, depending on the SiP you are using.

The PDN library automatically subscribes to the necessary AT notifications using :ref:`mlil_callbacks`.
This includes automatically resubscribing to the notifications upon functional mode changes.

.. note::
   The subscription to AT notifications is lost upon changing the modem functional mode to ``+CFUN=0``.
   If the application subscribes to these notifications manually, it must also take care of resubscription.

Following are the AT commands that are used by the library:

* ``AT%XNEWCID`` - To create a PDP context
* ``AT+CGDCONT`` - To configure or destroy a PDP context
* ``AT+CGACT`` - To activate or deactivate a PDN connection
* ``AT%XGETPDNID``- To retrieve the PDN ID for a given PDP context
* ``AT+CGAUTH`` - To set the PDN connection authentication parameters

For more information about these commands, see `Packet Domain AT commands`_ in the nRF9160 AT Commands Reference Guide or the `same section <nRF91x1 packet Domain AT commands_>`_ in the nRF91x1 AT Commands Reference Guide, depending on the SiP you are using.

The application can create PDP contexts by using the :c:func:`pdn_ctx_create` function, and a callback can be assigned to receive the events pertaining to the state and connectivity of the PDP contexts.
The application can use the :c:func:`pdn_default_ctx_cb_reg` function to register an event handler for events pertaining the default PDP context, and the :c:func:`pdn_default_ctx_cb_dereg` to deregister it.
The library stores 12 bytes of memory on the system heap for each PDP context created using :c:func:`pdn_ctx_create` and for each event handler for the default PDP context.
The maximum number of PDP contexts that can be created is limited by the maximum number of PDP context supported by the nRF91 Series modem firmware and the amount of system heap memory available.
The :c:func:`pdn_ctx_configure` function is used to configure a PDP context, which can be configured with a family, access point name, and optional authentication parameters.
The :c:func:`pdn_activate` function activates a PDN connection for a PDP context.
A PDN connection is identified by an ID as reported by ``AT%XGETPDNID``, and it is distinct from the PDP context ID (CID).
The modem creates a PDN connection for a PDP context as necessary.
Multiple PDP contexts might share the same PDN connection if they are configured similarly.

Configuration
*************

The PDN library overrides the default PDP context configuration automatically after the :ref:`nrfxlib:nrf_modem` is initialized, if the :kconfig:option:`CONFIG_PDN_DEFAULTS_OVERRIDE` Kconfig option is set.
The default PDP context configuration consists of the following parameters, each controlled with a Kconfig setting:

* Access point name, :kconfig:option:`CONFIG_PDN_DEFAULT_APN`
* Family, :kconfig:option:`CONFIG_PDN_DEFAULT_FAM`
* Authentication method, :kconfig:option:`CONFIG_PDN_DEFAULT_AUTH`
* Authentication credentials, :kconfig:option:`CONFIG_PDN_DEFAULT_USERNAME` and :kconfig:option:`CONFIG_PDN_DEFAULT_PASSWORD`

.. note::
   The default PDP context configuration must be overridden before the device is registered with the network.

Limitations
***********

You have to set the callback for the default PDP context before the device is registered to the network (``CFUN=1``) to receive the first activation event.

API documentation
*****************

| Header file: :file:`include/modem/pdn.h`
| Source file: :file:`lib/pdn/pdn.c`

.. doxygengroup:: pdn
   :project: nrf
   :members:
