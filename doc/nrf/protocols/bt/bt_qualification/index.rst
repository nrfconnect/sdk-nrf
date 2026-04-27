.. _ug_bt_qualification:

Bluetooth qualification
#######################

.. contents::
   :local:
   :depth: 2

Every product using *Bluetooth®* technology must be qualified through a process defined by the Bluetooth Special Interest Group (SIG).
When your product is qualified, you will be authorized to do the following:

* Prove that your products satisfy the requirements of the Bluetooth license agreements and specifications.
* Use Bluetooth trademark and logos.
* Sell products that use Bluetooth LE technology.

As a result, your products will be `publicly listed <Qualified products_>`_ and can be promoted on the official Bluetooth SIG page.

Pre-qualified design
********************

A *design* is a specific configuration of hardware or software implementation (or both) of adopted Bluetooth specifications, either in parts or as a whole.
If relevant, you can use an existing design that has already been qualified (called a *pre-qualified design*) and has a Bluetooth Design Number (DN) or, for older qualifications, a Qualification Design ID (QDID) number.
This allows you to speed up your product's qualification process by omitting any additional testing.
You will notice that all valid DNs begin with the letter Q while older QDID numbers do not have a Q prefix to the number.

If your product uses a qualified Bluetooth stack provided as part of the |NCS|, you can inherit this qualification from Nordic Semiconductor, provided that you do not introduce any changes to the stack.

To find the applicable DNs, go to the Compatibility Matrix relevant for the SoC or SiP model you use (for example, the `nRF52840 Compatibility Matrix`_), and open the Bluetooth DNs and QDIDs section.

You can independently qualify any qualified design offered by Nordic Semiconductor by performing all of the required testing yourself.
However, this process might be complex and time-consuming.
Thus, it is recommended that you inherit Nordic Semiconductor's design qualifications in your product's listing.

If you want to have additional Bluetooth profiles (or additional ICS layers) in your product design that are not offered through Nordic Semiconductor's design qualifications, you must implement them and run the required tests through the `Profile Tuning Suite (PTS)`_ tool before qualifying the product.

Requirements
************

Before you start the Bluetooth SIG's Qualification Process, ensure the following:

1. You have access to your company's member account.
   To check if your company is already a SIG member, go to `Join Bluetooth SIG`_ page and search the Member Directory list.

#. Get acquainted with the `Qualification Program Reference Document`_ (QPRD) and Declaration Process Document (DPD) to understand the basics of the qualification process.
   Log into your company's member account to review both the QPRD and DPD documents.

Qualification process
*********************

Although you can make use of Nordic's pre-qualified designs, you need to perform online paperwork necessary for initiating the Bluetooth Product qualification of your product yourself.
Hence, you must complete the qualification process for all of your products under your company name and account.

You can either perform a full qualification process for a new product or edit an existing entry of a product that has already been qualified by your company.
See the following sections for more information about both possibilities.

.. note::

   Each new qualification listing requires a fee from the Bluetooth SIG for the DN.
   At the top of the main page for the Qualification Workspace tool, you will find a link marked **Pay Product Qualification Fee**.
   You can pay the fee for a receipt number using invoice or credit card.
   Your company must have at least one available receipt number to complete a new product listing.

Creating a new product listing
==============================

Log into the Qualification Workspace (found at the `Bluetooth SIG's Qualification Process`_ page), the official qualification tool , that will guide you through one of the following processes depending on the state of your product:

* Qualification Option 2a - Create a new design that combines multiple existing designs.
  Complete it if your product uses only pre-qualified DNs and implements APIs as instructed in the corresponding |NCS| documentation.
  Refer to the `QPRD section 3.2.2.1`_ for Option 2a.
  This is the most common and straightforward path for Nordic customers.

* Qualification Option 2b - Create any other new design.
  Complete it in the following cases:

  * If your product uses a new Bluetooth design that is not yet qualified by Nordic Semiconductor.
  * If your product alters a previously qualified Bluetooth design by changing the code or configuring it in a way that can break the qualification requirements for the Nordic Semiconductor's qualified design.
    Refer to the `QPRD section 3.2.2.2`_ for Option 2b.

