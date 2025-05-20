.. _dm_licenses:

Licenses
########

.. contents::
   :local:
   :depth: 2

.. licenses_start

Licenses are located close to the source files.
You can find a :file:`LICENSE` file, containing the details of the license, at the top of every |NCS| repository.
Each file included in the repositories also has an `SPDX identifier`_ that mentions this license.

.. licenses_end

Open-source licenses
********************

If a folder or set of files is open source and included in |NCS| under its own license (for example, any of the Apache or MIT licenses), it will have either its own :file:`LICENSE` file included in the folder or the license information embedded inside the source files themselves.

License report
**************

You can use the west :ref:`ncs-sbom <west_sbom>` utility to generate a license report.
It allows you to generate a report for the |NCS|, built application, or specific files.
The tool is highly configurable.
It uses several detection methods, such as:

 * Search based on SPDX tags.
 * Search license information in files.
 * The `Scancode-Toolkit`_.

Depending on your configuration, the report is generated in HTML or SPDX, or in both formats.
See the :ref:`west_sbom` script documentation for more information.
