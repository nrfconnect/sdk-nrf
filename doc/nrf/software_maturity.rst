.. _software_maturity:

Software maturity levels
########################

.. contents::
   :local:
   :depth: 2

The |NCS| supports its various features and components at different levels of software maturity.
The tables at the end of this page summarize the maturity level for each feature and component supported in the |NCS|.
The following categories are used in the tables to classify the software maturity of each feature and component:

Supported
   The feature or component is implemented and maintained, and is suitable for product development.

Not supported
   The feature or component is neither implemented nor maintained, and it does not work.

Experimental
   The feature can be used for development, but it is not recommended for production.
   This means that the feature is incomplete in functionality or verification and can be expected to change in future releases.
   The feature is made available in its current state, but the design and interfaces can change between release tags.
   The feature is also labeled as ``EXPERIMENTAL`` in Kconfig files to indicate this status.

   .. note::
      By default, the build system generates build warnings to indicate when features labeled ``EXPERIMENTAL`` are included in builds.
      To disable these warnings, disable the :kconfig:option:`CONFIG_WARN_EXPERIMENTAL` Kconfig option.
      See :ref:`app_build_additions` for details.

See the following table for more details:

.. _software_maturity_table:

.. list-table:: Software maturity
   :header-rows: 1
   :align: center
   :widths: auto

   * -
     - Supported
     - Experimental
     - Not supported
   * - **Technical support**
     - Customer issues raised for features supported for developing end products on tagged |NCS| releases are handled by technical support on DevZone.
     - Customer issues raised for experimental features on tagged |NCS| releases are handled by technical support on DevZone.
     - Not available.
   * - **Bug fixing**
     - Reported critical bugs will be resolved in both ``main`` and the latest tagged versions.
     - Bug fixes and improvements are not guaranteed to be applied.
     - Not available.
   * - **Implementation completeness**
     - Complete implementation of the agreed features set.
     - Significant changes may be applied in upcoming versions.
     - A feature or component is available as an implementation, but is not compatible with (or tested on) a specific device or series of products.
   * - **API definition**
     - The API definition is stable.
     - The API definition is not stable and may change.
     - Not available.
   * - **Maturity**
     - Suitable for integration in end products.

       A feature or component that is either fully complete on first commit, or has previously been labelled *experimental* and is now ready for use in end products.

     - Suitable for prototyping or evaluation.
       Not recommended to be deployed in end products.

       A feature or component that is either not fully verified according to the existing test plans or currently being developed, meaning that it is incomplete or that it may change in the future.
     - Not applicable.

   * - **Verification**
     - Fully verified according to the existing test plans.
     - Incomplete verification
     - Not applicable.

Protocol support
****************

The following table indicates the software maturity levels of the support for each protocol:

.. sml-table:: top_level

HomeKit features support
************************

The following table indicates the software maturity levels of the support for each HomeKit feature:

.. toggle::

  .. sml-table:: homekit

Thread features support
***********************

The following table indicates the software maturity levels of the support for each Thread feature:

.. toggle::

  .. sml-table:: thread

Matter features support
***********************

The following table indicates the software maturity levels of the support for each Matter feature:

.. toggle::

  .. sml-table:: matter

Zigbee feature support
**********************

The following table indicates the software maturity levels of the support for each Zigbee feature:

.. toggle::

  .. sml-table:: zigbee

Security Feature Support
************************

The following sections contain the tables indicating the software maturity levels of the support for the following security features:

* Trusted Firmware-M
* PSA Crypto
* |NSIB|
* Hardware Unique Key

Trusted Firmware-M support
--------------------------

.. toggle::

  .. sml-table:: trusted_firmware_m

PSA Crypto support
------------------

.. toggle::

  .. sml-table:: psa_crypto

|NSIB|
------

.. toggle::

  .. sml-table:: immutable_bootloader

Hardware Unique Key
-------------------

.. toggle::

  .. sml-table:: hw_unique_key
