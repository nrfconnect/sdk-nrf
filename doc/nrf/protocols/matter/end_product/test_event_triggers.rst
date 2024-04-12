.. _ug_matter_test_event_triggers:

Matter test event triggers
##########################

.. contents::
   :local:
   :depth: 2

Matter test event triggers are a set of software commands designed to initiate specific test scenarios.
You can activate these commands using the Matter Controller, which serves as the interface for executing them.
To activate a command, you need to enter the pre-defined activation codes that correspond to the desired test event.
All test event triggers are based on the ``generaldiagnostics`` Matter cluster, and utilize the ``test-event-trigger`` command to function.
This mechanism can be used during the Matter certification process to simulate common and specific device scenarios, and during software verification events to validate the correct operation of Matter-compliant devices.
To learn how to use the test event triggers functionality during the Matter certification process, see the :ref:`matter_test_event_triggers_matter_certification` section.

You can utilize the test event triggers predefined in the :ref:`matter_test_event_triggers_default_test_event_triggers` section to streamline your testing process.
If the predefined options do not meet your requirements, you can also define your own event triggers.
To learn how to create custom event triggers, refer to the :ref:`matter_test_event_triggers_custom_triggers` section of the documentation.

Requirements
************

The :ref:`CHIP Tool application <ug_matter_gs_tools_chip>` is required to send ``test-event-trigger`` commands to the general diagnostics cluster.
Ensure that it is installed before initiating Matter testing events.

Your device needs to includes the general diagnostics cluster within its Matter data model database to perform Matter testing events.
For guidance on configuring the general diagnostics cluster on your device, refer to the instructions provided on the :ref:`ug_matter_gs_adding_cluster` page.

Activation of test event codes requires the provisioning of an enable key.
To ensure secure operation, the enable key must be specific for each device.
You can find instructions on how to establish the enable key for your device in the :ref:`matter_test_event_triggers_setting_enable_key` section.

.. _matter_test_event_triggers_default_test_event_triggers:

Default test event triggers
***************************

You can use the pre-defined common test event triggers in your application.
To disable them, set the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_REGISTER_DEFAULTS` Kconfig option to ``n``.

The following table lists the available triggers and their activation codes:

.. list-table:: Default test event triggers
  :widths: auto
  :header-rows: 1

  * - Name
    - Requirements
    - Description
    - Activation code [range]
    - Value description
  * - Factory reset
    - None
    - Perform a factory reset of the device with a delay.
    - `0xF000000000000000` - `0xF00000000000FFFF`
    - The range of `0x0000` - `0xFFFF` represents the delay in ms to wait until the factory reset occurs.
      The maximum time delay is UINT32_MAX ms.
      The value is provided in HEX format.
  * - Reboot
    - None
    - Reboot the device.
    - `0xF000000000000000` - `0xF00000000000FFFF`
    - The range of `0x0000` - `0xFFFF` represents the delay in ms to wait until the reboot occurs.
      The maximum time delay is UINT32_MAX ms.
      The value is provided in HEX format.
  * - OTA query
    - :kconfig:option:`CONFIG_CHIP_OTA_REQUESTOR` = ``y``
    - Trigger an OTA firmware update.
    - `0x002a000000000100` - `0x01000000000001FF`
    - The range of `0x00` - `0xFF` is the fabric index value.
      The maximum fabric index value depends on the current device's settings.
  * - Door lock jammed
    - :kconfig:option:`CONFIG_CHIP_DEVICE_PRODUCT_ID` = ``32774``
    - Simulate the jammed lock state.
    - `0x3277400000000000`
    - This activation code does not contain any value.

.. _matter_test_event_triggers_setting_enable_key:

Setting the enable key
**********************

The enable key can be provided either by utilizing the factory data, or directly in the application code.

Using factory data
==================

You can not set the enable key to a specific value using factory data unless the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA` Kconfig option is set to ``y``.
If it is not set, the default value ``00112233445566778899AABBCCDDEEFF`` will be used.
For secure operation, you need to ensure that the enable key is unique for all of your devices.

