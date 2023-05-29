"""
NCS Software Maturity Level Scanner
===================================

This script can be used to generate a software maturity level report based
on experimental warnings in the build output and parsing Kconfig files.

The script requires three inputs to be functional:
- The twister output directory. This directory is searched for build.log files
  in which the experimental warnings appear. It is also searched for .config
  files to find relevant enabled Kconfig symbols.
- An Azure access key, to write the generated report to Azure blob storage.
- A YAML file describing all the desired features and their respective Kconfig
  rules. The file should contain a field for all the desired technologies
  listed in the top-level table, and then a field for all features within each
  technology.

  The Kconfig rules listed in the file can cosist of one or more Kconfig
  symbols, related with '&&' (indicating that all symbols must be present) or
  '||' (indicating that at least one of the symbols must be present). && has a
  higher precedence than ||, so  A || B && C is equivalent to A || (B && C).
  Parenthesis can also be used to form rules like (A || B && (C || D)) && E.
  Finally, a symbol can be negated with '!', indicating that the symbol must
  not be present. Only symbols can be negated, not entire expressions.

  Specific values for symbols can be given with '=', e.g. A=2. When no specific
  value is given, a symbol is automatically expanded to A=y. Negating a symbol
  with a value allows all occurrences of the symbol where the value differs
  from the given value.

  One such file might look like this:

    top_table:
        Bluetooth: BT_CTLR
        Zigbee: ZIGBEE
        Thread: NET_L2_OPENTHREAD
        LTE: LTE_LINK_CONTROL
    features:
        bluetooth:
            LLPM: CAF_BLE_USE_LLPM
            Periodic Advertisement: BT_CTLR_SYNC_PERIODIC
        matter:
            Matter commissioning over IP: CHIP
            Matter over Thread: CHIP && NET_L2_OPENTHREAD

If no input file is specified, the script will look in the local directory for
a 'software_maturity_features.yaml' file.

The ``TWISTER_OUT_DIR`` environment variable is used by default to provide the
necessary input folder. It can be overwritten by supplying the
``--build-output`` option.

The Azure access key can be supplied with the argument ``--access-key``.
If not provided as an argument explicitly, the environment variable
``AZURE_REPORT_CACHE_ACCESS_KEY`` is used by default.

Publish Results/Run Locally
***************************
The script can run in two different modes, based on which of the following
options are supplied: ``--save-output`` or ``--publish-results``.

When ``--save-output`` is used, no Azure access keys are needed, and the
results are written to the specified file on the local filesystem.

When run with the ``--publish-results``, the script will run as normal, and
upload the resulting YAML file to Azure blob storage. This should ONLY be
done by the CI build system. To differentiate the results of different runs,
use the ``--commit-sha`` option upon execution to mark the commit on which the
build output is based.

Script Logic
************

A complete list of all available SoCs are found by searching through the
samples and applications folder for sample.yaml files and retrieving the
``integration_platforms`` list from each. Anything nRF51 and not starting with
nRF is excluded.

To mark features as experimental the twister output folder is searched for
all build.log files and scanned for the string "Experimental symbol XXX is
enabled". When found, The SoC is looked up in /zephyr/.config, and a mapping
from each SoC to the experimental symbols detected when using it to build is
stored and used to evaluate the Kconfig rules.

Next, the twister output folder is searched for all .config-files. These
are evaluated against the given Kconfig rules to determine which features
are supported, experimental or not supported. If a rule contains multiple
symbols connected with either && or ||, all of the symbols must be present in
the same .config-file for a SoC to be registered as supporting that feature.
That means that a feature is not marked as supported if there is no explicit
test case where all the required Kconfig symbols are present at the same time,
even if the required options have been enabled individually in separate
samples. If at least one of the symbols are marked as experimental, the feature
is also marked as experimental.

In cases where there are rules with multiple symbols connected with ||, and one
is experimental and the others are not, the feature will still be marked as
experimental. If this is an undesired outcome, the feature should be split
into two separate features, one with the experimental symbols and the other
with the supported ones.

Copyright (c) 2022 Nordic Semiconductor ASA
"""

from enum import IntEnum
from pathlib import Path
from typing import Dict, List, Set
from azure.storage.blob import ContainerClient
import kconfiglib
import os
import re
import sys
import yaml
import logging
import argparse


AZ_CONN_STR = ";".join(
    (
        "DefaultEndpointsProtocol=https",
        "EndpointSuffix=core.windows.net",
        "AccountName=ncsdocsa",
        "AccountKey={}",
    )
)
"""Azure connection string (private acces)"""

