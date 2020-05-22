Introduction
============

``nrfx`` is a standalone set of drivers for peripherals present in Nordic
Semiconductor's SoCs. It originated as an extract from the nRF5 SDK.
The intention was to provide drivers that can be used in various environments
without the necessity to integrate other parts of the SDK into them.
For the user's convenience, the drivers come with the MDK package. This package
contains definitions of register structures and bitfields for all supported
SoCs, as well as startup and initialization files for them.

Refer to the :ref:`Driver support matrix <nrfx_drv_supp_matrix>` to check which
drivers are suitable for a given SoC.

.. _nrfx_integration:

Integration
-----------

The purpose of *nrfx* is to make it possible to use the same set of peripheral
drivers in various environments, from RTOSes to bare metal applications.
Hence, for a given host environment, a light integration layer must be provided
that implements certain specific routines, like interrupt management, critical
sections, assertions, or logging. This is done by filling a predefined set of
macros with proper implementations (or keeping some empty if desired) in files
named:

- *nrfx_glue*
- *nrfx_log*

Templates of these files are provided in the `templates` subfolder. Their
customized versions can be placed in any location within the host environment
that the used compiler can access via include paths.

.. _templates: ../../templates

In addition, the following locations should be specified as include paths
([nrfx] stands for the *nrfx* root folder location):

.. code-block::

    [nrfx]/
    [nrfx]/drivers/include
    [nrfx]/mdk

.. nrfx_irq_handlers:

IRQ handlers
------------

The IRQ handlers in all drivers are implemented as ordinary API functions
named "nrfx_*_irq_handler". They can be bound to some structures or called in
a specific way according to the requirements of the host environment.
To install the handlers in the standard MDK way, you must only add the following
line to the *nrfx_glue* file:

.. code-block:: c

    #include <soc/nrfx_irqs.h>

This will cause the preprocessor to properly rename all the IRQ handler
functions so that the linker could install them in the vector table.

.. nrfx_configuration:

Configuration
-------------

The drivers use both dynamic (run time) and static (compile time) configuration.

Dynamic configuration is done by specifying desired options in configuration
structures passed to the drivers during their initialization.
Refer to the API reference for a given driver to see the members of its
configuration structure.

Static configuration allows enabling and disabling (excluding their code from
compilation) particular drivers or in some cases their specific features,
defining default parameters for dynamic configuration, parametrization of
logging in particular drivers. It is done by specifying desired values of macros
in a file named:

- nrfx_config.h

This file, similarly to the integration files mentioned above, can be placed
in any suitable location within the host environment.
The `templates_` subfolder contains templates of configuration files for all
currently supported Nordic SoCs. These files are included through a common
nrfx_config.h file, according to the selected SoC. Refer to the "driver
configuration" section in the API reference for a given driver for more
information regarding configuration options available for it.

.. nrfx_additional_reqs:

Additional requirements
-----------------------

Nordic SoCs are based on ARM® Cortex™-M series processors. Before you can
start developing with *nrfx*, you must add the CMSIS header files to include
paths during the compilation process. Download these files from the following
website:

- `ARM® CMSIS repository_` (CMSIS/Include directory)

.. _ARM® CMSIS repository: https://github.com/ARM-software/CMSIS
