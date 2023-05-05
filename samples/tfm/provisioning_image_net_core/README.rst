.. _provisioning_image_net_core:

TF-M: Provisioning image for network core
#########################################

.. contents::
   :local:
   :depth: 2


Running this provisioning image in the network core will disable the debugging access, as required by the Trusted Firmware-M (TF-M) provisioning process.
The debugging is disabled by enabling the APPROTECT in the UICR register.
This sample is included by default as a child image of the :ref:`provisioning image<provisioning_image>` sample which runs on the application core.

The provisioning images initialize the provisioning process of a device in a manner compatible with TF-M.
The APPROTECT feature is explained in detail in :ref:`app_approtect`.

Requirements
************

The following development kits are supported:

.. table-from-sample-yaml::

Building and running
********************

.. |sample path| replace:: :file:`samples/tfm/provisioning_image_net_core`

.. include:: /includes/build_and_run.txt

Testing
=======
The sample does not build firmware for the application core and thus cannot be tested separately.
