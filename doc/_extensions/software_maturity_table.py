"""
Copyright (c) 2023 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

This extension provides a directive ``sml-table`` that will produce a table
of generated content on software maturity levels. This information is
downloaded from a cache in Azure, and stored in the build environment.

The directive takes a feature category as an argument, and produces a
single table for that category. Alternatively, ``top_level`` can be
supplied as an argument to produce a top-level table on the software
maturity level of different technologies.

Related files
*************
- nrf/doc/_scripts/software_maturity_features.yaml
- nrf/doc/_scripts/software_maturity_scanner.py

All possible feature categories are listed in
``software_maturity_features.yaml``. To add a new category or a feature
to a category, add them to this file.

Options
*******
:remove-columns: string of SoCs separated by whitespace.
    Columns titled with the given values will be removed from the
    generated table.
    Example:
        :remove-columns: nRF52811 nRF52820 nRF52833


:remove-rows: list of features.
    Rows titled with the given feature (case sensitive) will be removed from
    the generated table.
    Example:
        :remove-rows: ["Zigbee (Sleepy) End Device", "Zigbee Coordinator"]

:add-columns: list of tuples on the form (title, default-value).
    Adds additional columns to the table with the given title and all
    values set to the default value. Values can be override with the
    :insert-value: option.
    Example:
        :add-columns: [("nRF9998", "Experimental"), ("nRF0001", "-")]

:add-rows: list of tuples on the form (title, default-value).
    Adds additional rows to the table with the given title and all
    values set to the default value. Values can be override with the
    :insert-value: option.
    Example:
        :add-rows: [("My Feature", "Experimental"), ("My other feature", "-")]

:insert-value: list of tuples on the form (row-title, column-title, new-value)
    Overrides the value of an entry in the table with the given value.
    Example:
    :insert-values:
        [
            (
                "OTA dfu over Zigbee",
                "NRF52820",
                "Supported"
            ),
            (
                "Zigbee (Sleepy) End Device",
                "nRF52811",
                "Experimental"
            )
        ]

Example of use
**************
.. sml-table:: bluetooth

or

.. sml-table:: zigbee
  :remove-columns: nRF52810 nrf52832 nrf9999
  :remove-rows: ["Zigbee Coordinator + Bluetooth LE multiprotocol",
    "Zigbee Router + Bluetooth LE multiprotocol"]
  :add-columns: [("nRF9998", "Experimental"), ("nRF0001", "-")]
  :add-rows: [("My Feature", "Experimental"), ("My other feature", "-")]
  :insert-values:
    [
      (
        "OTA dfu over Zigbee",
        "NRF52820",
        "Supported"
      ),
      (
        "Zigbee (Sleepy) End Device",
        "nRF52811",
        "Experimental"
      )
    ]
"""

from docutils import nodes
from docutils.parsers.rst import directives
from sphinx.util.docutils import SphinxDirective
from sphinx.application import Sphinx
from azure.storage.blob import ContainerClient
from sphinx.util import logging
import requests
import yaml

__version__ = "0.1.1"

AZ_CONN_STR_PUBLIC = ";".join(
    (
        "DefaultEndpointsProtocol=https",
        "EndpointSuffix=core.windows.net",
        "AccountName=ncsdocsa",
    )
)
"""Azure connection string (public acces)."""

AZ_CONTAINER = "ncs-doc-generated-reports"
"""Azure container."""

logger = logging.getLogger(__name__)


