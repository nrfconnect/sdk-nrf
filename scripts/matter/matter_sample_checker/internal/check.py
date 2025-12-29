#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from abc import abstractmethod
from dataclasses import dataclass
from pathlib import Path

from internal.utils.defines import LEVELS, MatterSampleCheckerResult


@dataclass
class MatterCheckerConfig:
    config_file: dict
    sample_path: Path
    nrf_path: Path
    verbose: bool = False
    skip: bool = False
    expected_years: list[int] | None = None
    allowed_names: list[str] | None = None


class MatterSampleTestCase:
    def __init__(self):
        self.config: MatterCheckerConfig
        self.sample_name = ""
        self.result: list[MatterSampleCheckerResult] = []

    def run(self, config: MatterCheckerConfig) -> list[MatterSampleCheckerResult]:
        """
        Run the test case.

        This method is called to run the test case.
        It will prepare the test case, run the check and return the result.
        """
        # save config
        self.config = config
        self.sample_name = config.sample_path.name if config.sample_path else ""

        # Prepare the test case
        self.prepare()

        # display the name of the check
        self.info(f"\n=== {self.name()} check ===")
        if self.config.skip:
            self.info("✅ Check skipped")
            return self.result
        try:
            # run the check
            self.check()
        except Exception as e:
            self.issue(str(e))
        finally:
            if (
                len([result for result in self.result if result.level == LEVELS["issue"]]) == 0
                and len([result for result in self.result if result.level == LEVELS["warning"]])
                == 0
            ):
                self.info("✅ No issues found")
            if len([result for result in self.result if result.level == LEVELS["issue"]]) > 0:
                self.info(
                    f"❌ {
                        len([result for result in self.result if result.level == LEVELS['issue']])
                    } Issues found"
                )
            if len([result for result in self.result if result.level == LEVELS["warning"]]) > 0:
                self.info(
                    f"⚠️ {
                        len([result for result in self.result if result.level == LEVELS['warning']])
                    } Warnings found"
                )
        return self.result

    def issue(self, message: str):
        self.result.append(MatterSampleCheckerResult(level=LEVELS["issue"], message=message))

    def warning(self, message: str):
        self.result.append(MatterSampleCheckerResult(level=LEVELS["warning"], message=message))

    def info(self, message: str):
        self.result.append(MatterSampleCheckerResult(level=LEVELS["info"], message=message))

    def debug(self, message: str):
        self.result.append(MatterSampleCheckerResult(level=LEVELS["debug"], message=message))

    @abstractmethod
    def prepare(self):
        """
        Prepare the test case.

        This method is called before running the check.
        Implement here all the logic and variables that are needed for the check,
        but are not part of the check body
        """
        raise NotImplementedError

    @abstractmethod
    def name(self) -> str:
        """
        Get the name of the test case.

        This name will be displayed in the output.
        """
        raise NotImplementedError

    @abstractmethod
    def check(self):
        """
        Run the check.

        This method is called to run the check.
        Implement here the logic of the check.

        Return a MatterSampleTestCaseResult object with the issues, warnings, info and debug lists.
        """
        raise NotImplementedError
