.. _tfm_psa_template_net_core:

TF-M: PSA template for network core
###################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates the update of the network core firmware as part of the :ref:`tfm_psa_template` sample.
:ref:`tfm_psa_template` sample runs in the application core and is responsible for updating this sample in the network core.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Building and running
********************

.. |sample path| replace:: :file:`samples/tfm/tfm_psa_template`

.. include:: /includes/build_and_run.txt

Testing
=======

The sample does not build firmware for the application core and because of that the sample cannot be tested separately.