class SoftwareMaturityTable(SphinxDirective):
    """A directive that generates a table from information in a YAML file."""

    has_content = True
    required_arguments = 1
    optional_arguments = 5

    option_spec = {
        "remove-columns": directives.unchanged,
        "remove-rows": eval,
        "add-columns": eval,
        "add-rows": eval,
        "insert-values": eval,
    }

    def run(self):
        # Table information is stored in self.env.sml_table
        if not hasattr(self.env, "sml_table"):
            return [
                nodes.paragraph(text="Software maturity level table source not found")
            ]

        table_type = self.arguments[0]
        table_info = self.env.sml_table

        default_values = {}

        if table_type != "top_level" and table_type not in table_info["features"]:
            logger.error(f"No information present for requested feature '{table_type}'")
            return []

        all_socs = table_info["all_socs"]
        if "remove-columns" in self.options:
            for column in self.options["remove-columns"].split():
                column = column.lower()
                if column in all_socs:
                    all_socs.remove(column)

        if "remove-rows" in self.options:
            self.options["remove-rows"] = list(
                map(str.lower, self.options["remove-rows"])
            )

        if "add-columns" in self.options:
            for soc, default_value in self.options["add-columns"]:
                soc = soc.lower()
                if soc not in all_socs:
                    all_socs.append(soc)
                default_values[soc] = default_value

        if table_type == "top_level":
            features = table_info["top_level"]
        else:
            features = table_info["features"][table_type]

        if "add-rows" in self.options:
            for feature, default_value in self.options["add-rows"]:
                if feature not in features:
                    maturity_list = {"Experimental":[], "Supported":[], default_value:all_socs}
                    features[feature] = maturity_list

        # Create a matrix-like table
        table = nodes.table()
        tgroup = nodes.tgroup(cols=len(all_socs) + 1)
        for _ in range(len(all_socs) + 1):
            colspec = nodes.colspec(colwidth=1)
            tgroup.append(colspec)
        table += tgroup

        # Table head row displays the SoCs
        thead = nodes.thead()
        tgroup += thead
        row = nodes.row()
        row += nodes.entry()

        for soc in all_socs:
            soc = soc.replace("nrf", "nRF")
            entry = nodes.entry()
            entry += nodes.paragraph(text=soc)
            row += entry
        thead.append(row)

        # Table body
        rows = []
        feature_prefix = table_type + "_"
        for feature, maturity_list in features.items():
            feature = feature.lstrip(feature_prefix)
            if (
                "remove-rows" in self.options
                and feature.lower() in self.options["remove-rows"]
            ):
                continue

            row = nodes.row()
            rows.append(row)

            # First column displays the feature
            entry = nodes.entry()
            entry += nodes.strong(text=feature)
            row += entry

            for soc in all_socs:
                entry = nodes.entry()
                if soc in maturity_list["Experimental"]:
                    text = nodes.Text("Experimental")
                elif soc in maturity_list["Supported"]:
                    text = nodes.Text("Supported")
                elif soc in default_values:
                    text = nodes.Text(default_values[soc])
                else:
                    text = nodes.Text("-")

                if "insert-values" in self.options:
                    for in_feature, in_soc, val in self.options["insert-values"]:
                        if (
                            in_feature.lower() == feature.lower()
                            and in_soc.lower() == soc.lower()
                        ):
                            text = nodes.Text(val)
                            break

                entry += text
                row += entry

        tbody = nodes.tbody()
        tbody.extend(rows)
        tgroup += tbody

        return [table]


def download_sml_table(app: Sphinx) -> None:
    """Download SML table from Azure"""

    # Check if local cache exists
    if hasattr(app.env, "sml_table"):
        local_cache = app.env.sml_table
    else:
        logger.info("No software maturity level cache found locally")
        local_cache = None

    # Check internet connection
    try:
        requests.get("https://ncsdocsa.blob.core.windows.net", timeout=3)
    except (requests.ConnectionError, requests.Timeout):
        logger.info("Could not retrieve software maturity level information online")
        if local_cache:
            logger.info("Using local software maturity level cache")
        return

    # Get remote files
    cc = ContainerClient.from_connection_string(AZ_CONN_STR_PUBLIC, AZ_CONTAINER)
    remote_files = sorted(
        [b for b in cc.list_blobs(name_starts_with="software_maturity")],
        key=lambda b: b.last_modified,
        reverse=True,
    )

    # Check for updates
    target = remote_files[0]
    if local_cache and local_cache["md5"] == target.content_settings["content_md5"]:
        logger.info("Up to date software maturity level table found in cache")
        return

    # Download new version
    bc = cc.get_blob_client(target)
    res = bc.download_blob().content_as_text()
    app.env.sml_table = yaml.safe_load(res)
    app.env.sml_table["md5"] = target.content_settings["content_md5"]
    logger.info("Software maturity level table cached locally")


def setup(app: Sphinx):
    app.add_directive("sml-table", SoftwareMaturityTable)
    app.connect("builder-inited", download_sml_table)

    return {
        "version": __version__,
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
