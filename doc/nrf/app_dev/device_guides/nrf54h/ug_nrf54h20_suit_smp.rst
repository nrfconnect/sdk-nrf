.. _ug_nrf54h20_suit_smp:

SUIT and SMP protocol (including SUIT SMP extension)
####################################################

.. contents::
   :local:
   :depth: 2

SUIT DFU offers a flexible and extensible update process.
You can use the Zephyr management subsystem, through the Simple Management Protocol (SMP) protocol, to manage SUIT-based images alongside existing MCUboot images.

Slots and images in Zephyr
==========================

In the |NCS|, *slot* and *image* concepts are derived from MCUboot.
An *image* consists of two slots: a *primary* and a *secondary*.
The application runs from the primary slot, while updates are uploaded to the secondary slot.
MCUboot is responsible for swapping these slots during boot.

The slots support a single upgradable application, with equal sizes.
Zephyr supports up to two images, each represented by an index.

.. note::
   Image metadata, such as version information, is embedded within the image payload.

Components in SUIT
==================

SUIT defines a *component* as an updatable logical block, which can be firmware, software, configuration, or data.
This is similar to how MCUmgr defines an *image*.
Updates are delivered to the device as a SUIT *envelope*, containing SUIT manifests and, optionally, integrated payloads.
The relationships between components are maintained through SUIT manifests.

Metadata, such as version information, expected image digest, and image size is embedded into the SUIT manifest.

MCUmgr and SUIT - key similarities and differences
===================================================

The following table summarizes key similarities and differences between MCUmgr and SUIT:

+-----------------------------+-------------------------+----------------------------------+
| Feature                     | MCUmgr                  | SUIT                             |
+=============================+=========================+==================================+
| Metadata                    | Attached to the images  | Defined by a separate SUIT       |
|                             |                         | manifest                         |
+-----------------------------+-------------------------+----------------------------------+
| Image Topology              | Slots (primary,         | Manifests organized into         |
|                             | secondary) organized as | a hierarchical tree              |
|                             | a list                  |                                  |
+-----------------------------+-------------------------+----------------------------------+
| Slot/Image Configuration    | Slots defined in        | Slot definitions can be modified |
|                             | MCUboot; difficult to   | through the SUIT manifest,       |
|                             | change after deployment | allowing greater flexibility     |
+-----------------------------+-------------------------+----------------------------------+
| Installation/Boot           | Limited by static       | Customizable through SUIT        |
| Customization               | image metadata          | manifest commands                |
+-----------------------------+-------------------------+----------------------------------+
| State information           | Embedded within the     | Update candidate existence is    |
|                             | image slot metadata     | represented within the SUIT      |
|                             |                         | manifest                         |
+-----------------------------+-------------------------+----------------------------------+

SUIT in SMP image management group
==================================

The SMP protocol can provide SMP-based SUIT image management, assuming that even a complex SUIT-based system can be represented as a *single image* system.
In this model, the contents of the *primary slot* of image 0  represent the currently installed SUIT root manifest, while the secondary slot of image 0 holds the update candidate root manifest.
For more information on images and slots in Zephyr, see :ref:`zephyr:mcumgr_smp_group_1`


Get state of images request
---------------------------

This command retrieves images and their states.
The response is in CBOR format:

.. code-block::

    {
    (str)"images" : [
        {
            (str,opt)"image"      : (int)       //SUIT - always 0.
            (str)"slot"           : (int)       //SUIT - 0 - "primary" - installed Root manifest, 1 - update candidate manifest in the DFU partition.
            (str)"version"        : (str)       //SUIT - sequence-number from root manifest.
            (str)"hash"           : (byte str)  //SUIT - digest of root manifest.
        }
        ...
    ]}

Image upload request
--------------------

This command uploads an image containing a SUIT envelope to be stored in the DFU partition or a dump of the DFU cache partition.
The CBOR data format is the following:

.. code-block::

    {
        (str,opt)"image"    : (uint)     //SUIT - 0 - DFU partition, 1 to n - DFU cache partition 0 to n-1.
        (str,opt)"len"      : (uint)
        (str)"off"          : (uint)
        (str,opt)"data"     : (byte str)
    }

