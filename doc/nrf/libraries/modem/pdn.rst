.. _pdn_readme:

PDN
###

.. contents::
   :local:
   :depth: 2

The PDN library can be used to manage Packet Data Protocol (PDP) contexts and PDN connections.
It provides an API for the following purposes:

* Creating and configuring PDP contexts
* Receiving events pertaining to the PDP state and connectivity
* Managing PDN connections

The library uses several AT commands, and it relies on the following two types of AT notifications to work:

* Packet domain events notifications (``+CGEV``) - Subscribed by using the `AT+CGEREP set command`_ (``AT+CGEREP=1``)
* Notifications for unsolicited reporting of error codes sent by the network (``+CNEC``) - Subscribed by using the `AT+CNEC set command`_ (``AT+CNEC=16``)

.. note::
   The application must subscribe to the AT notifications for packet domain events and unsolicited reporting of error codes for the functioning of the library.

Following are the AT commands that are used by the library:

* ``AT%XNEWCID`` - To create a PDP context
* ``AT+CGDCONT`` - To configure or destroy a PDP context
* ``AT+CGACT`` - To activate or deactivate a PDN connection
* ``AT%XGETPDNID``- To retrieve the PDN ID for a given PDP context
* ``AT+CGAUTH`` - To set the PDN connection authentication parameters

For more information, see `Packet Domain AT commands`_.

The application can create PDP contexts by using the :c:func:`pdn_ctx_create` function, and a callback can be assigned to receive the events pertaining to the state and connectivity of the PDP contexts.
The :c:func:`pdn_ctx_configure` function is used to configure a PDP Context, which can be configured with a family, access point name, and optional authentication parameters.
The :c:func:`pdn_activate` function activates a PDN connection for a PDP context.
A PDN connection is identified by an ID as reported by ``AT%XGETPDNID``, and it is distinct from the PDP context ID (CID).
The modem creates a PDN connection for a PDP context as necessary.
Multiple PDP contexts might share the same PDN connection if they are configured similarly.

The library requires the following Kconfig options to be set to ``y``:

* :kconfig:`CONFIG_AT_CMD`
* :kconfig:`CONFIG_AT_NOTIF`

Configuration
*************

The default PDP context configuration consists of the following parameters:

* Access point name
* Family
* Authentication method
* Authentication credentials

The PDN library can override the default PDP context configuration by using the :kconfig:`CONFIG_PDN_DEFAULTS_OVERRIDE` Kconfig option.

Limitations
***********

The callback for the default PDP context must be set before the device is put to function mode 1 (CFUN=1) to receive the first activation event.

API documentation
*****************

| Header file: :file:`include/modem/pdn.h`
| Source file: :file:`lib/pdn/pdn.c`

.. doxygengroup:: pdn
   :project: nrf
   :members:
