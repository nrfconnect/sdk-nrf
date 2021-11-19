.. _ug_matter_tools:

Matter tools
############

.. contents::
   :local:
   :depth: 2

Use tools listed on this page to test :ref:`matter_samples` and develop Matter applications in the |NCS|.

GN tool
*******

To build and develop Matter applications, you need the `GN`_ meta-build system.
This system generates the Ninja files that the |NCS| uses.

If you are updating from the |NCS| version earlier than v1.5.0, see the :ref:`GN installation instructions <gs_installing_gn>`.

Matter controller tools
***********************

.. include:: ug_matter_configuring_controller.rst
   :start-after: matter_controller_start
   :end-before: matter_controller_end

See the :ref:`ug_matter_configuring_controller` page for information about how to build and configure the Matter controller.

Thread tools
************

Because Matter is based on the Thread network, you can use the following :ref:`ug_thread_tools` when working with Matter in the |NCS|.

Thread Border Router
====================

.. include:: ug_thread_tools.rst
    :start-after: tbr_shortdesc_start
    :end-before: tbr_shortdesc_end

See the :ref:`ug_thread_tools_tbr` documentation for configuration instructions.

nRF Sniffer for 802.15.4
========================

.. include:: ug_thread_tools.rst
    :start-after: sniffer_shortdesc_start
    :end-before: sniffer_shortdesc_end

nRF Thread Topology Monitor
===========================

.. include:: ug_thread_tools.rst
    :start-after: ttm_shortdesc_start
    :end-before: ttm_shortdesc_end
