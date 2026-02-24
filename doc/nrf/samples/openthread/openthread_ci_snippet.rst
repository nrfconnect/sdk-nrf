.. _snippet_ot_ci:

OpenThread snippet for CI purpose (ot-ci)
#########################################

.. contents::
   :local:
   :depth: 2

To build with this snippet, follow the instructions in the :ref:`using-snippets` page.
When building with west, run the following command:

.. code-block:: console

   west build -- -D<project_name>_SNIPPET=ot-ci

Overview
********

This snippet is used in the OpenThread samples to ensure that the samples are tested without the boot banner and shell prompt.
