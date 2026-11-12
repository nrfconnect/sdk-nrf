.. _snippet-hpf-gpio-icbmsg:

HPF GPIO snippet with ICBMsg backend (hpf-gpio-icbmsg)
######################################################

.. contents::
   :local:
   :depth: 2

The ``hpf-gpio-icbmsg`` snippet configures the :ref:`HPF <hpf_index>` GPIO software peripheral to use the ICBMsg IPC backend for the communication between the application core and the FLPR core.

.. note::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>`.

Overview
********

The snippet enables the ``nordic,hpf-gpio`` node on the FLPR VPR core and selects the ICBMsg backend on both sides of the IPC link (:kconfig:option:`CONFIG_GPIO_HPF_GPIO_BACKEND_ICBMSG` on the application core and :kconfig:option:`CONFIG_HPF_GPIO_BACKEND_ICBMSG` on the FLPR core).
For each supported SoC, the snippet also provides the devicetree overlays that define the FLPR code partition, the shared memory regions, and the ``zephyr,ipc-icbmsg`` IPC instance.

Compared to the :ref:`snippet-hpf-gpio-icmsg` and :ref:`snippet-hpf-gpio-mbox` snippets, the ICBMsg backend supports multiple endpoints over a single shared memory region.

Supported SoCs
**************

The snippet supports the application core and the FLPR core of the following SoCs:

* nRF54L15
* nRF54LC10A
* nRF54LM20A and nRF54LM20B
* nRF54LV10A
* nRF7120

Usage
*****

Apply the snippet when building, for example:

.. code-block:: console

   west build -S hpf-gpio-icbmsg [...]
