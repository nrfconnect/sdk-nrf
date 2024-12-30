.. _caf_net_state:

CAF: Network state module
#########################

.. contents::
   :local:
   :depth: 2

The |net_state| of the :ref:`lib_caf` (CAF) is responsible for broadcasting the events about current network state.
The module provides different backends for available networks (like LTE or OpenThread).

Configuration
*************

To enable the |net_state|, set the :kconfig:option:`CONFIG_CAF_NET_STATE` Kconfig option in the configuration.
The module selects the backend based on the link layer enabled.

Log information about connection waiting
========================================

The :kconfig:option:`CONFIG_CAF_LOG_NET_STATE_WAITING` Kconfig option enables periodic message logging, while the module is waiting for network connection.
To configure the period between logs, use the :kconfig:option:`CONFIG_CAF_LOG_NET_STATE_WAITING_PERIOD` option.

Implementation details
**********************

The module generates the :c:struct:`net_state_event`.
The event reports the current network state using :c:enum:`net_state`.
The following network states are available:

* :c:enumerator:`NET_STATE_DISABLED` - Network interface is disabled.
* :c:enumerator:`NET_STATE_DISCONNECTED` - Network interface is ready to connect but waiting for connection.
* :c:enumerator:`NET_STATE_CONNECTED` - Network interface is connected.

:c:enumerator:`NET_STATE_CONNECTED` means that IP packets can be transmitted.
For example, in a Thread network, this means not only that the connection to the mesh network is established, but also that the Border Router is working and it is possible to transfer data.
