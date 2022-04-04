.. _kconfig_regex:

Regex tips and tricks
#####################

Following are some of the useful JavaScript-based Regex patterns that can be used while searching the configuration options:

* ``.*`` - To search for zero or more occurrences of the string that follows the special character combination.
*  ``\d`` - To search for any digit. Equivalent to ``[0-9]``. For example, ``\d`` or ``[0-9]`` matches ``6`` in ``IPV6``.
* ``^...$`` - To search for a string of finite length, starting and ending with definite characters. For example, ``^c....._m...m$`` matches :Kconfig:option:`CONFIG_MODEM`.

For more information, see `Regular expression syntax cheatsheet`_.

.. _`Regular expression syntax cheatsheet`: https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Regular_Expressions/Cheatsheet
