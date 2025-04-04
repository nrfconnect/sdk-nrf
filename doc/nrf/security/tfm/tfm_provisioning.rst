.. _ug_tfm_provisioning:

TF-M provisioning
#################

.. contents::
   :local:
   :depth: 2

For devices that need provisioning, TF-M implements the following Platform Root of Trust (PRoT) security lifecycle states that conform to the `ARM Platform Security Model 1.1`_:

* Device Assembly and Test
* PRoT Provisioning
* Secured

The device starts in the **Device Assembly and Test** state.
The :ref:`provisioning_image` sample shows how to switch the device from the **Device Assembly and Test** state to the **PRoT Provisioning** state, and how to provision the device with hardware unique keys (HUKs) and an identity key.

To switch the device from the **PRoT Provisioning** state to the **Secured** state, set the :kconfig:option:`CONFIG_TFM_NRF_PROVISIONING` Kconfig option for your application.
On the first boot, TF-M ensures that the keys are stored in the Key Management Unit (KMU) and switches the device to the **Secured** state.
The :ref:`tfm_psa_template` sample shows how to achieve this.
