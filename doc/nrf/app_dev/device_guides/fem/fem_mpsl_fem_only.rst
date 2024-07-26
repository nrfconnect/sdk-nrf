.. _ug_radio_fem_direct_support:

MPSL FEM-only configuration
###########################

If your application cannot use MPSL, you can use only the FEM driver provided by MPSL by enabling the following Kconfig options:

   * :kconfig:option:`CONFIG_MPSL`
   * :kconfig:option:`CONFIG_MPSL_FEM_ONLY`

You can use the FEM-only configuration to run simple radio protocols that are not intended to be used concurrently with other protocols.

Some applications might perform calls to the :ref:`nrfxlib:mpsl_fem` API even though no RF Front-End module is physically connected to the device and the :kconfig:option:`CONFIG_MPSL_FEM` Kconfig option is set to ``n``.
In that case, ensure that the :kconfig:option:`CONFIG_MPSL_FEM_API_AVAILABLE` Kconfig option is set to ``y``.
