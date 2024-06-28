.. _ug_nrf54h20_suit_intro:

Introduction to SUIT
####################

.. contents::
   :local:
   :depth: 2

This documentives an overview of SUIT and its characteristics.
See the :ref:`nrf54h_suit_sample` if you want to try using the SUIT procedure on the nRF54H20 SoC.

.. _ug_suit_overview:

SUIT overview
*************

SUIT uses a serialization format based on Concise Binary Object Representation (CBOR), and CBOR object signing and encryption (COSE) to ensure security.
The SUIT procedure features a flexible, script-based system that allows the DFU implementation to be customized, particularly for devices with multiple CPU cores.

It also features a root manifest that contains the main instructions of the SUIT procedure, as well as local manifests for each of its required dependencies.
The SUIT manifests control the invocation procedure at the same level of detail as the update procedure.
See the :ref:`ug_nrf54h20_suit_manifest_overview` page to read more about the contents of the manifests.

The nRF54H Series contains multiple CPUs with one dedicated CPU for the Secure Domain.
The Secure Domain CPU firmware is validated and started by the Secure Domain ROM.
When it starts execution, it continues the SoC boot sequence according to the manifest(s).
The local domain CPU firmware images are validated and started by the Secure Domain as instructed by the manifest.

Once the invocation process is complete, the SDFW is still active and may serve specific requests from specified domains.
Therefore, unlike in MCUboot, the application core and other cores may use the SDFW services.
(See the :ref:`ug_nrf54h20_suit_compare_other_dfu` page for more details and further comparison of SUIT with other DFU and bootloader procedures.)
The bootloader SDFW image provided by Nordic Semiconductor is offered in binary form.
Along with this, you can compose a final image with your own application image that is signed by your own keys.

.. figure:: images/nrf54h20_suit_example_update_workflow.png
   :alt: Example of the anticipated workflow for an application domain update using SUIT

.. _ug_suit_dfu_suit_concepts:

SUIT-specific concepts
**********************

Below is a description of SUIT-specific concepts.

.. _ug_suit_dfu_component_def:

Component
=========

An updatable logical block of firmware, software, configuration, or data structure.
Components are the elements that SUIT operates on.
They are identified by the ``Component_ID`` in the manifest and are abstractions that map to memory locations on the device.
For example, a memory slot on a device that contains one firmware image is a typical example of a component, though components can be of any size.

Command sequence
================

A set of commands.
Commands include both directives and conditions.
Most commands operate on components.

Directive
---------

An action for the recipient device to perform.
For example, to copy code or a data payload from the source component to the specified destination.

Condition
---------

A test that passes or fails for a specific property of the recipient device or its component(s).
For example, to ensure that the digest of the code or data in a specific component is equal to the expected value.

Envelope
========

An outer container for the manifest that may also contain code or data payloads.
Code or data payloads are optional in the envelope because the manifest can be created so that payload fetching is encoded within the command sequences.

The SUIT envelope includes: an authentication wrapper, the manifest, severable elements, integrated payloads, and the integrated dependencies.
Below is a description of the contents of the SUIT envelope structure that have not yet been described.

Authentication wrapper
----------------------

Every SUIT envelope contains an authentication wrapper.
The wrapper contains cryptographic information that protects the manifest, and includes one or more authentication blocks.

The authentication wrapper is important because it checks the authenticity of the manifest, but it is not involved in executing command sequences.

.. figure:: images/nrf54h20_suit_envelope_structure.png
   :alt: SUIT envelope structure

   SUIT envelope structure

Manifest
--------

A bundle of metadata describing one or more pieces of code or data payloads.
This includes instructions on how to obtain those payloads, as well as how to install, verify, and invoke them.
These instructions are encoded in the manifest in the form of command sequences.
See the :ref:`ug_nrf54h20_suit_manifest_overview` page for more details about the contents of a manifest.
Each manifest, either the root or dependency manifest, is encased in its own envelope.

.. note::

   The manifest is the most important concept within SUIT.
   The manifest is represented in a file, as either a YAML or JSON file based on Nordic Semiconductor's implementation, that can be edited to control aspects of the DFU.

Severable elements
------------------

Severable elements are elements that belong to the manifest but are held outside of the manifest.
They can later be deleted when they are no longer needed to save storage space.
To maintain integrity, a digest of the severable element is kept inside the manifest.
These are optional for SUIT envelopes.

Integrated payloads
-------------------

Integrated payloads are payloads that are integrated within the envelope of the manifest.
This allows for a one-step update, where everything needed for the update is in one image (the envelope).
These are optional for SUIT envelopes.

Integrated dependencies
-----------------------

Integrated dependencies contain the manifests needed for any required dependencies and are encased in their own SUIT envelope structure.
These are optional for SUIT envelopes and only necessary if there are dependencies needed for the DFU.

.. _ug_suit_dfu_suit_procedure:

SUIT procedure
**************

The SUIT procedure uses a SUIT envelope.
This envelope is a container to transport an update package.

An update package contains an authentication wrapper, one root manifest within an envelope, severable elements, one or more payloads as well as integrated dependencies.
Payloads can be either:

* Images

* Dependency manifests (each in their own envelope)

* Other data

Payloads can be distributed individually or embedded in the envelope of the manifest where it is used.
This means that an update package or invocation process can be distributed in one large package or as several small packages.

.. figure:: images/nrf54h20_suit_example_update_package.png
   :alt: Example of an update package

   Example of an update package

SUIT workflows
==============

There are two anticipated workflows for the recipient device that is receiving the update: the update procedure and the invocation procedure.

The update procedure contains the following steps:

.. figure:: images/nrf54h20_suit_update_workflow.png
   :alt: Update procedure workflow

   Update procedure workflow

The invocation procedure contains the following steps:

.. figure:: images/nrf54h20_suit_invocation_workflow.png
   :alt: Invocation procedure workflow

   Invocation procedure workflow

To follow these workflows, there are six main sequences in the SUIT procedure that belong to either the update or the invocation procedure.

The update procedure has three sequences:

* ``dependency-resolution`` - prepares the system for the update by identifying any missing dependency manifests.

* ``payload-fetch`` - all non-integrated payloads are requested over the network.

* ``install`` - the downloaded payloads are copied to their final location.

The following is an example of `Diagnostic Notation`_ (decoded CBOR) that features the update procedure's ``payload-fetch``:

.. code-block::

   / payload-fetch / 16:<< [
         / directive-set-component-index / 12,1 ,
         / directive-override-parameters / 20,{
            / image-digest / 3:<< [
               / algorithm-id / -16 / "sha256" /,
               / digest-bytes / h'0011…76543210'

         ] >>,
         / uri / 21:'http://example.com/file.bin',

      } ,
      / directive-fetch / 21,2 ,
      / condition-image-match / 3,15
   ] >>,

   / install / 17:<< [
      / directive-set-component-index / 12,0 ,

      / directive-override-parameters / 20,{
         / source-component / 22:1 / [h'02'] /,

      } ,
      / directive-copy / 22,2 ,
      / condition-image-match / 3,15
   ] >>,


The invocation procedure has three sequences, although not all of them are needed for every use case.
They are as follows:

* ``validate`` - calculates the digest and checks that it matches the expected digest to ensure that a secure invocation process can take place.

* ``load`` - is used in special cases when the firmware needs to be moved before invoking it.

* ``invoke`` - hands over execution to the firmware.
