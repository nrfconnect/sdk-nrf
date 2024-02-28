.. _thread_tcat:

Thread Commissioning Over Authenticated TLS
###########################################

.. contents::
   :local:
   :depth: 2

Thread Commissioning Over Authenticated TLS (TCAT) was developed to address the needs of professional installation and commercial building scenarios.
It makes it easy to wirelessly onboard Thread devices that are pre-installed in difficult-to-reach places, such as inside a ceiling or embedded in a wall.
Instead of scanning physical install codes once the pre-installed devices are powered, Thread commissioning can be implemented over authenticated TLS (Transport Layer Security) by exchanging security certificates.
TCAT can be performed using a mobile device (such as a phone, tablet) while in close proximity to the pre-installed device over a wireless connection such as Bluetooth® Low Energy.
The |NCS| currently provides experimental support for TCAT.
To test this feature, build the :ref:`ot_cli_sample` sample with the :ref:`TCAT snippet <ot_cli_sample_activating_variants>` enabled.

After flashing the sample to the device, use the following command to enable TCAT:

.. code-block:: console

   uart:~$ ot tcat start

Currently, `BBTC Client`_ is the only available TCAT commissioner tool, and can be found in the OpenThread repository.
Refer to the tool's documentation for more information.
