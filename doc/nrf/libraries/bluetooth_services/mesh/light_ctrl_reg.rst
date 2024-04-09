.. _bt_mesh_light_ctrl_reg_readme:

Illuminance regulator interface
###############################

.. contents::
   :local:
   :depth: 2

This module defines an interface for implementation and interaction with an abstract illuminance regulator.

Using the regulator
*******************

As this defines an interface for an abstract illuminance regulator, it cannot be used on its own.
For an example of regulator usage, see :ref:`bt_mesh_light_ctrl_srv_readme`.

A regulator implementation will provide a :c:struct:`bt_mesh_light_ctrl_reg` instance, and this is used as the interface to the regulator.
Field :c:member:`bt_mesh_light_ctrl_reg.user_data` of this struct can be used to store a user context.

Before using the regulator, it needs to be initialized through the :c:member:`bt_mesh_light_ctrl_reg.init` function.
Functions :c:member:`bt_mesh_light_ctrl_reg.start` and :c:member:`bt_mesh_light_ctrl_reg.stop` are used to start and stop the regulator.

Regulator inputs
----------------

The regulator has two inputs:

* Target value
* Measured value

The illuminance regulator takes the specified target value as the reference level and compares it to the reported measured value.
The inputs are compared to establish an error for the regulator, which the regulator tries to minimize.

The measured value is passed through the :c:member:`bt_mesh_light_ctrl_reg.measured` variable.
The passed value is to be used in the next regulator step.
The regulator depends on measurement frequency to provide a stable output.
The measurement frequency should be as close as possible to the update interval of the regulator.
If the measurement frequency is too low, the regulator might oscillate as it attempts to compensate for outdated feedback.

The desired target value is passed through the :c:func:`bt_mesh_light_ctrl_reg_target_set` function.
If the transition time is greater than zero, the target value will be interpolated linearly over the transition time from the previously set value to the new one.

Regulator output
----------------

The regulator contains a proportional (P) and an integral (I) component whose outputs are summarized to a regulator output value.
To get output from the regulator, set the :c:member:`bt_mesh_light_ctrl_reg.updated` callback.
When the regulator is running, it will repeatedly call this callback.

Tuning the regulator
--------------------

Regulator tuning is complex and depends on a lot of internal and external parameters.
Varying sensor delay, light output and light change rate may significantly affect the regulator performance and accuracy.
To compensate for the external parameters, each regulator component provides user controllable coefficients that change their impact on the output value:

* K\ :sub:`p` - for the proportional component
* K\ :sub:`i` - for the integral component

These coefficients can have individual values for positive and negative errors, referenced as follows in the API:

* K\ :sub:`pu` - proportional up; used when target is higher.
* K\ :sub:`pd` - proportional down; used when target is lower.
* K\ :sub:`iu` - integral up; used when target is higher.
* K\ :sub:`id` - integral down; used when target is lower

Regulators are tuned to fit their environment by changing the coefficients.
The coefficient adjustments are typically done by analyzing the system's *step response*.
The step response is the overall system response to a change in reference value, for instance in a state change in the light level state machine.

.. note::
    The transition time between states in the Light LC Server makes the feedback loop more forgiving.
    A larger transition time can compensate for poor regulator response.

The illuminance regulator is a PI regulator, which uses the following components to compensate for a mismatch between the reference and the measured level:

* Instantaneous error - The proportional component that is typically the main source of correction for a PI regulator.
  It compares the reference value to the most recent feedback value, and attempts to correct the error by eliminating the difference.
* Accumulated error - The integral component that compensates for errors by adding up the sum of the error over time.
  Its main contribution is to eliminate system bias and accelerate the system step response.

Changing different coefficients will affect the step response differently.
Increasing the two coefficients will have the following effect on the step response:

=========== ========= ========= ============= ==================
Coefficient Rise time Overshoot Settling time Steady-state error
=========== ========= ========= ============= ==================
K\ :sub:`p`  Faster    Higher    \-            \-
K\ :sub:`i`  Faster    Higher    Longer        Reduced
=========== ========= ========= ============= ==================

The value of the coefficients is typically a trade-off between fast response time and system instability:

* If the value is too high, the system might become unstable, potentially leading to oscillation and loss of control.
* If the value is too low, the step response might be too slow or unable to reach the target value altogether.

Implementing a new regulator
****************************

To implement a new regulator using this generic interface, declare a :c:struct:`bt_mesh_light_ctrl_reg` struct, and set the :c:member:`bt_mesh_light_ctrl_reg.init`, :c:member:`bt_mesh_light_ctrl_reg.start`, and :c:member:`bt_mesh_light_ctrl_reg.stop` fields to implementations of these functions.
Use :c:func:`bt_mesh_light_ctrl_reg_input_get` to get the current target value for the regulator supplied by the regulator user.
The value returned is interpolated linearly over the transition time, if a transition time is requested by the regulator user.

The :c:member:`bt_mesh_light_ctrl_reg.init` function must perform the necessary steps to initialize the implementation, such as initializing interrupt handlers or timers, but not start the regulator.

Use the :c:member:`bt_mesh_light_ctrl_reg.start` and :c:member:`bt_mesh_light_ctrl_reg.stop` functions to start and stop the regulator after it has been initialized by a call to ``init``.

On every regulator step, the regulator must call :c:member:`bt_mesh_light_ctrl_reg.updated` callback supplied by the user.

For an example of regulator implementation, see :ref:`bt_mesh_light_ctrl_reg_spec_readme`.


API documentation
*****************

| Header file: :file:`include/bluetooth/mesh/light_ctrl_reg.h`
| Source file: :file:`subsys/bluetooth/mesh/light_ctrl_reg.c`

.. doxygengroup:: bt_mesh_light_ctrl_reg
   :project: nrf
   :members:
