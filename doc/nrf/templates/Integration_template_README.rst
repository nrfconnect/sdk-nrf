.. _integration_template:

Template: Integration
#####################

.. contents::
   :local:
   :depth: 2

.. note::
   Provide a short introductory description of the product or service, indicating the value it brings to |NCS|.
   Refer to the following instructions to create the description:

   * Provide a title that starts with the product or service name with "integration" as a suffix, for example, "Memfault integration".
   * Place the documentation inside the :file:`nrf/doc/nrf` folder.
   * List the file name with path in the ``.. toctree:`` of the :file:`integrations` RST file.
   * Sections with * are optional and can be left out.
     All other sections are required.
     There could be more sections, which can be added after discussion with the tech writer team.
     However, it should have the mandatory sections from the template in the recommended format at the very least.

.. tip::
   Describe the product or service briefly (full sentences, not sentence fragments), so that the user gets a clear idea of what the product or service does.
   This introduction must be concise and clear, highlighting the features of the product or service that it brings to |NCS|.

Edge Impulse is a development platform that can be used to enable embedded machine learning on |NCS| devices.
You can use this platform to collect data from sensors, train machine learning model, and then deploy it to your Nordic Semiconductor device.

Product overview*
*****************

.. note::
   Provide an explanation of the product.
   This is optional if the product is described above.

XYZ integration provides the following features to |NCS|:

* Feature 1 - Feature description
* Feature 2 - Feature description

Integration prerequisites
*************************

.. note::
   * Provide prerequisites that must be completed before the integration with the product or service.
   * Mention any other product or service setup that needs to be completed.

Before you start the |NCS| integration with AVSystem's Coiote IoT Device Management, make sure that the following prerequisites are completed:

* :ref:`Install the nRF Connect SDK <installation>`.
* :ref:`Setup of an nRF91 Series DK <ug_nrf91>`.
* :ref:`Create an nRF Cloud account <creating_cloud_account>`.
* Create an account for `AVSystem Coiote Device Management <Coiote Device Management server_>`_.

Solution architecture
*********************

.. note::
   * Provide a short description of how the various components of the integration interact with each other.
   * Mention the :term:`Development Kit (DK)` used for integration, how it interacts with the product or service, and what the key elements are, for example connectivity and onboarding.
   * Include a diagram showing the interaction, if possible.

Integration overview*
*********************

.. note::
   Explain in more detail how the integration of |NCS| with the product or service works.
   This is optional if the integration details are covered in the integration steps.

Integration steps
*****************

.. note::
   * Explain the integration in steps.
     For an example, see the :ref:`ug_integrating_fast_pair` section of the :ref:`ug_bt_fast_pair` documentation.

.. tip::
   * You can list the configuration that must be enabled for the integration to work (if applicable).
   * You can add information about overlay configuration files and how they are specified in the build system using |VSC| or command line to enable specific features (if applicable).

Some title*
***********

.. note::
   Add optional sections for other technical details about the integration (for example, user-defined configuration options).
   Give suitable titles (sentence style capitalization, thus only the first word capitalized).
   If there is nothing important to point out, you need not include any such section.

Applications and samples
************************

.. note::
   Add details about applications and samples that use or implement the product or service.

The following application uses the Memfault integration in |NCS|:

* :ref:`asset_tracker_v2`

The following samples demonstrate the Memfault integration in |NCS|:

* :ref:`peripheral_mds`
* :ref:`memfault_sample`

Library support
***************

.. note::
   * Add details about libraries that support the product or service.
   * If there is no documentation for libraries, include the path.

Required scripts*
*****************

.. note::
   * Add details about scripts that are required for the product or service integration.
   * If there is no documentation for scripts, include the path.

References*
***********

.. note::
   Provide a link to other relevant documentation for more information.

.. tip::
   Do not duplicate links that have been mentioned in other sections before.

Terms and licensing*
********************

.. note::
   * Describe licensing aspects of the product or service and provide information on what is available to Nordic Semiconductor customers for development.
   * Refer to the third-party documentation or contact points.

Dependencies*
*************

.. note::
   * Use this section to list all dependencies, like other product or service references, certification requirements (if applicable).
   * Do not duplicate the dependencies that have been mentioned in other sections.
   * If possible, link to the respective dependencies.