AZ_CONTAINER = "ncs-doc-generated-reports"
"""Azure container."""

UPLOAD_PREFIX = "software_maturity"
"""Prefix used by all files related to this script."""

UPLOAD_FILENAME = UPLOAD_PREFIX + "/{}.yaml"
"""Filename of uploaded software maturity level report."""

NRF_SAMPLE_DIR = Path(__file__).absolute().parents[3]
"""Directory containing samples/ and applications/ subfolders."""

BOARD_FILTER = lambda b: re.search(r"_nrf[0-9]+", b) and not b.startswith("nrf51")
"""Function to filter non-relevant boards."""


class MaturityLevels(IntEnum):
    """Software maturity level constants."""

    NOT_SUPPORTED = 1
    EXPERIMENTAL = 2
    SUPPORTED = 3

    def get_str(self):
        return {
            self.NOT_SUPPORTED: "Not supported",
            self.EXPERIMENTAL: "Experimental",
            self.SUPPORTED: "Supported",
        }[self]


__version__ = "0.1.0"

logger = logging.getLogger(__name__)


def upload_data(filename: str, data: dict, cc: ContainerClient) -> None:
    """Upload software maturity level report to Azure blob storage.

    Args:
        filename: Full path of uploaded file.
        data: Data to publish to Azure.
        cc: Azure container client.

    Raises:
        azure.core.exceptions.ClientAuthenticationError: Invalid access keys.
    """

    data = yaml.safe_dump(data)
    bc = cc.get_blob_client(filename)

    logger.info("Uploading software maturity level information...")
    bc.upload_blob(data, overwrite=True)
    logger.info("Upload complete")


def soc_from_kconfig(kconfig: kconfiglib.Kconfig) -> str:
    """Extract the SoC from a Kconfig object.

    Args:
        kconfig: Kconfig object containing variables.

    Returns:
        SoC base name on the form "nrf1234".
    """

    if not "CONFIG_SOC" in kconfig.variables:
        return None
    soc = kconfig.variables["CONFIG_SOC"].value
    return soc.strip('"').split("_")[0].lower()


def find_experimental_symbols(logs: List[str]) -> Dict[str, Set[str]]:
    """Search through a list of files for experimental symbols.

    In cases where experimental symbols are found, but the folder in which they
    were found does not contain a /zephyr/.config file, the symbols are
    ignored.

    Args:
        logs: List of full path to all build.log files.

    Return:
        A mapping from each SoC to a set of symbols that trigger the
        experimental warning when built with that SoC.
    """

    socs = {}
    for log in logs:
        path = Path(log)
        content = path.read_text()
        matches = re.findall(r"Experimental symbol ([A-Z0-9_]+) is enabled", content)
        if matches:
            config_path = path.parent / "zephyr" / ".config"
            if not config_path.exists():
                logger.warning(f"path '{config_path.as_posix()}' does not exist")
                continue
            kconf = kconfiglib.Kconfig(filename=config_path)
            soc = soc_from_kconfig(kconf)
            if not soc:
                continue
            for symbol in matches:
                symbol = "CONFIG_" + symbol
                if soc not in socs:
                    socs[soc] = set()
                socs[soc].add(symbol)
    return socs


def find_all_socs(all_samples: List[str]) -> List[str]:
    """Search sample.yaml files to find all SoCs currently in use.

    The ``integration_platforms`` field under common and each test is used to
    find all possible boards, and the SoCs are derived from the board names.

    Any board name that is not nRF, or any board of the nRF51 series is
    excluded from the returned set.

    Args:
        all_samples: List of full path to all sample.yaml files.

    Returns:
        A sorted list of all unique SoC names.

    Raises:
        yaml.YAMLError: Corrupted YAML file.
        FileNotFoundError: Invalid file path.
    """

    all_boards = set()
    for sample_path in all_samples:
        with open(sample_path) as sample:
            content = yaml.safe_load(sample)

        # Add common integration platforms
        common_platforms = []
        if "common" in content and "integration_platforms" in content["common"]:
            common_platforms = content["common"]["integration_platforms"]

        # Add each test's integration platforms
        for test in content["tests"].values():
            integration_platforms = common_platforms
            if "integration_platforms" in test:
                integration_platforms = test["integration_platforms"]
            all_boards.update(integration_platforms)

    # Only use nRF boards, but not nRF51 series
    all_boards = set(filter(BOARD_FILTER, all_boards))
    all_socs = {re.findall(r"_(nrf[0-9]+)", b)[-1] for b in all_boards}
    return list(sorted(all_socs))


