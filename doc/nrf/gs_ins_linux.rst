.. _gs_installing_linux:

.. |os| replace:: Linux

.. include:: gs_ins_windows.rst
   :start-after: intro_start
   :end-before: intro_end




.. _gs_installing_tools_linux:

Installing the required tools
*****************************

To install the required tools, follow the :ref:`zephyr:linux_requirements` section of Zephyr's Getting Started Guide.

In addition, make sure that you have dtc v1.4.6 or later installed.
Depending on the Linux distribution that you use, you might need to install it manually because the current official package version might be older than v1.4.6.
If you use Ubuntu, install v1.4.7 from Cosmic by entering the following commands::

   wget http://mirrors.kernel.org/ubuntu/pool/main/d/device-tree-compiler/device-tree-compiler_1.4.7-1_amd64.deb
   sudo dpkg -i device-tree-compiler_1.4.7-1_amd64.deb

.. note::
   You do not need to install the Zephyr SDK.
   We recommend to install the compiler toolchain separately, as detailed in `Installing the toolchain`_.


.. _gs_installing_toolchain_linux:

.. |installextract| replace:: Extract
.. |tcfolder| replace:: ``~/gnuarmemb``

.. include:: gs_ins_windows.rst
   :start-after: toolchain_start
   :end-before: #. If you want to build and program

#. If you want to build and program applications from the command line, define the environment variables for the GNU Arm Embedded toolchain.
   To do so, enter the following commands in a terminal window (assuming that you have installed the toolchain to |tcfolder|; if not, change the value for GNUARMEMB_TOOLCHAIN_PATH)::

     export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
     export GNUARMEMB_TOOLCHAIN_PATH="~/gnuarmemb"

#. Instead of setting the environment variables every time you start a new session, define them in the``~/.zephyrrc`` file as described in `Setting up the build environment`_.




.. _cloning_the_repositories_linux:

.. |bash| replace:: terminal window

.. include:: gs_ins_windows.rst
   :start-after: cloning_start
   :end-before: cloning_end




.. _additional_deps_linux:

.. |prompt| replace:: terminal window

.. include:: gs_ins_windows.rst
   :start-after: add_deps_start
   :end-before: add_deps_end

.. code-block:: console

   pip3 install --user -r zephyr/scripts/requirements.txt
   pip3 install --user -r nrf/scripts/requirements.txt




.. _build_environment_linux:

.. |envfile| replace:: ``zephyr/zephyr-env.sh``
.. |rcfile| replace:: ``~/.zephyrrc``

.. include:: gs_ins_windows.rst
   :start-after: buildenv_start
   :end-before: buildenv_end
