.. _gs_installing:

Installing the |NCS|
####################

To work with the |NCS|, you must install the required tools and clone the |NCS| repositories.
These steps are interrelated, because the repositories contain information about what tools you need to install.

Supported operating systems
***************************

The |NCS| supports Microsoft Windows and Linux for development.
However, there are some Zephyr features that are currently only available on Linux, including:

* sanitycheck
* BlueZ integration
* net-tools integration


.. _gs_installing_toolchain:

Installing the toolchain
************************

Follow Zephyr's Getting Started Guide to install the toolchain on your operating system:

* :ref:`zephyr:installing_zephyr_win`
* :ref:`zephyr:installation_linux`

.. important::
   As part of the installation process, you must clone the Zephyr repository.
   The instructions contain the URL of Zephyr's official repository; you should replace this with the |NCS| fork of the Zephyr repository.
   See :ref:`cloning_the_repositories` for more information.

.. _cloning_the_repositories:

Cloning the repositories
************************

.. note::
   The following instructions are temporary.
   There will be a tool that automizes the process of cloning the repositories.

1. Fork the four |NCS| repositories to your own GitHub account by clicking the **Fork** button in the upper right-hand corner of each repository page:

   a. |ncs_zephyr_repo|
   #. |ncs_repo|
   #. |ncs_mcuboot_repo|
   #. |ncs_nrfxlib_repo|

#. Create a folder named ``ncs``.
   This folder will hold all |NCS| repositories.
#. Open a GIT bash in the ncs folder.
#. Clone your forks:

   .. code-block:: console

      git clone https://github.com/<username>/_fw-nrfconnect-nrf91-zephyr.git zephyr

      git clone https://github.com/<username>/fw-nrfconnect-mcuboot.git mcuboot

      git clone https://github.com/<username>/fw-nrfconnect-nrf.git nrf

      git clone https://github.com/<username>/nrfxlib.git nrfxlib

   Replace *<username>* with your GitHub user name.
#. To link your local repositories to the |NCS| repositories, add remotes that point to the original repositories:

   .. code-block:: console

      cd zephyr
      git remote add ncs https://github.com/NordicPlayground/_fw-nrfconnect-nrf91-zephyr.git

      cd ../mcuboot
      git remote add ncs https://github.com/NordicPlayground/fw-nrfconnect-mcuboot.git

      cd ../nrf
      git remote add ncs https://github.com/NordicPlayground/fw-nrfconnect-nrf.git

      cd ../nrfxlib
      git remote add ncs https://github.com/NordicPlayground/nrfxlib.git

#. Optionally, add remotes to the upstream repositories for Zephyr and Mcuboot:

   .. code-block:: console

      cd ../zephyr
      git remote add upstream https://github.com/zephyrproject-rtos/zephyr.git

      cd ../mcuboot
      git remote add upstream https://github.com/runtimeco/mcuboot.git

Your directory structure now looks like this::

   ncs
    |___ mcuboot
    |___ nrf
    |___ nrfxlib
    |___ zephyr