def find_enclosing_parenthesis(rule: str) -> int:
    """Get string length of a subrule enclosed in parenthesis.

    Args:
        rule: Kconfig rule starting with a parenthesis.

    Returns:
        Number of characters in the subrule.
    """

    level = 0
    for i, char in enumerate(rule):
        if char == "(":
            level += 1
        elif char == ")":
            level -= 1
        if level == 0:
            return i + 1

    logger.error(f"Invalid Kconfig rule '{rule}': parenthesis missing")


def split_on(delimiter: str, rule: str) -> List[str]:
    """Like regular split, but ignore delimiters in subrules.

    Args:
        delimiter: Substring to split on ('||' or '&&').
        rule: Kconfig rule to split.

    Returns:
        List of sub-expressions delimited by ``delimiter``.
    """

    i = 0
    parts = []
    while i < len(rule):
        if rule[i] == "(":
            i += find_enclosing_parenthesis(rule[i:])
        elif rule[i:].startswith(delimiter):
            parts.append(rule[0:i].strip())
            rule = rule[i + len(delimiter) :]
            i = 0
        else:
            i += 1
    parts.append(rule.strip())
    return parts


def parse_rule(rule: str) -> list:
    """Parse a Kconfig rule and return a nested list of expressions.

    A Kconfig rule can be described by the following CFG:
    rule -> or-rule
    or-rule -> and-rule | and-rule '||' or-rule
    and-rule -> sub-rule | sub-rule '&&' and-rule
    sub-rule -> not-symbol | '(' or-rule ')'
    not-symbol -> symbol | '!' symbol
    symbol -> any matches of the regular expression '[A-Z0-9_=]+'

    And-expressions have a higher precedence than or-expressions, making the
    rule 'A || B && C' equivalent with 'A || (B && C)'

    Each list in the returned nested list of expressions is prefixed by the
    'AND' or 'OR' keyword.

    For example, the rule 'A && (B || C)' will return the following list:
    [OR, [AND, A, [OR, [AND, B], [AND, C]]]]

    Every symbol can be negated. The rule 'A && !B' will return the following
    list: [OR, [AND, A, (NOT B)]]. Only symbols can be negated, not expressions.

    Args:
        rule: Kconfig rule string.

    Returns:
        List of or-expressions with parsed sub-expressions.
    """

    and_expressions = split_on("||", rule)
    parsed_rule = ["OR"]
    for expression in and_expressions:
        symbols = split_on("&&", expression)
        for i, symbol in enumerate(symbols):
            # Symbol is a nested expression
            if symbol.startswith("(") and symbol.endswith(")"):
                symbols[i] = parse_rule(symbol[1:-1])
                continue

            negation = False
            if not re.match(r"^(!\s*)?[A-Z0-9_=]+$", symbol):
                logger.error(f"Invalid Kconfig symbol '{symbol}'")
            if symbol.startswith("!"):
                negation = True
                symbol = symbol[1:].strip()
            if not symbol.startswith("CONFIG_"):
                symbol = "CONFIG_" + symbol
            symbols[i] = ("NOT", symbol) if negation else symbol
        symbols.insert(0, "AND")
        parsed_rule.append(symbols)
    return parsed_rule


def evaluate_rule(vars: Dict[str, kconfiglib.Variable], rule: list) -> bool:
    """Evaluate a parsed Kconfig rule against a dictionary of Kconfig variables.

    Args:
        vars: Dictionary of present Kconfig variables and their values.
        rule: Parsed Kconfig rule, i.e. list of or-expressions.

    Returns:
        False if any of the required variables are missing, True otherwise.
    """

    assert rule[0] == "OR", f"invalid or-rule {rule}"
    # At least one of the and-rules must evaluate to True
    for and_rule in rule[1:]:
        assert and_rule[0] == "AND", f"invalid and-rule {and_rule}"
        # All of the symbols in the and-rule must be present
        for symbol_and_value in and_rule[1:]:
            # Extract the required value (default: "y")
            if "=" in symbol_and_value:
                symbol, value = symbol_and_value.split("=")
            else:
                symbol, value = symbol_and_value, "y"

            # A nested list indicates a subrule in parentheses
            if isinstance(symbol, list):
                if not evaluate_rule(vars, symbol):
                    break
            # The and-rule fails if a negated symbol is present
            elif isinstance(symbol, tuple):
                assert (
                    len(symbol) == 2 and symbol[0] == "NOT"
                ), f"invalid negation {symbol}"
                symbol = symbol[1]
                if symbol in vars and vars[symbol].value == value:
                    break
            # The and-rule fails if a symbol is not present
            elif symbol not in vars or vars[symbol].value != value:
                break
        # No breaks in for-loop means all symbols are present
        else:
            return True
    return False


