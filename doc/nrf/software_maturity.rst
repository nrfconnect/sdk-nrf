.. _software_maturity:

Software maturity levels
########################

.. contents::
   :local:
   :depth: 2

Following are the different software maturity levels shown for the different features:

* Supported - Features that are maintained and are suitable for product development.
* Experimental - Features that can be used for development but not recommended for production.
* Not supported - The feature or component will not work if not supported.

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
     - Customer issues raised for features supported for development on tagged |NCS| releases are handled by technical support on DevZone.
     - Customer issues raised for experimental features on tagged |NCS| releases are handled by technical support on DevZone.
     - Not available.
   * - **Bug fixing**
     - Reported critical bugs will be resolved in both main and latest tagged version.
     - Bug fixes and improvements are not guaranteed to be applied to current versions.
     - Not available.
   * - **Implementation completeness**
     - Complete implementation of the agreed features set.
     - Significant changes may be applied in upcoming versions.
     - A feature or component is available as an implementation, but is not compatible with, or tested on a specific device or series of products.
   * - **API definition**
     - API definition is stable.
     - API definition is not stable and may change before going to production.
     - Not available.
   * - **Maturity**
     - Suitable for integration in end-product development.

       A feature or component that is either fully complete on first commit, or has previously been labelled "experimental" is ready for use in end-product projects.

     - Suitable for prototyping or evaluation.
       Should not be deployed in end-products.

       A feature or component that is in a core supported sub-system but is in development and either:

       * Not fully tested according to existing test plans.
       * In development and changing; not complete.
     - Not applicable.

   * - **Verification**
     - Verified for product development.
     - Incomplete verification
     - Not applicable.

Protocol support
****************

.. sml-table:: top_level

Thread feature support
**********************

.. sml-table:: thread

Matter feature support
**********************
The matter features are still experimental for now:

.. sml-table:: matter