For certain DNs Nordic creates design "subsets" that contain a reduced set of features from the main design.
You can combine and use these in the same manner as the DNs from the main design.
As an example, the Nordic nRF54L15 SoC might have a "main" DN that includes the Channel Sounding feature and a subset that removes that feature for products that do not require it.
Choose the DN that most closely matches your product features and requirements.

.. note::
   It is not a problem to have "unused" features in your design.
   For example, if your referenced Nordic controller design supports the 2M PHY feature, but your product does not require it, that is not a concern.
   You do not have to use every feature claimed in the product's qualification listing, but conversely you *must* claim every feature that you use.
   You can later add products in the same design listing (without paying any extra fee) that uses a previously unused feature from inherited design.

Updating an existing product listing
====================================

.. note::
   An existing product listing can be edited only by the person that completed the listing.
   If you want to change the listing owner, contact SIG support.

Update an existing qualification listing through the Qualification Workspace tool (found at `Bluetooth SIG's Qualification Process`_ page) in the following cases:

* You changed an existing product's design with no changes in the product's features, hence you want to update the listing to indicate new updates or add new marketing information regarding this product.
* You can add a new product as a new entry to an existing list of product entries as there was no design change in the new product.
  The new product may or may not use previously unused features from an inherited design.
  For example, if there was a new model number or variant being sold.
* You want to switch to another major |NCS| version for an existing product.
* In some cases, if you want to switch to another minor |NCS| version for an existing product.
* You need to alter the configuration of the qualified design or slightly modify parts of the existing design to better fit your application needs.

If you make such changes, ensure that the resulting design does not change the Implementation Conformance Statement (ICS) that has already been declared for a relevant Nordic Semiconductor's qualified design.
If there is a change required to the Implementation Conformance Statement, where a new feature must be added or selected, this triggers the need for a new qualification listing.

Switching to another SDK version
--------------------------------

If you want to switch to another |NCS| version for an existing product, complete the following steps:

1. Go to the Compatibility Matrix relevant for the SoC or SiP model you use.

#. Navigate to the Bluetooth DNs section and, based on the table, confirm the following:

   * The version you want to use must be compatible with versions of the other components you want to keep for your product.
   * Relevant Host and SoftDevice Controller components implemented in the |NCS| version you want to use must be pre-qualified.
     If both are pre-qualified, check the following:

     * If the version you want to use is fully pre-qualified and has the same DN numbers as the ones currently used for your product, no further action is required.
     * If the version you want to use has at least one QDID number that is different from the numbers already used for your product, a new qualification is required.
     * Log into the Qualification Workspace tool (found at the `Bluetooth SIG's Qualification Process`_ page) and follow the steps for the Qualification Option 2a path.

Qualification listing
*********************

The **Manage Submitted Products** option on the main page of Qualification Workspace lists all the qualified designs from your company.
New listings are added there automatically when the qualification is granted by SIG.
Your completed listing may be visible to you, before it is publicly viewable.
If the SIG performs a validation of the submission, it might take a few days before the qualification is approved and publicly available.
You might also request that the design *not* be publicly viewable for a period of up to six months from the qualification completion.

You can use the listing search for the following purposes:

* Check if your qualification process succeeded and is publicly viewable.
* Search for existing designs that can be used in your product.
* Check if you can add your device model to an existing qualification entry.

Bluetooth QDID inheritance in Matter certification
**************************************************

When applying for Matter certification, you must present a self-attestation that confirms you have applied for and obtained the certification for the transport platform you are using for your Matter component.
If your product uses a qualified Bluetooth stack provided as part of the |NCS|, you can inherit this certification from Nordic Semiconductor, provided that you do not introduce any changes to the stack.

See the :ref:`ug_matter_device_certification_reqs_dependent` section for details.

Support
*******

For more details about getting started with the qualification process, see the `Qualification Quick Start Guide`_ page or work with `Bluetooth Qualification Consultants`_ for advice.

In case of any questions regarding Nordic Semiconductor's qualified designs and their use in your products, contact Nordic Semiconductor's technical support on `DevZone`_.