def flatten(items: list) -> Set[str]:
    """Flatten a Kconfig rule to a set of unique symbols.

    Args:
        items: Nested list of symbols.

    Returns:
        Set of unique symbols.
    """

    out = set()
    for item in items:
        if item in ["OR", "AND"]:
            continue
        elif isinstance(item, list):
            out.update(flatten(item))
        else:
            out.add(item)
    return out


def find_maturity_level(
    vars: Dict[str, kconfiglib.Variable], experimental_symbols: Set[str]
) -> MaturityLevels:
    """Find if any of the symbols used in a rule are experimental or not.

    Args:
        vars: Dictionary of present Kconfig variables and their values.
        experimental_symbols: A list of the experimental symbols used in the samples.

    Returns:
        'Experimental' | 'Supported'
    """

    for symbol in experimental_symbols:
        if evaluate_rule(vars, ["OR", ["AND", symbol]]):
            return MaturityLevels.EXPERIMENTAL
    return MaturityLevels.SUPPORTED


def generate_tables(
    build_output: Path,
    input_file: Path,
    save_output: Path = None,
    publish_results: bool = False,
    access_key: str = "",
    commit_sha: str = "",
) -> None:
    """Entry point of software maturity level scanning.

    Read all SoCs from sample.yaml files, find all .config files and evaluate
    Kconfig rules given in ``input_file`` against them, and find if any of the
    relevant Kconfig symbols for a feature is experimental or not by scanning
    the build.log files.

    At least on of the arguments ``publish_results`` or ``save_output`` must
    have a truthy value for a report of the results to be generated.

    Args:
        build_output: Path to the twister build output.
        input_file: Path to the input YAML file containing features and
                    their corresponding Kconfig rules.
        save_output: Path to the generated YAML report. If None, no report
                     is generated.
        publish_results: If True, the report is stored in Azure blob storage.
                         This requires ``access_key`` and ``commit_sha`` to be
                         set to function.
        access_key: Azure access key to write to blob storage.
        commit_sha: SHA of the commit the twister output is generated by.
    """

    # Directory sanity check
    sample_dir = NRF_SAMPLE_DIR
    if not build_output.exists():
        sys.exit("Invalid output directory")
    elif (
        not (sample_dir / "samples").exists()
        or not (sample_dir / "applications").exists()
    ):
        sys.exit(f"samples/ or applications/ cannot be found in {sample_dir}")

    # Read input file
    try:
        with open(input_file) as f:
            input_doc = yaml.safe_load(f)
    except FileNotFoundError as fnfe:
        sys.exit(f"Could not find input file '{fnfe.filename}'")
    except yaml.YAMLError as yamlerr:
        sys.exit(f"Invalid or corrupt input file: {yamlerr}")

    # Create a dictionary of all unique features and their respective rule
    all_features = {}
    feature_categories = input_doc["features"]
    for cat, features in feature_categories.items():
        for feature, rule in features.items():
            key = f"{cat}_{feature}"
            all_features[key] = parse_rule(rule)

    # Find all sample.yaml files
    all_samples = list(sample_dir.glob("samples/**/sample.yaml"))
    all_samples += list(sample_dir.glob("applications/**/sample.yaml"))

    # Find all SoCs from integration_platforms in sample.yaml
    all_socs = find_all_socs(all_samples)

    # Check which symbols are experimental by looking through build.log files
    build_logs = list(build_output.glob("**/build.log"))
    exp_soc_sets = find_experimental_symbols(build_logs)

    # Each feature is associated with a dictionary mapping each supported SoC
    # to its state (Experimental/Supported)
    soc_sets = {}
    top_table_info = input_doc["top_table"]
    for tech in top_table_info:
        soc_sets[tech] = set()

    for feature in all_features:
        soc_sets[feature] = {}

    # Look for the desired symbol combinations in all .config files
    all_conf_files = build_output.glob("**/zephyr/.config")
    for conf_file in all_conf_files:
        kconf = kconfiglib.Kconfig(filename=conf_file)
        soc = soc_from_kconfig(kconf)
        if not soc:
            continue

        # Look for technology symbols
        for tech, rule in top_table_info.items():
            rule = parse_rule(rule)
            # Top level technologies are currently only Supported/Not supported
            if evaluate_rule(kconf.variables, rule):
                soc_sets[tech].add(soc)

        # Look for feature symbols
        for feature, rule in all_features.items():
            if evaluate_rule(kconf.variables, rule):
                experimental_symbols = (
                    exp_soc_sets[soc] if soc in exp_soc_sets else set()
                )
                status = find_maturity_level(kconf.variables, experimental_symbols)
                soc_sets[feature][soc] = (
                    max(status, soc_sets[feature][soc])
                    if soc in soc_sets[feature]
                    else status
                )

    # Create the output data structure
    top_table = {}
    for tech in sorted(top_table_info):
        top_table[tech] = {
            MaturityLevels.SUPPORTED.get_str(): [],
            MaturityLevels.EXPERIMENTAL.get_str(): [],
        }
        for soc in all_socs:
            if soc in soc_sets[tech]:
                top_table[tech][MaturityLevels.SUPPORTED.get_str()].append(soc)

    # Feature tables
    category_results = {}
    for cat in sorted(feature_categories):
        features = feature_categories[cat]
        table = {}

        for feature_name in sorted(features):
            feature = f"{cat}_{feature_name}"
            table[feature] = {
                MaturityLevels.SUPPORTED.get_str(): [],
                MaturityLevels.EXPERIMENTAL.get_str(): [],
            }

            for soc in all_socs:
                if soc in soc_sets[feature]:
                    status = soc_sets[feature][soc].get_str()
                    table[feature][status].append(soc)

        category_results[cat] = table

    output = {
        "all_socs": all_socs,
        "top_level": top_table,
        "features": category_results,
    }

    # Save to local file
    if save_output:
        with open(save_output, "w") as f:
            yaml.safe_dump(output, f)

    # Upload to Azure
    if publish_results:
        cc = ContainerClient.from_connection_string(
            AZ_CONN_STR.format(access_key), AZ_CONTAINER
        )

        filename = UPLOAD_FILENAME.format(commit_sha)
        upload_data(filename, output, cc)


