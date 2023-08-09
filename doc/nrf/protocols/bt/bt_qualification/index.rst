.. _ug_bt_qualification:

Bluetooth qualification
#######################

.. contents::
   :local:
   :depth: 2

Every product using the *Bluetooth®* Low Energy technology must be certified (qualified) by Bluetooth Special Interest Group (SIG).
Only when your product is qualified, you will be authorized to do the following:

* Prove that your products satisfy the requirements of the Bluetooth license agreements and specifications.
* Use Bluetooth trademark and logos.
* Sell products that use Bluetooth LE technology.

As a result, your products will be listed and can be promoted on the official Bluetooth SIG page.

Pre-qualified design
********************

A *design* is a specific configuration of hardware or software implementation (or both) of adopted Bluetooth specifications, either in parts or as a whole.
If relevant, you can use an existing design that has already been qualified (called *pre-qualified design*).
This allows you to speed up your product's qualification process by omitting a testing phase.

The |NCS| contains qualified portions of various Bluetooth features (for example, :ref:`ug_ble_controller_softdevice` versions).

Any qualified design offered by Nordic Semiconductor (for example, :ref:`ug_bt_mesh` versions) can be qualified independently by the customers.
However, this process may be complex and time-consuming, thus it is recommended to inherit Nordic Semiconductor's design qualifications in your End Product's listing.
If you want to create a Profile Subsystem that is not on the `Bluetooth Qualification Listing`_, run the relevant tests through the `Profile Tuning Suite (PTS)`_ tool and qualify the product.

A full list of Bluetooth product types including their descriptions is available on the `Bluetooth product types`_ page.

Requirements
************

Before you start the `Bluetooth SIG's Qualification Process`_, ensure the following:

1. You have access to your company's member account.
   To check if your company is already a SIG member, go to `Join Bluetooth SIG`_ page and search the Member Directory list.

#. Get acquainted with the Program Reference Document (PRD) and Declaration Process Document (DPD) to understand the basics of the qualification process.
   Log into your company's member account to review both `PRD and DPD`_ documents.

Qualification process
*********************

Although you can use pre-qualified designs, the Bluetooth Product qualification can only be inherited through action on your end.
Hence, you must complete a qualification process of all of your products by yourself.

You can either perform a full qualification process for a new product or edit an existing entry of a product that has already been qualified.
See the following sections for more information about both possibilities.

Creating new entry
==================

Log into `Launch Studio`_, the official qualification tool, that will guide you through one of the following processes depending on the state of your product:

* `Qualification process with no required testing`_ - Complete it if your product uses only pre-qualified Subsystems and implements APIs as instructed in the corresponding |NCS| documentation.
* `Qualification process with required testing`_ - Complete it in the following cases:

  * If your product uses a new Bluetooth design that is not yet qualified by Nordic Semiconductor.
  * If your product uses a pre-qualified Component.
  * If your product alters a previously qualified Bluetooth design by changing the code or configuring it in a way that can break the qualification requirements for the Nordic Semiconductor's qualified design.

Updating existing entry
=======================

.. note::
   An existing entry can be edited only by the person that completed the listing.
   If you want to change the entry owner, contact SIG support.

Update an existing qualification entry through the `Launch Studio`_ tool in the following cases:

* You changed an existing product's design with no changes in the product's features, hence you want to update the listing to indicate new updates or add new marketing information regarding this product.
* You can add a new product as a new entry to an existing list of product entries as there was no design change in the new product.
* You want to switch to another major |NCS| version for an existing product.
* In some cases, if you want to switch to another minor |NCS| version for an existing product.

You might need to alter the configuration of the qualified design or slightly modify parts of the existing design to better fit your application needs.
If you make such changes, you must check the following:

* Ensure that the resulting design does not change the Implementation Conformance Statement (ICS) that has already been declared for a relevant Nordic Semiconductor's qualified design.
* Ensure that the resulting design does not break the qualification of the existing design.

You can complete the checks by running relevant qualification tests using test tools indicated in the 'Launch Studio'_ tool.

Switching to another SDK version
--------------------------------

If you want to switch to another |NCS| version for an existing product, complete the following steps:

1. Go to the `Nordic Semiconductor Infocenter`_ and search for the SoC or SiP model you use.

#. Navigate to the relevant Compatibility Matrix directory.

#. Open the Bluetooth QDIDs article and, based on the table, confirm the following:

   * The version you want to use must be compatible with versions of other Subsystems you want to keep for your product.
   * Relevant Host and SoftDevice Controller Subsystems implemented in the |NCS| version you want to use must be pre-qualified.
     If both are pre-qualified, check the following:

     * If the version you want to use is fully pre-qualified and has the same Qualification Design ID (QDID) numbers as the ones currently used for your product, then no further action is required.
     * If the version you want to use has at least one QDID number that is different from the numbers already used for your product, then a new qualification is required.
       Log into the `Launch Studio`_ tool and follow the steps for `Qualification process with no required testing`_ option.

Qualification listing
*********************

The `Bluetooth Qualification Listing`_ search lists all qualified designs.
They are added there automatically as soon as the qualification is granted by SIG.

Use the listing search in the following cases:

* To check if your qualification process succeeded.
* To search for existing designs that can be used in your product.
* To check if you can add your device model to an existing qualification entry.

Matter certification by inheritance
***********************************

Bluetooth QDIDs may be further used to obtain the Matter certification by inheritence.
See the :ref:`ug_matter_device_certification_reqs_dependent` section for details.

Support
*******

For details about the qualification process, see the `Qualifications and listings`_ page or ask `Bluetooth Qualification Consultants`_ for advice.

In case of any questions regarding Nordic Semicondutor's qualified designs and their use in your products, contact Nordic Semiconductor's technical support on `DevZone`_.
