.. _ug_npm1300_developing:

Developing with nPM1300 EK
##########################

.. contents::
   :local:
   :depth: 2

The |NCS| provides support for development with the `nPM1300 <nPM1300 product website_>`_ Power Management IC (PMIC), using the `nPM1300 EK <nPM1300 EK product page_>`_ (PCA10152).

To get started with your nPM1300 EK, follow the steps in the :ref:`ug_npm1300_gs` section.
If you are not familiar with the |NCS| and its development environment, see the :ref:`installation` and :ref:`configuration_and_build` documentation.

Importing an overlay from nPM PowerUP
*************************************

The nPM PowerUP application supports exporting a full configuration of the nPM1300 in devicetree overlay format.
You can use this exported overlay file to quickly configure the nPM1300 in your application.

If there is no overlay file for your project, include the file directly in your application folder with the name
:file:`app.overlay`.

If an overlay already exists, append the contents of the generated overlay to the existing file.

For more information about devicetree overlays, see :ref:`zephyr:use-dt-overlays`.