if __name__ == "__main__":
    build_output = os.environ.get("TWISTER_OUT_DIR", None)
    access_key = os.environ.get("AZURE_REPORT_CACHE_ACCESS_KEY", None)

    parser = argparse.ArgumentParser(allow_abbrev=False)

    parser.add_argument(
        "-bo",
        "--build-output",
        default=build_output,
        type=Path,
        help="Twister output directory. "
        "The environment variable TWISTER_OUT_DIR is used by default.",
    )

    parser.add_argument(
        "-f",
        "--input-file",
        default=Path(__file__).parent / "software_maturity_features.yaml",
        type=Path,
        help="Input YAML file name. "
        "Defaults to look for 'software_maturity_features.yaml' in the local directory.",
    )

    parser.add_argument(
        "-o",
        "--save-output",
        type=Path,
        default=None,
        help="If not None, save results to the specified local file.",
    )

    parser.add_argument(
        "--publish-results",
        action="store_true",
        help="Store the results in Azure blob storage. "
        "This should only be done by the CI build system. "
        "Requires the --access-key option.",
    )

    parser.add_argument(
        "-a",
        "--access-key",
        default=access_key,
        type=str,
        help="Azure access key. "
        "The environment variable AZURE_REPORT_CACHE_ACCESS_KEY is used by default.",
    )

    parser.add_argument(
        "-sha",
        "--commit-sha",
        default=None,
        type=str,
        help="Git SHA of the commit used to produce the build output.",
    )

    args = parser.parse_args()

    if not args.save_output and not args.publish_results:
        sys.exit(
            "Must be run with one of the following options:\n"
            "--save-output or --publish-results.\n"
            "Use --help for more information."
        )
    elif args.publish_results and not args.access_key:
        sys.exit(
            "Azure access key not found, please supply it using the"
            "--access-key option or set the AZURE_REPORT_CACHE_ACCESS_KEY"
            "environment variable and retry."
        )
    elif args.publish_results and not args.commit_sha:
        sys.exit(
            "Git SHA of the commit used to produce the build output "
            "must be supplied to publish the results."
        )

    if not args.build_output:
        sys.exit(
            "twister output directory not provided, please provide it "
            "as an argument or set the TWISTER_OUT_DIR environment "
            "variable and retry."
        )

    generate_tables(
        args.build_output,
        args.input_file,
        args.save_output,
        args.publish_results,
        args.access_key,
        args.commit_sha,
    )
