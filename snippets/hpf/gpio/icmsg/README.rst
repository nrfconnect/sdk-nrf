.. _snippet-hpf-gpio-icmsg:

HPF GPIO snippet with ICMsg backend (hpf-gpio-icmsg)
####################################################

.. contents::
   :local:
   :depth: 2

The ``hpf-gpio-icmsg`` snippet configures the :ref:`HPF <hpf_index>` GPIO software peripheral to use the ICMsg IPC backend for the communication between the application core and the FLPR core.

.. note::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>`.

Overview
********

The snippet enables the ``nordic,hpf-gpio`` node on the FLPR VPR core and selects the ICMsg backend on both sides of the IPC link (:kconfig:option:`CONFIG_GPIO_HPF_GPIO_BACKEND_ICMSG` on the application core and :kconfig:option:`CONFIG_HPF_GPIO_BACKEND_ICMSG` on the FLPR core).
For each supported SoC, the snippet also provides the devicetree overlays that define the FLPR code partition, the shared memory regions, and the IPC instance.

Compared to the :ref:`snippet-hpf-gpio-icbmsg` snippet, the ICMsg backend provides a single endpoint and uses less memory.

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

   west build -S hpf-gpio-icmsg [...]