To specify the enable key through the build system, enable the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_BUILD` Kconfig option by setting it to ``y``.
Then, set the :kconfig:option:`CONFIG_CHIP_DEVICE_ENABLE_KEY` Kconfig option to a 32-byte hexadecimal string value.

If :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_BUILD` is set to ``n``, you can follow the :doc:`matter:nrfconnect_factory_data_configuration` guide in the Matter documentation to generate the factory data set with the specific enable key value.

If you do not use the |NCS| Matter common module, you need to read the enable key value manually from the factory data set and provide it to the ``TestEventTrigger`` class.

For example:

.. code-block:: c++

    /* Prepare the factory data provider */
    static chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::InternalFlashFactoryData> sFactoryDataProvider;
    sFactoryDataProvider.Init();

    /* Prepare the buffer for enable key data */
    uint8_t enableKeyData[chip::TestEventTriggerDelegate::kEnableKeyLength] = {};
    MutableByteSpan enableKey(enableKeyData);

    /* Load the enable key value from the factory data */
    sFactoryDataProvider.GetEnableKey(enableKey);

    /* Call SetEnableKey method to load the read value to the TestEventTrigger class. */
    Nrf::Matter::TestEventTrigger::Instance().SetEnableKey(enableKey);

Directly in the code
====================

Use the SetEnableKey method of the ``TestEventTrigger`` class to set up the enable key.

For example:

.. code-block:: c++

    /* Prepare Buffer for Test Event Trigger data which contains your "enable key" */
    uint8_t enableKeyData[chip::TestEventTriggerDelegate::kEnableKeyLength] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
        0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb,
        0xcc, 0xdd, 0xee, 0xff
    };

    /* Call SetEnableKey method to load the prepared value to the TestEventTrigger class. */
    Nrf::Matter::TestEventTrigger::Instance().SetEnableKey(ByteSpan {enableKeyData});

.. _matter_test_event_triggers_matter_certification:

Using in Matter certification
*****************************

When you provide the enable key using factory data, you can utilize the event trigger feature during the Matter certification process.
This is because, when done this way, you can turn off the event trigger functionality by disabling access to the ``generaldiagnostics`` cluster without altering the code that has already been certified.

Once the certification process is complete, you must deactivate the test event trigger functionality by generating new factory data with a modified enable key value.
This is done by setting the :kconfig:option:`CONFIG_CHIP_DEVICE_ENABLE_KEY` Kconfig option to a value consisting solely of zeros.

For instance, to generate factory data with disabled event trigger functionality, set the :kconfig:option:`CONFIG_CHIP_DEVICE_ENABLE_KEY` Kconfig option to the value ``0x00000000000000000000000000000000``.
After generating it, flash the :file:`factory_data.hex` file onto the device.

.. _matter_test_event_triggers_custom_triggers:

Custom test event triggers
**************************

You can define and register custom test event triggers to initiate specific actions on your device.

An activation code is 64 bits in length, and consist of two components: the event trigger ID and the event trigger value.

* The event trigger ID is 32 bits long and uniquely identifies the trigger.
  It is supplied as the first 32 bits of the activation code.
* The event trigger value is specific to a given trigger.
  It is supplied as the last 32 bits of the activation code.

This means that the activation code has the pattern ``0xIIIIIIIIVVVVVVVV``, where ``I`` represents the ID part and ``V`` represents the value part.

For example the ``0x1000000000001234`` activation code stands for a trigger ID equal to ``0x1000000000000000`` and a specific value of ``0x1234``.

A new event trigger consists of two fields: ``Mask``, and ``Callback``.

* The ``Mask`` field is 32 bits long and specifies a mask for the trigger's value.
* The ``Callback`` field is a callback function that will be invoked when the device receives a corresponding activation code.

The maximum number of event triggers that can be registered is configurable.
To adjust this limit, set the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_MAX` Kconfig option to the desired value.

To register a new test event trigger, follow these steps:

1. Create a function that will be executed when the device receives a valid enable key and activation code.

   This function must return a ``CHIP_ERROR`` code and accept a ``Nrf::Matter::TestEventTrigger::TriggerValue`` as its argument.
   You can utilize the provided argument within this function as needed.

   Use ``CHIP_ERROR`` codes to communicate appropriate responses to the Matter Controller.
   For instance, you might return ``CHIP_ERROR_INVALID_ARGUMENT`` to indicate that the user has provided an incorrect value argument.

   .. note::

     The callback method is not thread-safe.
     Ensure that all operations within it are executed in a thread-safe manner.
     To perform operations within the Matter stack context, use the ``chip::DeviceLayer::SystemLayer().ScheduleLambda`` method.
     For operations in the application context, use the ``Nrf::PostTask`` function.

   Here is an example of how to create a callback function:

   .. code-block:: c++

     CHIP_ERROR MyFunctionCallback(Nrf::Matter::TestEventTrigger::TriggerValue value)
     {
        /* Define the required behavior of the device here. */

        return CHIP_NO_ERROR;
     }

#. Register the new event trigger.

   Use the following example as a guide to register a new event:

   .. code-block:: c++

     /* Create a new event */
     Nrf::Matter::TestEventTrigger::EventTrigger myEventTrigger;

     /* Assign all fields */
     uint64_t myTriggerID = /* Set the trigger ID */
     myEventTrigger.Mask = /* Fill the value mask filed */;
     myEventTrigger.Callback = MyFunctionCallback;

     /* Register the new event */
     CHIP_ERROR err = Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTrigger(myTriggerID, myEventTrigger);

     /* Remember to check the CHIP_ERROR return code */

   If the returning `CHIP_ERROR` code is equal to `CHIP_ERROR_NO_MEMORY`, you need to increase the :kconfig:option:`NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_MAX` kconfig option to the higher value.

   Here's an example to handle the ``0xF000F00000001234`` activation code, where 1234 is the event trigger value field:

   .. code-block:: c++

     Nrf::Matter::TestEventTrigger::EventTrigger myEventTrigger;
     uint64_t myTriggerID = 0xF000F0000000;
     myEventTrigger.Mask = 0xFFFF;
     myEventTrigger.Callback = MyFunctionCallback;

     CHIP_ERROR err = Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTrigger(myTriggerID, myEventTrigger);

Register an existing test event trigger handler
***********************************************

The Matter SDK has some test event trigger handlers implemented.
All of them inherit the `TestEventTriggerHandler` class, and are implemented in various places in the Matter SDK.

The events declared in existing test event triggers can have a different semantic than described in the :ref:`matter_test_event_triggers_custom_triggers` section.

Use the following example as a guide to register an existing event trigger handler:

.. code-block:: c++

  /* Create the Trigger Handler object */
  static TestEventTriggerHandler existingTriggerHandler;
  CHIP_ERROR err = Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTriggerHandler(&existingTriggerHandler);

  /* Remember to check the CHIP_ERROR return code */

If the returning ``CHIP_ERROR`` code is equal to ``CHIP_ERROR_NO_MEMORY``, you need to increase the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS_MAX_TRIGGERS_DELEGATES` kconfig option to the higher value.

For example, you can register and use the ``OTATestEventTriggerHandler`` handler and trigger pre-defined Matter OTA DFU behaviors using the following code:

.. code-block:: c++

  /* Create the Trigger Handler object */
  static chip::OTATestEventTriggerHandler otaTestEventTrigger;
  ReturnErrorOnFailure(Nrf::Matter::TestEventTrigger::Instance().RegisterTestEventTriggerHandler(&otaTestEventTrigger));

Usage
*****

The Matter test event triggers feature is enabled by default for all Matter samples.
To disable it, set the :kconfig:option:`CONFIG_NCS_SAMPLE_MATTER_TEST_EVENT_TRIGGERS` kconfig option to ``n``.

To trigger a specific event on the device, run the following command:

.. code-block:: console

  ./chip-tool generaldiagnostics test-event-trigger hex:<enable key> <activation code> <node id> 0

Replace the ``enable key`` value with your device's enable key, and the ``activation code`` and ``node id`` values with the values for the event you want to trigger.

The following is an example of the Reboot activation code with a 5 ms delay for Matter Template device which has a ``node id`` set to ``1``, using the default enable key:

.. code-block:: console

  ./chip-tool generaldiagnostics test-event-trigger hex:00112233445566778899AABBCCDDEEFF 0xF000000100000005 1 0
