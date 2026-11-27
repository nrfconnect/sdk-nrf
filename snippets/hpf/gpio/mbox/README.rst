.. _snippet-hpf-gpio-mbox:

HPF GPIO snippet with MBOX backend (hpf-gpio-mbox)
##################################################

.. contents::
   :local:
   :depth: 2

The ``hpf-gpio-mbox`` snippet configures the :ref:`HPF <hpf_index>` GPIO software peripheral to use the MBOX backend for the communication between the application core and the FLPR core.

.. note::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>`.

Overview
********

The snippet enables the ``nordic,hpf-gpio`` node on the FLPR VPR core and selects the MBOX backend on both sides of the link (:kconfig:option:`CONFIG_GPIO_HPF_GPIO_BACKEND_MBOX` on the application core and :kconfig:option:`CONFIG_HPF_GPIO_BACKEND_MBOX` on the FLPR core).
For each supported SoC, the snippet also provides the devicetree overlays that define the FLPR code partition and the shared memory regions.

Unlike the :ref:`snippet-hpf-gpio-icbmsg` and :ref:`snippet-hpf-gpio-icmsg` snippets, this backend exchanges the GPIO requests directly over the MBOX driver, without the IPC service layer.

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

   west build -S hpf-gpio-mbox [...]
