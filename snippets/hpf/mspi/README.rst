.. _snippet-hpf-mspi:

HPF MSPI snippet (hpf-mspi)
###########################

.. contents::
   :local:
   :depth: 2

The ``hpf-mspi`` snippet enables the :ref:`HPF <hpf_index>` MSPI software peripheral, which implements a multi-bit SPI controller on the FLPR core.

.. note::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>`.

Overview
********

The snippet adds a ``nordic,hpf-mspi-controller`` node to the FLPR VPR core and configures a TIMER peripheral as a watchdog for the HPF firmware.
On the application core, it enables the MSPI driver (:kconfig:option:`CONFIG_MSPI`), the MBOX driver (:kconfig:option:`CONFIG_MBOX`), and the IPC service with the ICMsg backend (:kconfig:option:`CONFIG_IPC_SERVICE` and :kconfig:option:`CONFIG_IPC_SERVICE_BACKEND_ICMSG`).

Supported SoCs
**************

The snippet provides SoC overlays for the application core of the following SoCs:

* nRF54L15
* nRF54LC10A
* nRF54LM20A and nRF54LM20B
* nRF54LV10A

It additionally provides board overlays for the ``nrf54l15dk/nrf54l15/cpuapp``, ``nrf54lm20dk/nrf54lm20a/cpuapp``, and ``nrf54lm20dk/nrf54lm20b/cpuapp`` board targets.

Usage
*****

Apply the snippet when building, for example:

.. code-block:: console

   west build -S hpf-mspi [...]
