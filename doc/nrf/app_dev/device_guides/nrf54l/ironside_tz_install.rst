.. _ug_nrf54l_ironside_tz_install:

Installing IronSide TrustZone
#############################

.. contents::
   :local:
   :depth: 2

IronSide TrustZone is installed on the device using the ``nrfutil ironside`` command.
Once it is installed, all firmware executed on the device must be signed with a key from the keyset associated with the installation.
The following sections describe how to install IronSide TrustZone, how to sign firmware, and how to reinstall it when needed.

.. _ug_nrf54l_ironside_tz_installing:

Installing
**********

To install IronSide TrustZone, run the following command:

.. code-block:: console

   nrfutil ironside install

This triggers an ``ERASEALL`` operation, which clears the device, and installs the latest version of IronSide TrustZone suitable for the selected device.

.. note::
   The IronSide TrustZone binaries must be signed before they are programmed to the device.
   Signing requires a *keyset* that contains the keys used to sign the IronSide TrustZone binaries and the application, as well as the keys needed to perform authenticated operations on the device, such as enabling authenticated debugging.

Providing the keyset
====================

You can provide the keyset used for signing in several ways.
Select the option that matches your setup:

.. tabs::

   .. group-tab:: nRF Cloud

      Use this option if you want to use the nRF Cloud signing servers to host your keyset.
      Pass the URL, token, and keyset ID to the command:

      .. parsed-literal::
         :class: highlight

         nrfutil ironside install --url *url* --token *token* --keyset-id *keyset_id*

      This automatically creates an nrfutil KMS service backend that you can use later.

   .. group-tab:: Existing nrfutil KMS service backend

      Use this option if you have already configured an nrfutil KMS service backend, for example if you have set up your own signing server.
      Pass the backend using the ``--kms`` parameter:

      .. parsed-literal::
         :class: highlight

         nrfutil ironside install --kms *backend_name*

   .. group-tab:: Existing keyset file

      Use this option if you already have a keyset file.
      Pass its path using the ``--keyset`` parameter:

      .. parsed-literal::
         :class: highlight

         nrfutil ironside install --keyset *path_to_keyset*

      This automatically creates an nrfutil KMS service backend that you can use later.

   .. group-tab:: Generate a new keyset (default)

      If you do not pass any of the parameters described in the other tabs, a new keyset is generated.
      This is the default behavior:

      .. code-block:: console

         nrfutil ironside install

      You are then prompted for where to store the generated keys:

      .. code-block:: console

         Installing new keyset, where should the keys be stored? (.nrfutil/keys/ddmmyyyy_keys.tar is default)

      This automatically creates an nrfutil KMS service backend that you can use later.

      .. note::
         Store the generated keyset in a safe place.
         All firmware executed on this device must be signed with one of the private keys in that keyset.

For a detailed overview of installing IronSide TrustZone and signing artifacts, see the Production Programming Guide for IronSide for your specific device.

Signing firmware
****************

After IronSide TrustZone is installed, all firmware executed on the device must be signed.
The build systems provided by Nordic Semiconductor do this automatically.

If you build your application with west, signing is performed automatically as part of the flashing step:

.. code-block:: console

   west build hello_world
   west flash

The ``west flash`` command invokes the required signing steps before programming the device.

Alternatively, you can create the required signatures manually:

.. parsed-literal::
   :class: highlight

   nrfutil kms sign *backend_name* *file*.hex
   nrfutil device program *file*\_signed.hex

The ``nrfutil kms sign`` command generates a signed file (for example, :file:`file_signed.hex`), which you then program to the device with ``nrfutil device program``.

Reinstalling
************

If you lose the keys generated during installation, you can install a new keyset by reinstalling IronSide TrustZone.
To do so, follow the steps in the :ref:`ug_nrf54l_ironside_tz_installing` section.

This is only possible as long as the device has not been transitioned to the ``deployed`` life-cycle state.
