.. _snippet-zperf:

Zperf snippet (zperf)
#####################

.. contents::
   :local:
   :depth: 2

This snippet enables the :ref:`Zperf <zephyr:zperf>` network throughput measurement tool, allowing you to benchmark TCP and UDP performance over the device network connection.
It configures the device to run as a zperf server and exposes the network and zperf shell commands for controlling the measurements.

The snippet applies the following configuration:

* Enables the network shell (:kconfig:option:`CONFIG_NET_SHELL`) to provide the ``net`` and ``zperf`` shell commands.
* Enables the zperf tool (:kconfig:option:`CONFIG_NET_ZPERF`) and the zperf server (:kconfig:option:`CONFIG_NET_ZPERF_SERVER`).
* Enables BSD sockets support (:kconfig:option:`CONFIG_NET_SOCKETS`) and a receive timeout for network contexts (:kconfig:option:`CONFIG_NET_CONTEXT_RCVTIMEO`).
* Increases the maximum number of concurrently open file descriptors (:kconfig:option:`CONFIG_ZVFS_OPEN_MAX`) and the maximum number of file descriptors that can be polled (:kconfig:option:`CONFIG_ZVFS_POLL_MAX`) to accommodate the sockets used during measurements.

Usage
*****

Apply the snippet when building, for example:

.. code-block:: console

   west build -S zperf [...]
