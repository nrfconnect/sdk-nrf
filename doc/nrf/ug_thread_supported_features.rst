.. _thread_ug_supported_features:

Supported Thread features
#########################

.. contents::
   :local:
   :depth: 2

The OpenThread implementation of the Thread protocol supports all features defined in the Thread v1.1.1 specification.
This includes:

* All Thread networking layers:

  * IPv6
  * 6LoWPAN
  * IEEE 802.15.4 with MAC security
  * Mesh Link Establishment
  * Mesh Routing

* All device roles
* Border Router support

.. _thread_ug_supported_features_v12:

Support for Thread Specification v1.2
*************************************

The |NCS| is gradually implementing features from Thread Specification v1.2.

The features introduced with Thread Specification v1.2 are fully backward-compatible with the Thread v1.1 (more specifically, v1.1.1).
The Thread v1.2 improves network scalability, responsiveness, density, and power consumption.
For more information about this version, see the official `Thread Specification v1.2 Commercial White Paper`_ and the `Thread Specification v1.2 Base Features`_ documentation.

In |NCS|, you can choose which version of Thread protocol will be used in your application.
By default, |NCS| supports Thread Specification v1.1, but you can enable and configure Thread Specification v1.2 by using :ref:`dedicated options <thread_ug_thread_1_2>`.

.. note::
    Not all v1.2 functionalities are currently supported.
    See the dedicated options link for the list of v1.2 features currently available in |NCS|, with information about how to enable them.
    Currently, the :ref:`Thread CLI sample <ot_cli_sample>` is the only sample to offer :ref:`experimental v1.2 extension <ot_cli_sample_thread_v12>`.
