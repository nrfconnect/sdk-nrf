.. |matter_name| replace:: Closure
.. |matter_type| replace:: sample
.. |matter_dks_thread| replace:: ``nrf52840dk/nrf52840``, ``nrf5340dk/nrf5340/cpuapp``, ``nrf54l15dk/nrf54l15/cpuapp``, and ``nrf54lm20dk/nrf54lm20a/cpuapp`` board targets
.. |matter_dks_internal| replace:: nRF54LM20 DK
.. |sample path| replace:: :file:`samples/matter/closure`
.. |matter_qr_code_payload| replace:: MT:Y.K9042C00KA0648G00
.. |matter_pairing_code| replace:: 34970112332
.. |matter_qr_code_image| image:: /images/matter_qr_code_closure.png

.. include:: /includes/matter/shortcuts.txt

.. _matter_closure_sample:

Matter: Closure
###############

.. contents::
   :local:
   :depth: 2

This sample demonstrates the usage of the :ref:`Matter <ug_matter>` application layer to build a closure device.

.. include:: /includes/matter/introduction/no_sleep_thread_ftd.txt

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/matter/requirements/thread.txt

Overview
********

Closure is a Matter device type that represents a generic device capable of sealing an opening (such as a window, door, and similar structures).
After the device is commissioned, it operates as a Closure device and identifies itself as a garage door through its ``Descriptor`` cluster.

This sample implements the following features of the closure device:

* Positioning (Indicates that the closure can be set to discrete positions)
* Speed (Indicates that the closure supports configurable speed during motion toward a target position)
* Ventilation (Indicates that the closure can be set to a designated ventilation position, for example, partially open)

.. note::

   At least one of the Positioning or Motion Latching features must be supported.
   To add support for additional features, enable them in the ``Init()`` function of the :file:`closure_control_endpoint.cpp` file and implement the corresponding functionality in the :file:`closure_manager.cpp` file.

It implements the following attributes from the Closure Control cluster:

* ``CountdownTime`` (Time left before the movement is finished)
* ``MainState`` (The general state of the device for example Moving or Stopped)
* ``OverallCurrentState`` (The current state of the device including speed and position)
* ``OverallTargetState`` (The target state of the device including speed and position)

And allows for invoking the following commands:

* ``MoveTo`` (Set the desired position and speed)
* ``Stop`` (Stop the current movement of the closure)

See `User interface`_ for information about LEDs and buttons.

Configuration
*************

.. include:: /includes/matter/configuration/intro.txt

The |matter_type| supports the following build configurations:

.. include:: /includes/matter/configuration/basic_internal.txt

Advanced configuration options
==============================

.. include:: /includes/matter/configuration/advanced/intro.txt
.. include:: /includes/matter/configuration/advanced/dfu.txt
.. include:: /includes/matter/configuration/advanced/fem.txt
.. include:: /includes/matter/configuration/advanced/factory_data.txt
.. include:: /includes/matter/configuration/advanced/custom_board.txt
.. include:: /includes/matter/configuration/advanced/internal_memory.txt

User interface
**************

.. include:: /includes/matter/interface/intro.txt

First LED:
   .. include:: /includes/matter/interface/state_led.txt

Second LED:
   Shows the current state of the closure.
   (Off means fully open while on means fully closed.
   The values in between are represented by dimmed light.)

First Button:
   .. include:: /includes/matter/interface/main_button.txt

.. include:: /includes/matter/interface/segger_usb.txt

.. include:: /includes/matter/interface/nfc.txt

Building and running
********************

.. include:: /includes/matter/building_and_running/intro.txt
.. include:: /includes/matter/building_and_running/select_configuration.txt
.. include:: /includes/matter/building_and_running/commissioning.txt

|matter_ble_advertising_auto|

.. include:: /includes/matter/building_and_running/onboarding.txt

Testing
*******

.. |endpoint_name| replace:: **Closure Control cluster**
.. |endpoint_id| replace:: **1**

.. include:: /includes/matter/testing/intro.txt
.. include:: /includes/matter/testing/prerequisites.txt
.. include:: /includes/matter/testing/prepare.txt

Testing steps
=============

1. Enable CHIP Tool interactive mode by running the following command:

   .. code-block:: console

      ./chip-tool interactive start

#. Using the interactive mode, subscribe to the ``MovementCompleted`` event with the following command:

   .. parsed-literal::
      :class: highlight

      closurecontrol subscribe-event movement-completed 1 5 <node_id> 1

#. Set the target position of the closure with the following command:

   .. parsed-literal::
      :class: highlight

      closurecontrol move-to <node_id> 1 --Position 0 --Speed 0 --timedInteractionTimeoutMs 5000

   Where:

      * *<position>* is of type ``CurrentPositionEnum`` (integer from 0 to 5 ``0`` for ``FullyClosed``, ``1`` for ``FullyOpened``)
      * *<speed>* is of type ``ThreeLevelAutoEnum`` (``0`` for auto, ``1`` to ``3`` for ``Low`` to ``High``)

   The |Second LED| starts to glow brighter, indicating the closing of the closure.

#. Wait for the closure to finish its movement.
   You should be notified with the ``MovementCompleted`` event after the movement is complete.

#. Test the remaining functionalities in a similar manner

   * Read current state of the device by reading the attributes ``MainState``, ``OverallCurrentState``, and ``OverallTargetState``:

   .. parsed-literal::
      :class: highlight

      closurecontrol read-attribute main-state <node_id> 1
      closurecontrol read-attribute overall-current-state <node_id> 1
      closurecontrol read-attribute overall-target-state <node_id> 1

   * Call ``MoveTo`` command to move the closure to a different position and stop the closure by invoking the ``Stop`` command while the closure is moving:

   .. parsed-literal::
      :class: highlight

      closurecontrol move-to <node_id> <endpoint_id> --Position 0 --Speed 0 --timedInteractionTimeoutMs 5000
      closurecontrol stop <node_id> <endpoint_id>

   * Get the estimated movement time by reading the ``CountdownTime`` attribute.

   .. parsed-literal::
      :class: highlight

      closurecontrol read-attribute countdown-time <node_id> 1

Factory reset
=============

|matter_factory_reset|

Dependencies
************

.. include:: /includes/matter/dependencies.txt
