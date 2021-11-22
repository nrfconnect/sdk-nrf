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

     If the library targets a specific protocol or device, add it in the title before the library name (for example, "NFC:" or "nRF9160:").
     Do not include the word "library" in the title.
     Use the provided stock phrases and includes when possible.

   * If possible, start the reference tag at the top of the page with ``_lib_``.

   * Sections with ``*`` are optional and can be left out.
     All the other sections are required for all libraries.

     Do not add new sections or remove a mandatory section without consulting a tech writer.
     If any new section is needed, try first to fit it as a subsection of one of the sections listed below.

     When you keep an optional section, remove the asterisk from the title, and format the length of the related RST heading accordingly.

   * Include a tech writer in the PR review.

Overview*
*********

.. note::
   Use this section to give a general overview of the library.
   This is optional if the library is so small the initial short description also gives an overview of the library.

Implementation
==============

.. note::
   Use this section to describe the library architecture and implementation.

Supported features
==================

.. note::
   Use this section to describe the features supported by the library.

Supported backends*
===================

.. note::
   Use this section to describe the backends supported by the library, if applicable.
   This is optional.

Requirements*
*************

.. note::
   Use this section to list the requirements needed to use the library.
   This is not required if there are no specific requirements to use the library. It is a mandatory section if there are specific requirements that have to be met to use the library.

Configuration*
**************

.. note::
   Use this section to list available library configuration options, if applicable.
   This is optional.

Shell commands list*
====================

.. note::
   Use this section to list available library shell commands, if applicable.
   This is optional.

Usage*
******

.. note::
   Use this section to explain how to use the library.
   This is optional if the library is so small the initial short description also gives information about how to use the library.

Samples using the library*
**************************

.. note::
   Use this section to list all |NCS| samples using this library, if present.
   This is optional.

The following |NCS| samples use this library:

* Sample 1
* Sample 2
* Sample 3

Application integration*
************************

.. note::
   Use this section to explain how to integrate the library in a custom application.
   This is optional.

Additional information*
***********************

.. note::
   Use this section to describe any additional information relevant to the user.
   This is optional.

Limitations*
************

.. note::
   Use this section to describe any limitations to the library, if present.
   This is optional.

Dependencies
************

.. note::
   Use this section to list all dependencies of this library, if applicable.

API documentation
*****************

.. note::
   Use this section to call the API documentation as follows:

   .. code-block::

      | Header file: :file:`*indicate_the_path*`
      | Source files: :file:`*indicate_the_path*`

      .. doxygengroup:: *indicate_the_doxygroup_if_needed*
         :project: *indicate_the_project_name_if_needed*
         :members:
