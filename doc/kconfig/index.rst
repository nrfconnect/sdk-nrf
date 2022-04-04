.. _configuration_options:
.. _kconfig-search:

Kconfig search
##############


:file:`Kconfig` files describe build-time configuration options (called symbols in Kconfig-speak), how they are grouped into menus and sub-menus, and dependencies between them that determine what configurations are valid.
:file:`Kconfig` files appear throughout the directory tree.
For example, :file:`subsys/pm/Kconfig` defines power-related options.

All Kconfig options can be searched using the search functionality.
The search functionality supports searching using regular expressions.
See :ref:`kconfig_regex` for more information.
All the Kconfig options that match a particular regular expression is displayed along with the information on the matched Kconfig options.
The search results can be navigated page by page if the number of matches exceeds a page.

.. kconfig:search::

.. toctree::
   :maxdepth: 1
   :hidden:

   regex
