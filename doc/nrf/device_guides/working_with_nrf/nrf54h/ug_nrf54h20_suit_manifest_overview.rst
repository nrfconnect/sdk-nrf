:orphan:

.. _ug_nrf54h20_suit_manifest_overview:

SUIT manifest overview
######################

The SUIT DFU procedure features manifests, which are encased in a SUIT envelope.
The SUIT manifest contains components, which is the main element that the SUIT DFU procedure operates on.

Out-of-the-box, Nordic Semiconductor's implementation of SUIT features a standard hierarchical manifest topology.
This includes the following:

* A root manifest that contains the main execution instructions for the DFU within the different Domains.

* Two local manifests, one for the Application Domain, another for the Radio Domain.

* Manifests for the SecDom, which are controlled by Nordic Semiconductor and cannot be modified.
  This includes a Nordic "Top" manifest, one specifically for the SecDom firmware (SDFW), and the System Controller.

You can read more about manifest topology in the :ref:`ug_nrf54h20_suit_hierarchical_manifests` page.

Manifest templates are provided by Nordic Semiconductor, and automatically generated when you build the ``nrf54h_suit_sample`` sample.
These templates work out-of-the-box for development and testing purposes.

This document describes the contents of a SUIT manifest.
If you want to modify elements of the manifest template, see the :ref:`ug_nrf54h20_suit_customize_dfu` user guide.

.. _ug_how_suit_manifest_works:

How ``SUIT_Manifest`` works
***************************

The ``SUIT_Manifest``, found within root and local manifests, contains several sequences.
These sequences are data structures that can contain directives and conditions.

.. _ug_suit_dfu_suit_manifest_elements:

Manifest elements
=================

``SUIT_Manifest`` contains contains metadata elements and command sequences (a kind of "scripts") within its structure.
These scripts contain the commands that will be executed at certain stages of the update process.
Additionally, they provide a shortcut when a component's information, or other data, needs to be repeated throughout the manifest.

.. caution::

   Sequences will be provided within the SUIT manifest templates for typical use cases.
   These sequences can be customized, but with caution.
   This is only recommended for advanced use cases.

The SUIT manifest contains the following elements:

``suit-manifest-version``
-------------------------

Checks and compares the version number of the manifest format, or, in other words, the serialization format.
If the SUIT processor receives a manifest and sees a version number it does not recognize, it will fail due to incompatibility.

``suit-manifest-sequence-number``
---------------------------------

This is a SUIT-specific version number of the software that is contained in this manifest.
It is compared against the existing sequence number to verify that the update is newer than the current software.
The sequence number does not have to match the official version number of the software, it only needs to increase for each update.

.. _ug_suit_dfu_suit_common:

``suit-common``
---------------

This element contains two subsections:

* ``suit-components`` - a data field that contains all declared components to be targeted in the manifest.

   Components are identified by ``SUIT_Component_Identifier``, which is introduced by Nordic Semiconductor's implementation of the SUIT procedure.
   For a list of available components, see the :ref:`ug_nrf54h20_suit_components` page.

* ``suit-shared-sequence`` - a sequence that executes before the other sequences.

   It supports only a few directives and conditions.

   For example, when performing a DFU, the SUIT processor may be instructed to run ``suit-payload-fetch``, but first ``suit-shared-sequence`` runs before each sequence to save memory space.
   This is done by declaring items, such as the vendor ID, in ``suit-shared-sequence`` once rather than declaring them separately for each update or invocation procedure.

.. _ug_suit_dfu_suit_concepts_sequences:

Sequences
---------

SUIT manifest contains the following command sequences:

* ``suit-payload-fetch`` - obtains the needed payloads.

* ``suit-install`` - installs payloads.

   Typical actions may include: verifying a payload stored in temporary storage, coping a staged payload from temporary storage, and unpacking a payload.

* ``suit-validate`` - validates that the state of the device is correct and okay for booting.

   Typically involves image validation.

* ``suit-load`` - prepares payload(s) for execution.

   A typical action of this sequence is to copy an image from the permanent storage into the RAM.

