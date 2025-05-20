.. _sysbuild_enabled_ncs_configuring:

Configuring sysbuild usage in west
##################################

The |NCS| has :ref:`sysbuild enabled by default <sysbuild_enabled_ncs>` for :ref:`all types of applications <create_application_types>` in the :ref:`SDK repositories <dm_repo_types>`.

You can configure your project to not sysbuild by default whenever invoking ``west build``.
You can do this either per-workspace, using the local configuration option, or for all your workspaces, using the global configuration option:

.. tabs::

   .. group-tab:: Local sysbuild configuration

      Use the following command to configure west not to use sysbuild by default for building all projects in the current workspace:

      .. parsed-literal::
         :class: highlight

         west config --local build.sysbuild False

      Use the following command to configure west to use sysbuild by default:

      .. parsed-literal::
         :class: highlight

         west config --local build.sysbuild True

   .. group-tab:: Global sysbuild configuration

      Use the following command to configure west not to use sysbuild by default for building all projects in all workspaces:

      .. parsed-literal::
         :class: highlight

         west config --global build.sysbuild False

      Use the following command to configure west to use sysbuild by default:

      .. parsed-literal::
         :class: highlight

         west config --global build.sysbuild True

.. note::
    |parameters_override_west_config|
    This means that you can pass ``--no-sysbuild`` to the build command to disable using sysbuild for a given build even if you configured west to always use sysbuild by default.
