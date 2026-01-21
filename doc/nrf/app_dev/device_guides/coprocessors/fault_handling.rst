.. _hpf_fault_handling:

Fault handling
##############

.. contents::
   :local:
   :depth: 2

.. caution::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>` and is limited to the nRF54L15 device.

The following page outlines mechanisms and strategies that shall be employed to manage faults in both the Host and High-Performance Framework (HPF) firmware environments.

.. note::
   This page outlines best practices for architecture design.
   You might need to implement some components, as they are not yet available.

Handling Host crash
*******************

In case of a Host crash followed by a reset, the hardware is designed to ensure that a reset of the application core (Host) simultaneously triggers a reset of the HPF firmware (FLPR core).

This ensures synchronization between the cores.

Handling firmware crash
***********************

In case a HPF firmware crashes, the issue must be signalled to the Host.
Since there are no hardware resources dedicated to this task, you must implement the solution in software.
You can employ the following strategies, individually or combined:

* Use the TIMER peripheral as watchdog in one of the following ways:

  * Set up the TIMER with the Host and ensure it is regularly cleared by the HPF firmware.
    If the firmware fails to clear the TIMER, an Interrupt Request (IRQ) is sent to the Host indicating an HPF firmware error.
  * Use the Watchdog Timer (WDT) peripheral with STOP functionality.
    HPF firmware has to feed the watchdog.
    If it fails to do so, the Host receives an interrupt and must execute TASK_STOP.
    This solution assumes that the Host is always able to invoke TASK_STOP within two 32kHz cycles. Otherwise, the WDT triggers a reset, which might be undesired.

* Implement a trap handler that signals the fault condition to the Host, for example, using the VEVIF IRQ and a predefined memory location to store the error code.

   .. note::
         The drawback of relying only on this solution is its inability to address scenarios where the HPF firmware is caught in an infinite loop.
         This risk might be mitigated through thorough code inspection and testing.

* Create a flag within shared memory that is periodically updated by the HPF firmware and monitored by the Host.

 By implementing the outlined strategies, you can ensure that both the Host and HPF firmware can manage and recover from faults efficiently, thereby enhancing system stability and performance.
