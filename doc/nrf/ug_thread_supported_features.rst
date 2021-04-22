.. _thread_ug_supported_features:

Supported Thread features
#########################

.. contents::
   :local:
   :depth: 2

The OpenThread implementation of the Thread protocol supports all features defined in the Thread 1.1.1 Specification:

* All Thread networking layers:

  * IPv6
  * 6LoWPAN
  * IEEE 802.15.4 with MAC security
  * Mesh Link Establishment
  * Mesh Routing

* All device roles
* Border Router support

.. _thread_ug_supported_features_v12:

Support for Thread 1.2 Specification
************************************

The |NCS| is gradually implementing features from the Thread 1.2 Specification.

The features introduced with the Thread 1.2 Specification are fully backward-compatible with Thread 1.1 (more specifically, the Thread 1.1.1 Specification).
Thread 1.2 improves network scalability, responsiveness, density, and power consumption.
For more information about this Thread version, see the official `Thread 1.2 in Commercial White Paper`_ and the `Thread 1.2 Base Features`_ document.

In |NCS|, you can choose which version of the Thread protocol to use in your application.
By default, |NCS| supports Thread 1.1, but you can enable and configure Thread 1.2 by using :ref:`dedicated options <thread_ug_thread_1_2>`.

.. note::
    Not all Thread 1.2 functionalities are currently supported.
    See :ref:`thread_ug_thread_1_2` for the list of 1.2 features that are currently available in |NCS|, with information about how to enable them.
    Currently, the :ref:`ot_cli_sample` sample is the only sample that provides an :ref:`ot_cli_sample_thread_v12`.
