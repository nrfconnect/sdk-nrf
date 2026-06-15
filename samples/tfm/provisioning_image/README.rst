.. _provisioning_image:

.. ncs-sample::
   :title: TF-M: Provisioning image

   Running the provisioning image sample will initialize the provisioning process of a device in a manner compatible with Trusted Firmware-M (TF-M).
   This sample does not include a TF-M image, it is a Zephyr image intended to be flashed, run, and erased before the TF-M image is flashed.

   After completion, the device is in the Platform Root-of-Trust (PRoT) security lifecycle state called **PRoT Provisioning**.
   For more information about the PRoT security lifecycle, see `ARM Platform Security Model 1.1`_.

   When built for the ``nrf5340dk/nrf5340/cpuapp`` board target, this image by default also includes the :ref:`provisioning_image_net_core` sample as an image for the network core (``nrf5340dk/nrf5340/cpunet`` board target).
   The image demonstrates how to disable the debugging access on the network core by writing to the ``UICR.APPROTECT`` register.

.. _provisioning_image_workflow:

Relationship to the TF-M application
************************************

Provisioning is a one-time operation that must be completed before a TF-M application (such as :ref:`tfm_psa_template`) runs for the first time.
It generates and stores the device-unique key material (on nRF54L Series devices, the CRACEN IKG seed) and advances the PSA security lifecycle state from **Device Assembly and Test** to **PRoT Provisioning**.
Because this step generates secret key material and permanently changes the lifecycle state, it is delivered as a dedicated image rather than built into the application.

Keeping provisioning in a separate image lets you:

* Provision devices in a secure or otherwise controlled environment (for example, a trusted production line), and program the application firmware later in a different, less-trusted environment.
* Erase and re-provision a development device without rebuilding the application.

The typical development flow is:

1. Build this provisioning image, flash it, and let it run once (see `Building and running`_).
   The device transitions to the **PRoT Provisioning** state.
#. Build and flash your TF-M application, for example :ref:`tfm_psa_template`.
   On the first boot, TF-M completes provisioning by advancing the lifecycle state to **Secured**.

On nRF54L Series devices, applications such as :ref:`tfm_psa_template` build this provisioning image for you by default as a build-only sub-image, which is the recommended way to provision the device when building those applications.
Building the provisioning image standalone, as described here, is mainly useful for provisioning devices independently of the application.
See :ref:`tfm_psa_template_provisioning_image_bundled`.

.. note::
   After the device has been provisioned, program the application with a plain ``west flash`` command.
   Using ``--erase`` or ``--recover`` wipes the provisioned keys and lifecycle state, and you must repeat the provisioning step.

Requirements
************

The following development kits are supported:

.. table-from-sample-yaml::

The sample requires :ref:`lib_hw_unique_key`.
On nRF5340 and nRF91x Series devices, it also requires :ref:`lib_identity_key`.

On nRF54L Series devices, no separate identity key is stored.
The IAK is sourced from CRACEN.

.. note::
   After provisioning, do not erase the OTP (for example, by using ``ERASEALL``).
   Erasing the OTP removes the provisioned keys and lifecycle state.

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

   * On nRF5340 and nRF91x Series devices, random MKEK and MEXT keys are generated and stored in the key management unit (KMU).
   * On nRF54L Series devices, the system writes a random IKG seed to CRACEN KMU slots 183-185.
     MKEK, MEXT, and other HUK-derived keys are not stored.
     Instead, CRACEN IKG derives these keys on demand from the seed.

#. On nRF5340 and nRF91x Series devices, the system generates a random secp256r1 identity key.
   The system stores this key in the KMU, encrypted with the MKEK, and uses it as the Initial Attestation Key (IAK).

   On nRF54L Series devices, the IAK is sourced from the CRACEN IKG identity key.
   The system does not write a separate identity key.
#. The implementation ID is written to OTP.

On nRF54L Series devices, |NSIB| (NSIB, also called b0) validates firmware against a key stored in the CRACEN KMU, rather than in the OTP PROVISION region used on nRF5340.
This NSIB public key corresponds to the application's signing key, so it is provisioned from a ``keyfile.json`` generated by the application build (for example, :ref:`tfm_psa_template`), not by this sample.
The ``west flash --recover`` command uses that file with ``nrfutil device x-provision-keys`` to write the NSIB Ed25519 public key to KMU slot 242 before b0 runs for the first time.


Configuration
*************

|config|


Building and running
********************

.. |sample path| replace:: :file:`samples/tfm/provisioning_image`

.. include:: /includes/build_and_run.txt

On nRF54L Series devices, the NSIB public key must also be provisioned to the CRACEN KMU before the application can boot.
Because this key belongs to the application, provision it from the application build rather than from a standalone build of this sample.
The recommended flow builds this provisioning image together with the application and flashes it with ``west flash --recover``, using the ``keyfile.json`` generated by the application.
See :ref:`tfm_psa_template_provisioning_image_bundled`.

If the provisioned key does not match the key used to sign the image chain, b0 rejects MCUboot with error ``-102`` (``ESIGINV``).

Testing
=======

After programming the sample, the following output is displayed in the console on nRF5340 and nRF91x Series devices:

.. code-block:: console

    Successfully verified PSA lifecycle state ASSEMBLY!
    Successfully switched to PSA lifecycle state PROVISIONING!
    Generating random HUK keys (including MKEK)
    Writing the identity key to KMU
    Success!

On nRF54L Series devices, the identity key step is omitted:

.. code-block:: console

    Successfully verified PSA lifecycle state ASSEMBLY!
    Successfully switched to PSA lifecycle state PROVISIONING!
    Generating random HUK keys (including MKEK)
    Success!

If an error occurs, the sample logs a ``Failure: ...`` message describing the failed step and stops without printing ``Success!``.

.. note::
   The device cannot transition from **PRoT Provisioning** back to **Device Assembly and Test**.

   To run the sample again, reset the OTP by using ``west flash --erase`` (nRF5340 and nRF91x Series) or ``west flash --recover`` (nRF54L Series).
   Both operations wipe all provisioned keys, so you must repeat the full sequence.

Dependencies
************

The following libraries are used:

* :ref:`lib_hw_unique_key`
* :ref:`lib_identity_key` (nRF5340 and nRF91x only)
