.. _provisioning_image:

TF-M: Provisioning image
########################

.. contents::
   :local:
   :depth: 2

Running the provisioning image sample will initialize the provisioning process of a device in a manner compatible with Trusted Firmware-M (TF-M).
This sample does not include a TF-M image, it is a Zephyr image intended to be flashed, run, and erased before the TF-M image is flashed.

After completion, the device is in the Platform Root-of-Trust (PRoT) security lifecycle state called **PRoT Provisioning**.
For more information about the PRoT security lifecycle, see `ARM Platform Security Model 1.1`_.

When built for the ``nrf5340dk/nrf5340/cpuapp`` board target, this image by default also includes the :ref:`provisioning_image_net_core` sample as an image for the network core (``nrf5340dk/nrf5340/cpunet`` board target).
The image demonstrates how to disable the debugging access on the network core by writing to the ``UICR.APPROTECT`` register.

Requirements
************

The following development kits are supported:

.. table-from-sample-yaml::

The sample requires :ref:`lib_hw_unique_key`, and on nRF5340 and nRF91x also
:ref:`lib_identity_key`. On nRF54L no separate identity key is stored; the IAK
is sourced from CRACEN.

.. note::
   Once the device is provisioned, the OTP must not be erased (for example, by ``ERASEALL``), as this will erase the provisioned keys and lifecycle state.

Overview
********

The Platform Security Architecture (PSA) security model defines the PRoT security lifecycle states.
This sample performs the transition from the PRoT security lifecycle state **Device Assembly and Test** to the **PRoT Provisioning** state.

PRoT Provisioning is a state where the device platform security parameters are generated.
This enables a TF-M image to transition to the PRoT security lifecycle state **Secured** at a later stage.

The sample performs the following operations:

1. The device is verified to be in the Device Assembly and Test state.
#. The device is transitioned to the PRoT Provisioning state.
#. Hardware unique key (HUK) material is provisioned:

   * On nRF5340 and nRF91x, random MKEK and MEXT keys are generated and stored in the
     key management unit (KMU).
   * On nRF54L, a random IKG seed is written to CRACEN KMU slots 183-185. MKEK, MEXT
     and other HUK-derived keys are not stored; they are derived on demand by CRACEN
     IKG from the seed.

#. On nRF5340 and nRF91x only: a random secp256r1 identity key is generated and stored in
   the KMU (encrypted with the MKEK) and used as the Initial Attestation Key (IAK).
   On nRF54L the IAK is sourced from the CRACEN IKG identity key; no separate identity
   key is written.
#. The implementation ID is written to OTP.

On nRF54L, b0 (NSIB) validates firmware against a key in CRACEN KMU rather than in the OTP
PROVISION region used on nRF5340. The sysbuild generates a ``keyfile.json`` that
``west flash --recover`` uses with ``nrfutil device x-provision-keys`` to write the NSIB
Ed25519 public key to KMU slot 242 before b0 first runs.


Configuration
*************

|config|


Building and running
********************

.. |sample path| replace:: :file:`samples/tfm/provisioning_image`

.. include:: /includes/build_and_run.txt

On nRF54L, use ``west flash --recover`` so the sysbuild-generated
``keyfile.json`` is applied. The shared sample key under
:file:`samples/tfm/common/keys/` is used by default:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp nrf/samples/tfm/provisioning_image -d build_provisioning_image
   west flash --recover -d build_provisioning_image

To use a different NSIB signing key, pass it with ``-DNSIB_KEY_FILE=<pem>``
and set :kconfig:option:`SB_CONFIG_SECURE_BOOT_SIGNING_KEY_FILE` to the same
key when building ``tfm_psa_template``. Otherwise b0 will reject MCUboot with
error ``-102`` (``ESIGINV``).

Testing
=======

After programming the sample, the following output is displayed in the console on nRF5340 and nRF91x:

.. code-block:: console

    Successfully verified PSA lifecycle state ASSEMBLY!
    Successfully switched to PSA lifecycle state PROVISIONING!
    Generating random HUK keys (including MKEK)
    Writing the identity key to KMU
    Success!

On nRF54L series, the identity key step is omitted:

.. code-block:: console

    Successfully verified PSA lifecycle state ASSEMBLY!
    Successfully switched to PSA lifecycle state PROVISIONING!
    Generating random HUK keys (including MKEK)
    Success!

If an error occurs, the sample logs a ``Failure: ...`` message describing the failed step and stops without printing ``Success!``.

.. note::
   The device cannot transition from **PRoT Provisioning** back to
   **Device Assembly and Test**. To re-run the sample, reset the OTP with
   ``west flash --erase`` (nRF5340 and nRF91x) or ``west flash --recover``
   (nRF54L). Both wipe all provisioned keys, so the full sequence must be
   repeated.

Dependencies
************

The following libraries are used:

* :ref:`lib_hw_unique_key`
* :ref:`lib_identity_key` (nRF5340 and nRF91x only)
