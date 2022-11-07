.. _ug_thread_cert:

Thread certification
####################

.. contents::
   :local:
   :depth: 2

Thread Group requires certification for devices using the Thread protocol.
You can follow different scenarios to assure that your Nordic Semiconductor device based on |NCS| becomes a Thread-certified product.

For general information about the certification process, check the `Thread Group's certification information`_.

.. _ug_thread_cert_options:

Thread certification options
****************************

The :ref:`nrfxlib:ot_libs`, which are available in the :ref:`nrfxlib:nrfxlib` repository, are certified by Thread Group for various Nordic Semiconductor devices.

You can find the certification information for the chip that you are using in the `Nordic Semiconductor Infocenter`_.
Navigate to the compatibility matrix for your chip and select *Thread CIDs*.

Depending on your development approach, you have several certification options when using Nordic Semiconductor devices.

Certification by inheritance without modifications to binaries
==============================================================

If you are developing a Thread product with |NCS|, you can apply for certifying this product by inheritance as an official Thread certified product.
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

   Use the :ref:`ot_cli_sample` sample as a base, and apply the :file:`overlay-ci.conf` and :file:`overlay-multiprotocol.conf` overlay files.

   * If building on the command line, use the following command:

     .. code-block::

        cd ncs/nrf/samples/openthread/cli/
        west build -b nrf52840dk_nrf52840 -- -DOVERLAY_CONFIG="overlay-ci.conf;overlay-multiprotocol.conf" -DCONFIG_OPENTHREAD_LIBRARY=y

   * If building using Visual Studio Code, you must first `create and build the application <How to build an application_>`_ using the CLI sample.
     Select the :file:`overlay-ci.conf` and :file:`overlay-multiprotocol.conf` overlay files in the :guilabel:`Kconfig fragment` drop-down menu and add the following lines to the **Additional CMake arguments** text field:

     .. code-block::

        CONFIG_OPENTHREAD_LIBRARY=y

   .. note::
      The configuration option selects the precompiled OpenThread libraries.
      The :file:`overlay-multiprotocol.conf` overlay file enables :ref:`multiprotocol support <ug_multiprotocol_support>` with Bluetooth® LE advertising.

   .. note::
      The :file:`overlay-multiprotocol.conf` overlay file is not supported with ``nrf52833dk_nrf52833``.

#. Prepare Thread Test Harness.

   a. Copy the provided :file:`ncs/modules/lib/openthread/tools/harness-thci/OpenThread.py` file into :file:`C:\\GRL\\Thread1.2\\Thread_Harness\\THCI\\nRF_Connect_SDK.py`.

   b. Copy the provided :file:`ncs/nrf/samples/openthread/cli/harness-thci-1-3/nRF_Connect_SDK_1_3.py` file into :file:`C:\\GRL\\Thread1.2\\Thread_Harness\\THCI\\nRF_Connect_SDK_1_3.py`.

   c. Copy images of your choice to :file:`C:\\GRL\\Thread1.2\\Web\\images\\nRF_Connect_SDK.jpg` and :file:`C:\\GRL\\Thread1.2\\Web\\images\\nRF_Connect_SDK_1_3.jpg`.

     You can use the same image for both.

   d. Edit :file:`C:\\GRL\\Thread1.2\\Thread_Harness\\THCI\\nRF_Connect_SDK.py` as follows:

      .. code-block:: python

            >> Thread Host Controller Interface
            >> Device : OpenThread THCI
            >> Class : OpenThread

      to

      .. code-block:: python

            >> Thread Host Controller Interface
            >> Device : nRF_Connect_SDK THCI
            >> Class : nRF_Connect_SDK

      and

      .. code-block:: python

         class OpenThread(OpenThreadTHCI, IThci):

      to

      .. code-block:: python

         class nRF_Connect_SDK(OpenThreadTHCI, IThci):

   e. Edit :file:`C:\\GRL\\Thread1.2\\Web\\data\\deviceInputFields.xml` and prepend the following code:

      .. code-block::

         <DEVICE name="nRF Connect SDK" thumbnail="nRF_Connect_SDK.jpg" description = "Nordic Semiconductor: NCS Baudrate:115200" THCI="nRF_Connect_SDK">
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
         <DEVICE name="nRF Connect SDK 1.3" thumbnail="nRF_Connect_SDK_1_3.jpg" description = "Nordic Semiconductor: NCS Baudrate:115200" THCI="nRF_Connect_SDK_1_3">
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

      The device with name "nRF Connect SDK" is intended to be used for Thread 1.1 and Thread 1.2 Certification Programs tests.
      The device with name "nRF Connect SDK 1.3" is intended to be used for Thread 1.3 Certification Program tests.

See the following links for more information on OpenThread:

- `OpenThread THCI`_
- `OpenThread acting as a new reference platform`_

Thread Test Harness with nRF52840 DK
====================================

Thread Test Harness does not correctly identify the nRF52840 DK (PCA10056) out-of-the-box.

Due to a collision of USB PID:VID with another vendor, Nordic devices are not automatically added to the device list.
This is valid only for Nordic Semiconductor development kits with a J-Link virtual COM port.

To add an nRF52840 DK, drag the nRF52840 DK and drop it on the test bed configuration page.
After that, the device is configured and the proper baud rate (115200) and COM port are set.