* ``suit-invoke`` - invokes (boots) image(s).

* ``suit-dependency-resolution`` - prepares the system for the update by identifying and fetching any missing dependency manifests.

.. _ug_suit_dfu_suit_directives:

Directives
==========

The SUIT procedure defines the following directives:

* ``set-component-index`` - defines the component(s) to which successive directives and conditions will apply.

* ``override-parameters`` - allows the manifest to configure the behavior of future directives or conditions by changing (as in, setting or modifying) parameters that are read by those directives or conditions.

* ``fetch`` - retrieves the payload from a specified Uniform Resource Identifier (URI) and stores it in the destination component.
  A URI is provided in the ``override-parameters`` directive.
  The URI may indicate an external source, for example, HTTP or FTP, or the envelope (as a fragment-only reference as defined in `RFC3986 <https://datatracker.ietf.org/doc/html/rfc3986>`__, such as ``"#app_image.bin"``).

* ``copy`` - transfers the image from the source component to the destination component.
  The source component is provided in the ``override-parameters`` directive.

* ``write`` - works similarly to ``copy``, except that the source image is embedded in the manifest.
  This directive is best for small blocks of data due to manifest size limitations.

* ``invoke`` - starts the firmware. (In other words, "boots" the firmware.)

* ``try-each`` -  runs multiple ``SUIT_Command_Sequence`` instances, trying each one in succession.
  It stops when one succeeds or continues to the next if one fails, making it valuable for handling alternative scenarios.

* ``run-sequence`` - runs a single ``SUIT_Command_Sequence``.

.. _ug_suit_dfu_suit_conditions:

Conditions
==========

The SUIT procedure defines the following conditions:

* ``class-identifier``, ``vendor-identifier``, and ``device-identifier`` - these conditions make sure that the manifest procedure is working with the correct device.
  The correct UUIDs (16 bytes) must be given.

   .. note::

      Although not required, it is strongly recommended to change the values for ``class-identifier`` and ``vendor-identifier`` in the provided manifest templates.
      Read the :ref:`ug_suit_modify_class_vend_id` section of the :ref:`ug_nrf54h20_suit_customize_dfu` user guide for instructions.

* ``image-match`` -  checks the digest of an image.
  The expected digest and corresponding component are set here.
  It goes into the component and calculates the digest of the component, then checks it against the expected digest.

* ``component-slot`` - checks which component slot is currently active, if a component consists of multiple slots.
  Slots are alternative locations for a component, where only one is considered "active" at one time.

   It also checks which component, or memory location, is unoccupied so you can download the new image to the unoccupied slot.
   After reboot, the unoccupied component now has the new image, and the active image is not overridden.
   This follows an A/B slot system.

* ``check-content`` -  a special case of image matching that matches directly with expected data, not a digest.
  For use with small components where the overhead of digest checking is not wanted. Typically used when you want the manifest to check something other than the firmware.

   As opposed to ``image-match``, the specified component is checked against binary data that is embedded in the manifest with what is already installed in another component.

* ``abort`` - if you want the procedure to fail.

A sample description of ``SUIT_Manifest`` in CDDL is shown below.
Note that optional elements are preceded by a ``?``.
For more information about CDDL's syntax, see the IETF's `RFC 8610 <https://datatracker.ietf.org/doc/rfc8610/>`__.

.. code::

   SUIT_Manifest = {
      suit-manifest-version => 1,
      suit-manifest-sequence-number => uint,
      suit-common => bstr .cbor SUIT_Common,

      ? suit-validate => bstr .cbor SUIT_Command_Sequence,
      ? suit-load => bstr .cbor SUIT_Command_Sequence,
      ? suit-invoke => bstr .cbor SUIT_Command_Sequence,
      ? suit-payload-fetch => bstr .cbor SUIT_Command_Sequence,
      ? suit-install => bstr .cbor SUIT_Command_Sequence,
      ? suit-text => bstr .cbor SUIT_Text_Map

      * $$SUIT_Manifest_Extensions,
   }
