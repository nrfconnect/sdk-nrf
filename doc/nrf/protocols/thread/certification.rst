.. _ug_thread_cert:

Thread certification
####################

.. contents::
   :local:
   :depth: 2

Thread Group requires certification for devices using the Thread protocol.
You can follow different scenarios to assure that your Nordic Semiconductor device based on the |NCS| becomes a Thread-certified product.

For general information about the certification process, check the `Thread Group's certification information`_.

.. _ug_thread_cert_options:

Thread certification options
****************************

The :ref:`nrfxlib:ot_libs`, which are available in the :ref:`nrfxlib:nrfxlib` repository, are certified by Thread Group for various Nordic Semiconductor devices.

You can find the certification information for the SoC or the SiP you are using in the relevant Compatibility Matrix's "Thread CIDs" section.

Depending on your development approach, you have several certification options when using Nordic Semiconductor devices.

You will need to analyze the :ref:`ug_thread_build_report` to check that there is a proper Thread version and library used for the certification process and that there are no changes that may affect the certification by inheritance.
Additionally, you may be asked to include the build report during the certification process.

.. _ug_thread_cert_inheritance_without_modifications:

Certification by inheritance without modifications to binaries
==============================================================

If you are developing a Thread product with the |NCS|, you can apply for certifying this product by inheritance as an official Thread certified product.
As long as you do not modify the functionality of the certified binary libraries, your product is eligible for certification by inheritance.

This scenario offers the most simplified and potentially the shortest certification process at the lowest cost.
It is available to `Thread Group members`_ at Implementer tier or higher.


Certification by inheritance with modifications to binaries
===========================================================

If your solution uses the same version of the OpenThread stack as the one used in the precompiled, certified binaries, but you made modifications within the operating parameters defined by Thread Group, you need to contact Thread Group to check if certification by inheritance is still possible.

You must officially list all changes with a detailed explanation in your application.
Based on this list, Thread Group can accept or reject your application for certification by inheritance.

Certification at ATL
====================

If your solution modifies the certified functionality of the stack, or if Thread Group rejects your application for certification by inheritance, you need to undergo the full certification procedure at an Authorized Test Lab (ATL).
Only Contributor and Sponsor tier members of Thread Group can apply for certification by an ATL.

Thread Test Harness integration
*******************************

To certify a Thread product, all certification test cases must be run on the Thread Test Harness software.

A detailed description of how to assemble and configure a Thread Test Bed and run the Thread Test Harness can be found on the `Thread Group's Confluence wiki page`_.
Access to this information is only available to members of the Thread Group, and access to the Thread Test Harness depends on the membership tier.

.. note::
   The following procedure references the nRF52840 Development Kit.
   The same procedure can be used to run the certification using other development kits.

Complete the following steps to prepare for the certification tests:

#. Build the certification image.

   Use the :ref:`ot_cli_sample` sample as a base and apply the ``ci`` and ``multiprotocol`` :ref:`app_build_snippets`.
   If you are building for the ``nrf5340dk/nrf5340/cpuapp`` target, also set the additional :makevar:`FILE_SUFFIX` CMake option to ``ble``.
   See :ref:`app_build_file_suffixes` and :ref:`cmake_options` for more information.

   * If building on the command line, use the following command:

     .. code-block::

        cd ncs/nrf/samples/openthread/cli/
        west build -b nrf52840dk/nrf52840 -- -Dcli_SNIPPET="ci;multiprotocol"  -DCONFIG_OPENTHREAD_LIBRARY=y

   * If building using Visual Studio Code, you must first `create and build the application <How to build an application_>`_ using the CLI sample.
     Add the following lines to the **Additional CMake arguments** text field:

     .. code-block::

        -Dcli_SNIPPET="ci;multiprotocol"
        -DCONFIG_OPENTHREAD_LIBRARY=y

   .. note::
      The configuration option selects the precompiled OpenThread libraries.
      The ``multiprotocol`` snippet and the :makevar:`FILE_SUFFIX` CMake option set to ``ble`` enables the :ref:`multiprotocol support <ug_multiprotocol_support>` with Bluetooth® LE advertising.

#. Prepare Thread Test Harness.

   a. Copy all THCI files provided in the :file:`ncs/nrf/samples/openthread/cli/harness-thci/` directory into :file:`C:\\GRL\\Thread1.2\\Thread_Harness\\THCI\\`.

   b. Copy images of your choice to :file:`C:\\GRL\\Thread1.2\\Web\\images\\nRF_Connect_SDK.jpg` and :file:`C:\\GRL\\Thread1.2\\Web\\images\\nRF_Connect_SDK_1_3.jpg`.

     You can use the same image for both.

   c. Edit :file:`C:\\GRL\\Thread1.2\\Web\\data\\deviceInputFields.xml` and prepend the following code:

      .. code-block::

         <DEVICE name="nRF Connect SDK 1.1 1.2" thumbnail="nRF_Connect_SDK.jpg" description = "Nordic Semiconductor: NCS Baudrate:115200" THCI="nRF_Connect_SDK_11_12">
            <ITEM label="Serial Line"
               type="text"
               forParam="SerialPort"
               validation="COM"
               hint="eg: COM1">COM
            </ITEM>
            <ITEM label="Speed"
               type="text"
               forParam="SerialBaudRate"
               validation="baud-rate"
               hint="eg: 115200">115200
            </ITEM>
         </DEVICE>
         <DEVICE name="nRF Connect SDK 1.3 1.4" thumbnail="nRF_Connect_SDK_1_3.jpg" description = "Nordic Semiconductor: NCS Baudrate:115200" THCI="nRF_Connect_SDK_13_14">
            <ITEM label="Serial Line"
               type="text"
               forParam="SerialPort"
               validation="COM"
               hint="eg: COM1">COM
            </ITEM>
            <ITEM label="Speed"
               type="text"
               forParam="SerialBaudRate"
               validation="baud-rate"
               hint="eg: 115200">115200
            </ITEM>
         </DEVICE>

      The device with name "nRF Connect SDK 1.1 1.2" is intended to be used for Thread 1.1 and Thread 1.2 Certification Programs tests.
      The device with name "nRF Connect SDK 1.3 1.4" is intended to be used for Thread 1.3 and Thread 1.4 Certification Programs tests.

See the following links for more information on OpenThread:

- `OpenThread THCI`_
- `OpenThread acting as a new reference platform`_

Thread Test Harness with nRF52840 DK
====================================

Thread Test Harness does not correctly identify the nRF52840 DK (PCA10056) out-of-the-box.

Due to a collision of USB PID:VID with another vendor, Nordic devices are not automatically added to the device list.
This is valid only for Nordic Semiconductor development kits with a J-Link virtual COM port.

To add an nRF52840 DK, drag the nRF52840 DK and drop it on the test bed configuration page.
After that, the device is configured and the :ref:`proper baud rate (115200) <test_and_optimize>` and COM port are set.

.. _ug_thread_build_report:

OpenThread build report
***********************

The OpenThread build report contains information about:

   * The current |NCS| and OpenThread revisions.
   * The Thread feature set and Thread library path.
   * Changes in the :ref:`nrfxlib:nrfxlib` repository in comparison to the latest |NCS| release.

The report is generated to the output console log, and stored as an additional build artefact in the application build directory.

Generating the OpenThread report is enabled by default if the :kconfig:option:`CONFIG_NET_L2_OPENTHREAD` Kconfig option is set to ``y``.
This means that it is enabled for all samples that use the Thread stack.
To disable the generation, set the :kconfig:option:`CONFIG_OPENTHREAD_REPORT` Kconfig option to ``n``.

By default, the build artefact name is set as :file:`ot_report.txt`, but you can specify a different name by setting the :kconfig:option:`CONFIG_OPENTHREAD_REPORT_BUILD_ARTEFACT_NAME` Kconfig value to the new one.

Depending on if you build the application using the :ref:`nrfxlib:ot_libs` or if you build the application and Thread stack from the source files, you will see the following logs in your build console:

.. tabs::

   .. group-tab:: Using pre-built libraries

      .. code-block::

         ################### OPENTHREAD REPORT ###################
         + Target device: nrf54l15
         + Thread version: v1.4
         + OpenThread library feature set: Minimal Thread Device (MTD)
         + Thread device type: Sleepy End Device (SED)
         + OpenThread Library: openthread/lib/nrf54l15_cpuapp/soft-float/v1.4/mtd/
         + OpenThread NCS revision: ncs-thread-reference-20241002-dirty
         + OpenThread NCS SHA: ee86dc26d
         + NCS revision: v2.8.0-preview1-434-g49bcdd3c6d6-dirty
         + NCS SHA: 49bcdd3c6d6
         + No differences in the used Thread library in comparison to the NCS v2.8.0 release.
         ###################        END        ###################

      The generated build artefact will also include the list of the :ref:`nrfxlib:nrfxlib` repository changes between the current revision and the latest |NCS| release.
      The resulting report can be useful when applying for certification by inheritance.
      See :ref:`ug_thread_cert_options` for more details.

      An example of the changes detected in the Thread library:

      .. code-block::

         ################### OPENTHREAD REPORT ###################
         + Target device: nrf54l15
         + Thread version: v1.4
         + OpenThread library feature set: Minimal Thread Device (MTD)
         + Thread device type: Sleepy End Device (SED)
         + OpenThread Library: openthread/lib/nrf54l15_cpuapp/soft-float/v1.4/mtd/
         + OpenThread NCS revision: ncs-thread-reference-20241002-dirty
         + OpenThread NCS SHA: ee86dc26d
         + NCS revision: v2.8.0-preview1-434-g49bcdd3c6d6-dirty
         + NCS SHA: 49bcdd3c6d6
         + Found differences in the nrfxlib repository in comparison to the NCS v2.8.0 release. See the ot_report.txt report file to learn more.
         ###################        END        ###################

      You can look at the report file located in the application build directory to see the full list of changes.
      For example, for the :ref:`ot_cli_sample` sample, you can find a report file in the default location: :file:`cli/build/cli/ot_report.txt`.

   .. group-tab:: From source files

      .. code-block::

         ################### OPENTHREAD REPORT ###################
         + Target device: nrf54l15
         + Thread version: v1.4
         + OpenThread library feature set: Minimal Thread Device (MTD)
         + Thread device type: Sleepy End Device (SED)
         + OpenThread library has been built from sources
         + OpenThread NCS revision: ncs-thread-reference-20241002-dirty
         + OpenThread NCS SHA: ee86dc26d
         + NCS revision: v2.8.0-preview1-434-g49bcdd3c6d6-dirty
         + NCS SHA: 49bcdd3c6d6
         ###################        END        ###################

      The information shows that the Thread library has been build from sources, so it cannot be used for :ref:`ug_thread_cert_inheritance_without_modifications`.

..
