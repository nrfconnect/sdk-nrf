.. _library_template:

Template: Library
#################

.. contents::
   :local:
   :depth: 2

.. note::
   Use this template to create pages that describe the libraries included in the |NCS|, following these instructions:

   * Use this first section to give a short description (1 or 2 sentences) of the library, indicating what it does and why it should be used.

   * Give the library a concise name that also corresponds to the folder name.

     If the library targets a specific protocol or device, add it in the title before the library name (for example, "NFC:").
     Do not include the word "library" in the title.
     Use the provided stock phrases and includes when possible.

   * If possible, start the reference tag at the top of the page with ``_lib_``.

   * Sections with ``*`` are optional and can be left out.
     All the other sections are required for all libraries.

     Do not add new sections or remove a mandatory section without consulting a tech writer.
     If any new section is needed, try first to fit it as a subsection of one of the sections listed below.

     When you keep an optional section, remove the asterisk from the title, and format the length of the related RST heading accordingly.

   * Include a tech writer in the PR review.

Overview
********

.. note::
   Use this section to give a general overview of the library.

Implementation*
===============

.. note::
   Use this section to describe the library architecture and implementation.

Supported features*
===================

.. note::
   Use this section to describe the features supported by the library.

Supported backends*
===================

.. note::
   Use this section to describe the backends supported by the library, if applicable.

Requirements*
*************

.. note::
   Use this section to list the requirements needed to use the library.
   It is a mandatory section if there are specific requirements that must be met to use the library.

Configuration
*************

.. note::
   Use this section to describe how to enable the library for a project.
   This information is mandatory.

   You can also use this section to list available library configuration options, if applicable.
   This information is optional.

Shell commands list*
====================

.. note::
   Use this section to list available library shell commands, if applicable.

Usage
*****

.. note::
   Use this section to explain how to use the library.
   This is optional if the library is so small that the initial short description also provides information about how to use the library.

Samples using the library*
**************************

.. note::
   Use this section to list all the |NCS| samples using this library, if present.

The following |NCS| samples use this library:

* Sample 1
* Sample 2
* Sample 3

Application integration*
************************

.. note::
   Use this section to explain how to integrate the library in a custom application.

Additional information*
***********************

.. note::
   Use this section to provide any additional information relevant to the user.

Limitations*
************

.. note::
   Use this section to describe any limitations to the library, if present.

Dependencies
************

.. note::
   Use this section to list all dependencies of this library, if applicable.

API documentation
*****************

.. note::
   Add the following section to use the API documentation:

.. code-block::

   | Header file: :file:`*provide_the_path*`
   | Source files: :file:`*provide_the_path*`

   .. doxygengroup:: *doxygen_group_name*
      :project: *project_name*
      :members:
