.. _nrf_machine_learning_app_internal_modules:

nRF Machine Learning: Internal modules
######################################

.. contents::
   :local:
   :depth: 2

The nRF Machine Learning application uses modules available in :ref:`lib_caf` (CAF), a set of generic modules based on Application Event Manager and available to all applications, and a set of dedicated internal modules.
See :ref:`nrf_machine_learning_app_architecture` for more information.

The nRF Machine Learning application uses the following modules available in CAF:

* :ref:`caf_ble_adv`
* :ref:`caf_ble_bond`
* :ref:`caf_ble_smp`
* :ref:`caf_ble_state`
* :ref:`caf_buttons`
* :ref:`caf_click_detector`
* :ref:`caf_leds`
* :ref:`caf_power_manager`
* :ref:`caf_sensor_manager`

See the module pages for more information about the modules and their configuration.

The nRF Machine Learning application also uses the following dedicated application modules:

``ei_data_forwarder_bt_nus``
  The module forwards the sensor readouts over NUS to the connected Bluetooth Central.
  The sensor data is forwarded only if the connection is secured and the connection interval is within the limit defined by the :kconfig:option:`CONFIG_BT_PERIPHERAL_PREF_MAX_INT` and :kconfig:option:`CONFIG_BT_PERIPHERAL_PREF_MAX_INT` Kconfig options.

``ei_data_forwarder_uart``
  The module forwards the sensor readouts over UART.

``led_state``
  The module displays the application state using LEDs.
  The LED effects used to display the state of data forwarding, the machine learning results, and the state of the simulated signal are defined in the :file:`led_state_def.h` file located in the application configuration directory.
  The common LED effects are used to represent the machine learning results and the simulated sensor signal.

``ml_runner``
  The module uses the :ref:`ei_wrapper` API to control running the machine learning model.
  It provides the prediction results using :c:struct:`ml_result_event`.

``ml_app_mode``
  The module controls application mode.
  It switches between running the machine learning model and forwarding the data.
  The change is triggered by a long press of the button defined in the module's configuration.

``sensor_sim_ctrl``
  The module controls the parameters of the generated simulated sensor signal.
  It switches between predefined sets of parameters for the simulated signal.
  The parameters of the generated signals are defined by the :file:`sensor_sim_ctrl_def.h` file located in the application configuration directory.

``usb_state``
  The module enables USB.

.. note::
   The ``ei_data_forwarder_bt_nus`` and ``ei_data_forwarder_uart`` modules stop forwarding the sensor readouts if they receive a :c:struct:`sensor_event` that cannot be forwarded and needs to be dropped.
   This could happen, for example, if the selected sensor sampling frequency is too high for the used implementation of the Edge Impulse data forwarder.
   Data forwarding is stopped to make sure that dropped samples are noticed by the user.
   If you switch to running the machine learning model and then switch back to data forwarding, the data is again forwarded to the host.
