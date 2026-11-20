.. _provisioning_image_net_core:

.. ncs-sample::
   :title: TF-M: Provisioning image for network core

   Running this provisioning image in the network core will disable the debugging access, as required by the Trusted Firmware-M (TF-M) provisioning process.
   The debugging is disabled by enabling the APPROTECT in the UICR register.
   This sample is included by default as an image of the :ref:`provisioning_image` sample that runs on the application core.

   The provisioning images initialize the provisioning process of a device in a manner compatible with TF-M.
   The APPROTECT feature is explained in detail in :ref:`app_approtect`.

Requirements
************

The following development kits are supported:

.. table-from-sample-yaml::

The firmware runs on the ``nrf5340dk/nrf5340/cpunet`` board target.
The table lists the ``nrf5340dk/nrf5340/cpuapp`` board target used for the standalone build-only CI entry.

Building and running
********************

.. |sample path| replace:: :ncs-file:`/samples/tfm/provisioning_image_net_core`

.. include:: /includes/build_and_run.txt

Testing
=======

The sample does not build firmware for the application core and thus cannot be tested separately.

Test it together with :ref:`provisioning_image` on ``nrf5340dk/nrf5340/cpuapp`` (sysbuild adds this image automatically, see :ncs-file:`/samples/tfm/provisioning_image/Kconfig.sysbuild`).
See :ref:`provisioning_image_testing` for the expected application-core console output and the parent sample's Twister harness.

If you capture the network-core UART, expect output like the following:

.. code-block:: console

   The UICR->APPROTECT register is now configured to deny debugging access for the network core!
   Success!

If UICR is already locked, the network core may instead print the following:

.. code-block:: console

   Cannot write to the UICR->APPROTECT register, exiting...!
