# Kconfig documentation build configuration file

import os

NRF_BASE = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

# -- General configuration ------------------------------------------------

# Sphinx 2.0 changes the default from 'index' to 'contents'
master_doc = 'index'

# General information about the project.
project = 'Kconfig Reference'
copyright = '2019, Nordic Semiconductor'
author = 'Nordic Semiconductor'

version = '1.0.0'  # Dummy version

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
exclude_patterns = ['_build']

# -- Options for HTML output ----------------------------------------------

html_theme = "kconfig"
html_theme_path = ['{}/doc/themes'.format(NRF_BASE)]
html_favicon = '{}/doc/static/images/favicon.ico'.format(NRF_BASE)

# The name for this set of Sphinx documents.  If None, it defaults to
# "<project> v<release> documentation".
html_title = "Kconfig Reference"

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['{}/doc/static'.format(NRF_BASE)]

# If not '', a 'Last updated on:' timestamp is inserted at every page bottom,
# using the given strftime format.
html_last_updated_fmt = '%b %d, %Y'

# If false, no module index is generated.
html_domain_indices = False

# If false, no index is generated.
html_use_index = True

# If true, the index is split into individual pages for each letter.
html_split_index = True

# If true, "Created using Sphinx" is shown in the HTML footer. Default is True.
html_show_sphinx = False

# If true, "(C) Copyright ..." is shown in the HTML footer. Default is True.
html_show_copyright = True

# If true, license is shown in the HTML footer. Default is True.
html_show_license = True

def setup(app):
    app.add_stylesheet("css/common.css")
    app.add_stylesheet("css/kconfig.css")
