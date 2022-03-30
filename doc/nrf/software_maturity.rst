.. _software_maturity:

Software maturity levels
########################

.. contents::
   :local:
   :depth: 2

Following are the different software maturity levels shown for the different features:

* Supported - Features that are maintained and are suitable for product development.
* Experimental - Features that can be used for development but not recommended for production.

   * Feature is incomplete in functionality or verification, and can be expected to change in future releases. The feature is made available in its current state though the design and interfaces can change between release tags. The feature will also be labelled with "EXPERIMENTAL" in Kconfig files to indicate this status. Build warnings will be generated to indicate when features labelled EXPERIMENTAL are included in builds unless the Kconfig option :kconfig:option:`CONFIG_WARN_EXPERIMENTAL` is disabled.

* Not supported - The feature or component will not work if not supported.


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
