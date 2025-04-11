.. _hpf_fault_handling:

Fault handling
##############

The following page outlines mechanisms and strategies employed to manage faults in both the Host and High-Performance Framework (HPF) firmware environments.

Host fault
**********

In case of a Host crash followed by a reset, the management of the VPR state depends on the system architecture:

* On the :ref:`nRF54L platform<ug_nrf54l>`, the hardware is designed to ensure that a reset of the application core (Host) simultaneously triggers a reset of the HPF firmware (:ref:`FLPR core<vpr_flpr_nrf54l>`).
* On the :ref:`nRF54H platform<ug_nrf54h>`, the :ref:`Secure Domain Firmware<ug_nrf54h20_secure_domain>` is responsible for resetting all resources allocated to the owner of the HPF FW (FLPR), which is the Host.

This architecture ensures synchronization between the cores.

Firmware fault
**************

In case a HPF firmware crashes, it must be signalled to the Host.
Since there are no hardware resources dedicated to this task, you must implement the solution in software.
You can employ the following strategies individually or combined:

.. tabs::

   .. tab:: TIMER peripheral

      Use the TIMER peripheral as watchdog in one of the following ways:

         * Set up the TIMER with the Host and ensure it is regularly cleared by the HPF firmware.
           If the firmware fails to clear the TIMER, an Interrupt Request (IRQ) is sent to the Host indicating an HPF firmware error.
         * Use the Watchdog Timer (WDT) peripheral with STOP functionality.
           HPF firmware has to feed the watchdog.
           If it fails to do so, the Host is receives an interrupt and must execute TASK_STOP.
           This solution assumes that the Host is always able to invoke TASK_STOP within two 32kHz cycles. Otherwise, the WDT triggers a reset, which might be undesired.

   .. tab:: Trap handler

      Implement a trap handler that singals the fault condition to the Host, for example, using the VEVIF IRQ and a predefined memory location to store the error code.

      .. note::
          The drawback of relying only on this solution is its inability to address scenarios where the HPF firmware is caught in an infinite loop.
          This risk might be mitigated through thorough code inspection and testing.

   .. tab:: Shared memory flag

      Create a flag within shared memory that is periodically updated by the HPF firmware and monitored by the Host.
