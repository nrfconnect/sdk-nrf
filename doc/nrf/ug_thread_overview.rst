.. _ug_thread_intro:

OpenThread overview
###################

The |NCS|'s implementation is based on the OpenThread stack, which is integrated into Zephyr.
The integration with the stack and the radio driver is ensured by Zephyr's L2 layer, which acts as intermediary with Thread on the |NCS| side.

OpenThread is a portable and flexible open-source implementation of the Thread networking protocol, created by Nest in active collaboration with Nordic to accelerate the development of products for the connected home.

Among others, OpenThread has the following main advantages:

* A narrow platform abstraction layer that makes the OpenThread platform-agnostic.
* Small memory footprint.
* Support for system-on-chip (SoC), network co-processor (NCP) and radio co-processor (RCP) designs.
* Official Thread certification.

For more information about some aspects of Thread, see the following pages:

.. toctree::
   :maxdepth: 1

   ug_thread_architectures.rst
   ug_thread_ot_integration.rst
   ug_thread_commissioning.rst

You can also find more at `OpenThread.io`_ and `Thread Group`_ pages.

.. _thread_ug_supported_features:

Supported features
******************

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
=====================================

The |NCS| is gradually implementing features from Thread Specification v1.2.

The features introduced with Thread Specification v1.2 are fully backward-compatible with the Thread v1.1 (more specifically, v1.1.1).
The Thread v1.2 improves network scalability, responsiveness, density, and power consumption.
For more information about this version, see the official `Thread Specification v1.2 Commercial White Paper`_ and the `Thread Specification v1.2 Base Features`_ documentation.

In |NCS|, you can choose which version of Thread protocol will be used in your application.
By default, |NCS| supports Thread Specification v1.1, but you can enable and configure Thread Specification v1.2 by using :ref:`dedicated options <thread_ug_thread_1_2>`.

.. note::
    Not all v1.2 functionalities are currently supported.
    See the dedicated options link for the list of v1.2 features currently available in |NCS|, with information about how to enable them.