Set state of images request
---------------------------

This command triggers an installation process using the SUIT envelope stored in the DFU partition.
The CBOR data format is the following:

.. code-block::

    {
        (str)"confirm" : (bool) //Must be set to "true".
    }

SUIT SMP protocol extension
===========================

The existing image management approach works well for basic scenarios involving single-step SUIT envelope pushes.
More advanced scenarios require defining a new SMP management group.

Management Group ID
-------------------

The new SUIT SMP protocol uses ``MGMT_GROUP_ID_PERUSER + 2`` (for example, 66).

Commands
--------

Get SUIT manifests list request
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This command allows retrieving information about the roles of manifests supported by the device.
The SUIT SMP operation code (OP) and command ID are the following:

* ``OP: MGMT_OP_READ (0), MGMT_OP_READ_RSP(1)``
* ``Command ID: 0``

CBOR data of successful response:

.. code-block::

    {
        (str)"rc"        : (uint)
        (str)"manifests" : [
            {
                (str)"role"                     : (uint)
            }
        ]
    }

where:

* *role* - Manifest role encoded as two nibbles: `<domain ID> <index>`.

* *rc* - Return code:

  * *MGMT_ERR_ENOTSUP* - Unsupported command.
  * *MGMT_ERR_EMSGSIZE* - Device was unable to encode the answer.

Get SUIT manifest state request
-------------------------------

This command allows you to get information about the configuration of supported manifests and selected attributes of installed manifests of a specified role.
The SUIT SMP operation code (OP) and command ID are the following:

* ``OP: MGMT_OP_READ (0), MGMT_OP_READ_RSP(1)``
* ``Command ID: 1``

CBOR data of request:

.. code-block::

    {
        (str)"role"    : (uint)
    }

where:

* *role* - Manifest role, one of the values returned by Get SUIT Manifests List Request.

CBOR data of successful response:

.. code-block::

    {
        (str)"rc"                               : (uint)
        (str)"class_id"                         : (byte str)
        (str)"vendor_id"                        : (byte str)
        (str)"downgrade_prevention_policy"      : (uint)
        (str)"independent_updateability_policy" : (uint)
        (str)"signature_verification_policy"    : (uint)
        (str, opt)"digest"                      : (byte str)
        (str, opt)"digest_algorithm"            : (int)
        (str, opt)"signature_check"             : (uint)
        (str, opt)"sequence_number"             : (uint)
        (str, opt)"semantic_version"            : [0*5 int]
    }

where:

* *class_id* - UUID representing a given manifest type.
* *vendor_id* - Vendor ID, manifest class ID was generated in the space of this value.
* *downgrade_prevention_policy* - Downgrade prevention policy of the given supported manifest.
* *independent_updateability_policy* - Indicates whether the given supported manifest be updated independently.
* *signature_verification_policy* - Configured signature verification policy of a given manifest.
* *digest* - Digest of manifest carried in respective manifest authentication block.
  If the digest calculated on manifest content does not match the digest stored in the respective manifest authentication block, it is assumed that the manifest is not installed or corrupted.
  In such cases, the response will not contain the 'digest' field, and the remaining fields will not contain any meaningful information.
* *digest_algorithm* - See ietf-suit-manifest, cose-alg-sha-256 = -16, cose-alg-sha-512 = -44,…
* *signature_check* - Signature check status.
* *sequence_number* - Value taken from the respective field of the manifest.
* *semantic_version* - An array of up to 5 integers, taken from the respective field of the manifest, allowing expression of the semantic version, for example, 1.17.5-rc.2.

* *rc* - Return code:

  * *MGMT_ERR_ENOTSUP* - Unsupported command.
  * *MGMT_ERR_EMSGSIZE* - The device was unable to encode the answer.

Example of human-readable response using Get SUIT Manifests List Request and Get SUIT Manifest State Request(s):

