.. _sample:

Sample template
###############


.. note::
   Sections with * are optional.

The XYZ sample application demonstrates how to blink LEDs in the rhythm of the music that is played.

.. tip::
   Explain what this example demonstrates in one, max two sentences (full sentences, not sentence fragments).
   You can go into more detail in the next section.


Overview
********

You can use this sample application as a starting point to implement a disco light.

The sample applicaton uses the ``:ref:RST link`` library to control the LEDs. In addition, it uses the ``:ref:RST link`` sound sensor and hooks up to some internet service to download cool blinking sequences.

.. tip::
   Continue the explanation on what this example is about.
   What does it show, and why should users try it out?
   What is the practical use? How can users extend this example? What libraries are used?
   When linking, link to the conceptual documentation of other modules - these will in turn link to or contain the API documentation.

Some title*
===========
.. note::
   Add subsections for technical details. Give them a suitable title (sentence style capitalization, thus only the first word capitalized).

The sample application repeatedly calls function ABC, which ...

.. tip::
   Do not state what people can see and understand from looking at the code. Instead, clarify general concepts or explain parts of the implementation that might be unintuitive for some reason. If there's nothing important to point out, do not include any subsections.


Requirements
************

* One of the following development boards:

  * nRF52840 Development Kit board (PCA10056)
  * nRF52 Development Kit board (PCA10040)
  * nRF51 Development Kit board (PCA10028)

* A Shazam account
* A disco ball

.. tip::
   Unless the sample is meant to go upstream, require a Nordic board, not "any board with BLE". Be specific - "any Nordic board" will also mean nRF24 ... When listing the supported boards, give both the name (nRF52 Development Kit board) and the board number (PCA10040).


Wiring*
*******

Connect PIN1 to PIN2, then cut the connection again.

User interface*
***************

Button 1:
   Start or stop the disco show.

.. tip::
   Add Button and LED assignments here, plus other information related to the user interface.

Building and running
********************
.. |sample path| replace:: :file:`samples/XXX`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your board, test it by performing the following steps:

#. Connect to the board with some tool.
#. UART settings.
#. Press a button on the board.
#. Look at the flashing lights.
#. And so on ...


Sample output*
==============

The following output is logged on RTT::

   whatever


Troubleshooting*
================

If the LEDs do not start blinking, check if the music is loud enough.

Dependencies*
*************

This sample uses the following |NCS| libraries:

* :ref:`customservice_readme`

In addition, it uses the following Zephyr libraries:

* ``include/console.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

.. tip::
   If possible, link to the respective library.
   If there is no documentation for the library, include the path.


Known issues and limitations*
*****************************

The sample only works with good music.

References*
***********

* Music chapter in the Bluetooth Spec (-> always link)
* Disco ball datasheet

.. tip::
   Do not include links to documents that are common to all or many of our samples. For example, the Bluetooth Spec or the DK user guides are always important, but shouldn't be listed. Include specific links, like a chapter in the Bluetooth Spec if the sample demonstrates the respective feature, or a link to the hardware pictures in the DK user guide if there is a lot of wiring required, or specific information about the feature that is presented in the sample.
