.. _Ecosystem_integration_template:

Template: Ecosystem integration
###############################

.. contents::
   :local:
   :depth: 2

.. note::
   Provide a short introductory description of the tool or service (third-party), indicating the value it brings to |NCS|.
   Refer to the following instructions to create the description:

   * Provide a title that starts with the tool or service name with "integration" as a suffix, for example, "Memfault integration".
   * Place the documentation inside the :file:`nrf/doc/nrf` folder.
   * List the file name with path in the ``.. toctree:`` of the :file:`ecosystem_integration` RST file.
   * Sections with * are optional and can be left out.
     All other sections are required.
     There could be more sections, which can be added after discussion with the tech writer team.
     However, it should have the mandatory sections from the template in the recommended format at the very least.

.. tip::
   Describe the tool or service in one or two sentences maximum (full sentences, not sentence fragments), so that the user gets a clear idea of what the tool or service does.
   This introduction must be concise and clear, highlighting the features of the tool or service that it brings to |NCS|.

Partner overview*
*****************

.. note::
   Provide an explanation of the tool or service.
   This is optional if the tool or service is described above.

Prerequisites
*************

.. note::
   * Provide prerequisites that must be completed before the integration with the tool or service.
   * Mention any other tool or service integration that needs to be completed.

Before you start the |NCS| integration with AVSystem's Coiote IoT Device Management, make sure that the following prerequisites are completed:

   * :ref:`Setup of nRF Connect SDK <getting_started>` environment.
   * :ref:`Setup of nRF9160 DK <ug_nrf9160_gs>`.
   * :ref:`Create an nRF Cloud account <creating_cloud_account>`.
   * Create an account for `AVSystem Coiote Device Management <Coiote Device Management server_>`_.

Solution architecture
*********************

.. note::
   * Provide a short description of :term:`Development Kit (DK)` and |NCS| interaction with the tool or service and the key elements, for example, connectivity and onboarding.
   * You can include a diagram showing the interaction.

Integration overview
********************

.. note::
   * Explain in more detail how the integration of |NCS| with the tool or service works.
   * Try to explain the integration in steps.
     If possible, divide it into sections and provide a brief explanation in the overview with steps.
     For an example, see the :ref:`ug_integrating_fast_pair` section of the :ref:`ug_bt_fast_pair` documentation.

.. tip::
   * You can list the configuration that must be enabled for the integration to work (if applicable).
   * You can add information about overlay configuration files and how they are specified in the build system to enable specific features (if applicable).
     Following is an example for the overlay details for Memfault integration:

     .. code-block:: console

        west build -b nrf9160dk_nrf9160_ns -- -DOVERLAY_CONFIG=overlay-memfault.conf

Some title*
===========

.. note::
   Add optional subsections for technical details.
   Give suitable titles (sentence style capitalization, thus only the first word capitalized).
   If there is nothing important to point out, do not include any subsections.

Applications and samples
************************

.. note::
   Add the details about applications and samples that use or implement the tool or service.

The following application uses the Memfault integration in |NCS|:

* :ref:`asset_tracker_v2`

The following samples demonstrate the Memfault integration in |NCS|:

* :ref:`peripheral_mds`
* :ref:`memfault_sample`

Library support
***************

.. note::
   * Add details about libraries that support the tool or service.
   * If there is no documentation for libraries, include the path.

Required scripts*
*****************

.. note::
   * Add details about scripts that are required for the tool or service integration.
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
   * Describe licensing aspects of the tool or service and provide information on what is available to Nordic Semiconductor customers for development.
   * Refer to the third-party documentation or contact points.

Dependencies*
*************

.. note::
   * Use this section to list all dependencies, like other tool or service references, certification requirements (if applicable).
   * Do not duplicate the dependencies that have been mentioned in other sections.
   * If possible, link to the respective dependencies.