.. code-block:: shell

    ./newtmgr -c serial0 suit manifests
    Manifests:

     class ID:  3f6a3a4d-cdfa-58c5-acce-f9f584c41124 (nRF54H20_sample_root)
     vendor ID: 7617daa5-71fd-5a85-8f94-e28d735ce9f4 (nordicsemi.com)
       role: 0x20 (Root Manifest)
       digest: d0b69723..15d7140d (sha-256)
       signature check passed
       sequence number: 2
       semantic version: 1.17.5-rc.2

     class ID:  00112233-4455-5577-8899-aabbccddeeff
     vendor ID: 00110011-0011-0011-0011-001100110011
       role: 0x21 (Application Recovery Manifest)
       Manifest not installed or damaged!

Candidate envelope upload request
---------------------------------

The command delivers a SUIT envelope to the device.
Once the upload is completed, the device validates the delivered envelope and starts SUIT processing.
The SUIT SMP operation code (OP) and command ID are the following:

* ``OP: MGMT_OP_WRITE (2), MGMT_OP_WRITE_RSP(3)``
* ``Command ID: 2``

CBOR data of request:

.. code-block::

    {
        (str,opt)"len"            : (uint)
        (str,opt)"defer_install"  : (bool)
        (str)"off"                : (uint)
        (str)"data"               : (byte str)
    }

where:

* *len* - Length of an envelope.
  Must be provided with the first chunk when "off" is 0.
* *defer_install* - Evaluated by the device on the first delivered chunk when "off" is 0.
  When set, it indicates that envelope processing must NOT be triggered upon envelope delivery.
* *off* - Offset of the envelope chunk the request carries.
* *data* - Image chunk to be stored at the provided offset.

.. note::
   Request with len = 0, defer_install != true, off = 0 may be utilized to trigger processing on an already delivered envelope.

CBOR data of response:

.. code-block::

    {
        (str)"rc"         : (uint)
        (str)"off"        : (uint)
    }

where:

* *off* - Position of the "write pointer" after the operation.
* *rc* - Return code.
  The following values represent unrecoverable errors; once an SMP client receives any of these, it must stop the current transfer:

  * *MGMT_ERR_EBADSTATE* - Possible reason - device reboot occurred in the middle of the transfer.
  * *MGMT_ERR_ENOENT* - DFU partition not found.
  * *MGMT_ERR_EINVAL* - The requested data structure cannot be decoded or incorrect information in the request was detected.
  * *MGMT_ERR_ENOMEM* - Not enough space to store the image.
  * *MGMT_ERR_ENOTSUP* - Unsupported command.

  The following errors are possibly recoverable:

  * *MGMT_ERR_EMSGSIZE* - The device was unable to encode the answer.
  * *MGMT_ERR_EUNKNOWN* - Issues with NVM erase/write operations.


Get missing image state request
-------------------------------

This SUIT command sequence can conditionally execute directives based, for example, on the digest of the installed image.
This allows for the SUIT candidate envelope to contain only SUIT manifests, and the images required to be updated are fetched by the device only if necessary.
In that case, the device informs the SMP client that a specific image is required, and then the SMP client delivers the requested image in chunks.
Due to the fact that SMP is designed in a client-server pattern and lacks server-sent notifications, the implementation is based on polling.
The SUIT SMP operation code (OP) and command ID are the following:

* ``OP: MGMT_OP_READ (0), MGMT_OP_READ_RSP(1)``
* ``Command ID: 3``

CBOR data of response:

.. code-block::

    {
        (str)"rc"                          : (uint)
        (str,opt)"stream_session_id"       : (uint)
        (str,opt)"resource_id"             : (byte str)
    }

where:

* *resource_id* - Resource identifier, typically in the form of a URI.
* *stream_session_id* - Session identifier.
  Non-zero value, unique for image request, not provided if there is no pending image request.
* *rc* - Return code:

  * *MGMT_ERR_ENOTSUP* - Unsupported command.
  * *MGMT_ERR_EMSGSIZE* - The device was unable to encode the answer.

Image upload request
--------------------

This command delivers a requested image to the device.
The SUIT SMP operation code (OP) and command ID are the following:

* ``OP: MGMT_OP_WRITE (2), MGMT_OP_WRITE_RSP(3)``
* ``Command ID: 4``

CBOR data of request:

.. code-block::

    {
        (str,opt)"stream_session_id"      : (uint)
        (str,opt)"len"                    : (uint)
        (str)"off"                        : (uint)
        (str)"data"                       : (byte str)
    }

where:

* *stream_session_id* - Session identifier.
  The same value as obtained using Get Missing Image Info Request.
  Must appear when "off" is 0.
* *len* - Length of an image.
  Must appear when "off" is 0.
* *off* - Offset of the image chunk the request carries.
* *data* - Image chunk to be stored at the provided offset.

CBOR data of response:

.. code-block::

    {
        (str)"rc"         : (uint)
        (str)"off"        : (uint)
    }

where:

* *off* - Offset of the last successfully written byte of the candidate envelope.
* *rc* - Return code.
  The following values represent unrecoverable errors; once an SMP client receives any of these, it must stop the current transfer:

  * *MGMT_ERR_EBADSTATE* - Possible reasons - device reboot occurred in the middle of the transfer.
  * *MGMT_ERR_EINVAL* - The requested data structure cannot be decoded or incorrect information in the request was detected.
  * *MGMT_ERR_ENOMEM* - Not enough space to store the image.
  * *MGMT_ERR_ENOTSUP* - Unsupported command.

  The following errors are possibly recoverable:

  * *MGMT_ERR_EMSGSIZE* - The device was unable to encode the answer.
  * *MGMT_ERR_EUNKNOWN* - Issues with the NVM erase/write operations.

Cache raw image upload request
------------------------------

This command delivers a requested image to the device.
The SUIT SMP operation code (OP) and command ID are the following:

* ``OP: MGMT_OP_WRITE (2), MGMT_OP_WRITE_RSP(3)``
* ``Command ID: 5``

CBOR data of request:

.. code-block::

    {
        (str,opt)"target_id"              : (uint)
        (str,opt)"len"                    : (uint)
        (str)"off"                        : (uint)
        (str)"data"                       : (byte str)
    }

where:

* *target_id* - Cache pool identifier.
  Must appear when "off" is 0.
* *len* - Length of an image.
  Must appear when "off" is 0.
* *off* - Offset of the image chunk the request carries.
* *data* - Image chunk to be stored at the provided offset.

CBOR data of response:

.. code-block::

    {
        (str)"rc"         : (uint)
        (str)"off"        : (uint)
    }

where:

* *off* - Offset of the last successfully written byte of the candidate envelope.
* *rc* - Return code.
  The following values represent unrecoverable errors; once an SMP client receives any of these, it must stop the current transfer:

  * *MGMT_ERR_EBADSTATE* - Possible reason - device reboot occurred in the middle of the transfer.
  * *MGMT_ERR_ENOENT* - Cache pool not found.
    dfu_cache_partition_n not defined in device DTS, or, for cache pool 0, dfu_partition not defined in DTS or candidate envelope not stored in dfu partition.
  * *MGMT_ERR_EINVAL* - The requested data structure cannot be decoded or incorrect information in the request was detected.
  * *MGMT_ERR_ENOMEM* - Not enough space to store the image.
  * *MGMT_ERR_ENOTSUP* - Unsupported command.

  The following errors are possibly recoverable:

  * *MGMT_ERR_EMSGSIZE* - The device was unable to encode the answer.
  * *MGMT_ERR_EUNKNOWN* - Some issues with NVM erase/write operations.

Cleanup request
---------------

This command erases the DFU partition and DFU cache partitions.
The SUIT SMP operation code (OP) and command ID are the following:

* ``OP: MGMT_OP_WRITE (2), MGMT_OP_WRITE_RSP(3)``
* ``Command ID: 6``

CBOR data of response:

.. code-block::

    {
        (str)"rc"                          : (uint)
    }

where:

* *rc* - Return code:

  * *MGMT_ERR_ENOTSUP* - Unsupported command.
  * *MGMT_ERR_EMSGSIZE* - The device was unable to encode the answer.
