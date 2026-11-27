# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Regenerate the Hard Real-Time (HRT) assembly files of High-Performance Framework (HPF)
applications and install them in the application source tree.

For every HPF application and every SoC listed in the application's sample.yaml file,
the FLPR image is configured without sysbuild in the HPF developer mode and the asm_gen
target is built.
Each generated <name>-<soc>-temp.s file is compared with the <name>-<soc>.s file located
next to the <name>.c HRT source file, which is replaced if the content differs.

By default, all applications in the applications/hpf directory of the nrf repository
are processed. The assembly files must be generated with the Zephyr SDK version
required by Zephyr (see zephyr/SDK_VERSION).

Examples:

    west ncs-hpf-asm-update
    west ncs-hpf-asm-update --check
    west ncs-hpf-asm-update nrf/applications/hpf/mspi --soc nrf54l15
"""

import argparse
import shutil
import subprocess
import sys
import tempfile
import threading
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Any

import yaml
from west.commands import WestCommand

NRF_DIR = Path(__file__).resolve().parents[2]
HPF_APPS_DIR = NRF_DIR / 'applications' / 'hpf'
HPF_ASM_MARKER = 'hpf_assembly_install('
TEMP_SUFFIX = '-temp.s'
HRT_SOURCES_FILE = 'hpf_asm_sources.txt'


@dataclass(frozen=True)
class Target:
    app: Path
    board: str
    soc: str
    snippets: tuple[str, ...] = ()


class Status(Enum):
    UNCHANGED = 'unchanged'
    UPDATED = 'updated'
    ADDED = 'added'
    DIFFERS = 'differs'
    MISSING = 'missing'
    FAILED = 'failed'


@dataclass
class AsmResult:
    target: Target
    status: Status
    asm_file: Path | None = None
    detail: str = ''


def is_hpf_app(app_dir: Path) -> bool:
    cmake_file = app_dir / 'CMakeLists.txt'
    return cmake_file.is_file() and HPF_ASM_MARKER in cmake_file.read_text(encoding='utf-8')


def find_hpf_apps(root: Path) -> list[Path]:
    if not root.is_dir():
        return []
    return sorted(d for d in root.iterdir() if d.is_dir() and is_hpf_app(d))


def soc_from_board(board: str) -> str:
    parts = board.split('/')
    return parts[1] if len(parts) >= 2 else board


def _as_list(value: Any) -> list[str]:
    if value is None:
        return []
    if isinstance(value, str):
        return value.split()
    return [str(v) for v in value]


def _merged(common: dict[str, Any], scenario: dict[str, Any], key: str) -> list[str]:
    values = _as_list(common.get(key)) + _as_list(scenario.get(key))
    return list(dict.fromkeys(values))


def targets_from_sample_yaml(app: Path) -> list[Target]:
    """Return one target per SoC, using the first scenario that lists a platform with it."""
    sample_file = app / 'sample.yaml'
    if not sample_file.is_file():
        return []

    data = yaml.safe_load(sample_file.read_text(encoding='utf-8')) or {}
    common = data.get('common') or {}
    targets: dict[str, Target] = {}

    for scenario in (data.get('tests') or {}).values():
        scenario = scenario or {}
        snippets = tuple(_merged(common, scenario, 'required_snippets'))
        for board in _merged(common, scenario, 'platform_allow'):
            soc = soc_from_board(board)
            targets.setdefault(soc, Target(app, board, soc, snippets))

    return list(targets.values())


def build_command(target: Target, build_dir: Path) -> list[str]:
    cmd = [
        sys.executable,
        '-m',
        'west',
        'build',
        '--pristine',
        'always',
        '--no-sysbuild',
        '--board',
        target.board,
        '--build-dir',
        str(build_dir),
        '--target',
        'asm_gen',
    ]
    for snippet in target.snippets:
        cmd += ['--snippet', snippet]
    # In user mode, the build requires <name>-<soc>.s to already exist in the source tree.
    cmd += [str(target.app), '--', '-DCONFIG_HPF_DEVELOPER_MODE=y']
    return cmd


def read_cmake_cache(build_dir: Path) -> dict[str, str]:
    entries = {}
    for line in (build_dir / 'CMakeCache.txt').read_text(encoding='utf-8').splitlines():
        if not line or line.startswith(('#', '//')):
            continue
        key, sep, value = line.partition('=')
        if sep:
            entries[key.split(':', 1)[0]] = value
    return entries


def read_kconfig(build_dir: Path, name: str) -> str | None:
    config_file = build_dir / 'zephyr' / '.config'
    if not config_file.is_file():
        return None
    for line in config_file.read_text(encoding='utf-8').splitlines():
        if line.startswith(f'{name}='):
            return line.split('=', 1)[1].strip('"')
    return None


def toolchain_issue(cache: dict[str, str]) -> str | None:
    """Return a description of the toolchain mismatch, or None if it matches Zephyr."""
    variant = cache.get('ZEPHYR_TOOLCHAIN_VARIANT')
    if variant != 'zephyr':
        return f"toolchain variant is '{variant}', expected the Zephyr SDK ('zephyr')"

    sdk_version_file = Path(cache.get('ZEPHYR_BASE', '')) / 'SDK_VERSION'
    if not sdk_version_file.is_file():
        return f'cannot read the required Zephyr SDK version from {sdk_version_file}'
    expected = sdk_version_file.read_text(encoding='utf-8').strip()

    sdk_dir = cache.get('ZEPHYR_SDK_INSTALL_DIR', '')
    installed_file = Path(sdk_dir) / 'sdk_version'
    installed = (
        installed_file.read_text(encoding='utf-8').strip() if installed_file.is_file() else None
    )
    if installed != expected:
        return (
            f'Zephyr SDK {installed or "of unknown version"} used from {sdk_dir}, '
            f'but {sdk_version_file} requires {expected}'
        )
    return None


def _normalized(data: bytes) -> bytes:
    return data.replace(b'\r\n', b'\n')


def read_hrt_sources(build_dir: Path) -> list[Path]:
    sources_file = build_dir / HRT_SOURCES_FILE
    if not sources_file.is_file():
        return []
    lines = sources_file.read_text(encoding='utf-8').splitlines()
    return [Path(line) for line in lines if line.strip()]


def update_asm_files(target: Target, build_dir: Path, soc: str, check: bool) -> list[AsmResult]:
    sources = read_hrt_sources(build_dir)
    if not sources:
        return [
            AsmResult(target, Status.FAILED, detail=f'no HRT sources listed in {HRT_SOURCES_FILE}')
        ]

    results = []
    for source in sources:
        # Same as NAME_WE in CMake.
        name = source.name.split('.', 1)[0]
        temp_file = build_dir / f'{name}-{soc}{TEMP_SUFFIX}'
        asm_file = source.parent / f'{name}-{soc}.s'
        if not temp_file.is_file():
            results.append(
                AsmResult(target, Status.FAILED, detail=f'{temp_file.name} was not generated')
            )
            continue

        content = _normalized(temp_file.read_bytes())
        exists = asm_file.is_file()
        if exists and _normalized(asm_file.read_bytes()) == content:
            status = Status.UNCHANGED
        elif check:
            status = Status.DIFFERS if exists else Status.MISSING
        else:
            asm_file.write_bytes(content)
            status = Status.UPDATED if exists else Status.ADDED
        results.append(AsmResult(target, status, asm_file))

    return results


def find_orphaned_asm_files(results: list[AsmResult]) -> list[Path]:
    """Return assembly files in the HRT directories that no processed SoC generates."""
    known = {r.asm_file for r in results if r.asm_file}
    orphans = set()
    for asm_file in known:
        name = asm_file.stem.rsplit('-', 1)[0]
        for candidate in asm_file.parent.glob(f'{name}-*.s'):
            if (
                candidate not in known
                and not candidate.name.endswith(TEMP_SUFFIX)
                and candidate.stem.rsplit('-', 1)[0] == name
            ):
                orphans.add(candidate)
    return sorted(orphans)


def find_duplicate_targets(targets: list[Target]) -> dict[tuple[Path, str], list[str]]:
    """Return board targets that would generate the same assembly files of an application."""
    boards: dict[tuple[Path, str], list[str]] = {}
    for target in targets:
        boards.setdefault((target.app, target.soc), []).append(target.board)
    return {key: value for key, value in boards.items() if len(value) > 1}


def build_paths(build_root: Path, index: int, target: Target) -> tuple[Path, Path]:
    """Return a build directory and a log file unique for the target at the given index."""
    name = f'{index:02d}-{target.app.name}-{target.soc}'
    return build_root / name, build_root / f'{name}.log'


class NcsHpfAsmUpdate(WestCommand):
    def __init__(self):
        super().__init__(
            'ncs-hpf-asm-update',
            # Keep this in sync with the string in west-commands.yml.
            'update HPF hard real-time assembly files',
            __doc__,
            accepts_unknown_args=False,
        )
        self._toolchain_checked = False
        self._output_lock = threading.Lock()

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            description=self.description,
            formatter_class=argparse.RawDescriptionHelpFormatter,
        )
        parser.add_argument(
            'apps',
            nargs='*',
            type=Path,
            metavar='APP_DIR',
            help='HPF application directory; can be repeated '
            '(default: all applications in nrf/applications/hpf)',
        )
        parser.add_argument(
            '--soc',
            action='append',
            default=[],
            help='process only the given SoC, for example nrf54l15; can be repeated',
        )
        parser.add_argument(
            '-b',
            '--board',
            action='append',
            default=[],
            help='board target to build instead of the platforms listed in sample.yaml; '
            'can be repeated',
        )
        parser.add_argument(
            '-S',
            '--snippet',
            action='append',
            default=[],
            help='snippet to apply with --board; can be repeated',
        )
        parser.add_argument(
            '--check',
            action='store_true',
            help='only report outdated assembly files without modifying the source tree; '
            'fails if any file is outdated',
        )
        parser.add_argument(
            '--list',
            action='store_true',
            help='list the applications and board targets to process and exit',
        )
        parser.add_argument(
            '--build-root',
            type=Path,
            help='directory for build directories and logs '
            '(default: a temporary directory, removed when all builds succeed)',
        )
        parser.add_argument(
            '--keep-builds',
            action='store_true',
            help='do not remove the temporary build directory',
        )
        parser.add_argument(
            '-j',
            '--jobs',
            type=int,
            default=1,
            help='number of builds to run in parallel (default: 1)',
        )
        parser.add_argument(
            '--force',
            action='store_true',
            help='proceed even if the toolchain differs from the Zephyr SDK version '
            'required by Zephyr',
        )
        return parser

    def do_run(self, args, unknown_args):
        if args.jobs < 1:
            self.die('--jobs must be at least 1')
        if args.snippet and not args.board:
            self.die('--snippet can only be used together with --board')

        apps = [app.resolve() for app in args.apps] or find_hpf_apps(HPF_APPS_DIR)
        if not apps:
            self.die(f'no HPF applications found in {HPF_APPS_DIR}')
        for app in apps:
            if not is_hpf_app(app):
                self.die(
                    f'{app} is not an HPF application, '
                    f'its CMakeLists.txt does not call {HPF_ASM_MARKER})'
                )

        targets = self._select_targets(apps, args)

        if args.list:
            for target in targets:
                snippets = f", snippets: {' '.join(target.snippets)}" if target.snippets else ''
                self.inf(f'{target.app.name}: {target.board} (SoC: {target.soc}{snippets})')
            return

        if args.build_root:
            build_root = args.build_root.resolve()
            temporary = False
        else:
            build_root = Path(tempfile.mkdtemp(prefix='ncs-hpf-asm-'))
            temporary = True
        build_root.mkdir(parents=True, exist_ok=True)

        keep_builds = args.keep_builds or not temporary
        try:
            results = self._process(targets, build_root, args)
            failed = [r for r in results if r.status is Status.FAILED]
            keep_builds = keep_builds or bool(failed)
        finally:
            if keep_builds:
                self.inf(f'Build directories and logs: {build_root}')
            else:
                shutil.rmtree(build_root, ignore_errors=True)

        self._report(results, args)

    def _report(self, results: list[AsmResult], args: argparse.Namespace) -> None:
        failed = [r for r in results if r.status is Status.FAILED]
        outdated = [r for r in results if r.status in (Status.DIFFERS, Status.MISSING)]
        changed = [r for r in results if r.status in (Status.UPDATED, Status.ADDED)]

        self.inf('---- Summary ----', colorize=True)
        for r in results:
            location = self._display_path(r.asm_file) if r.asm_file else r.detail
            self.inf(f'{r.status.value:<10} {r.target.app.name:<8} {r.target.soc:<12} {location}')

        if not args.soc and not args.board:
            for orphan in find_orphaned_asm_files(results):
                self.wrn(
                    f'{self._display_path(orphan)} is not generated for any platform '
                    'listed in sample.yaml'
                )

        if failed:
            self.die(f'{len(failed)} assembly file update(s) failed, see the logs above')
        if outdated:
            self.die(
                f'{len(outdated)} assembly file(s) are outdated, '
                f'run west {self.name} to update them'
            )
        if changed:
            self.inf(
                f'{len(changed)} assembly file(s) changed. Review the changes with git diff '
                'and verify that the HRT timing requirements are still met.'
            )

    def _select_targets(self, apps: list[Path], args: argparse.Namespace) -> list[Target]:
        targets = []
        for app in apps:
            if args.board:
                app_targets = [
                    Target(app, board, soc_from_board(board), tuple(args.snippet))
                    for board in args.board
                ]
            else:
                app_targets = targets_from_sample_yaml(app)
                if not app_targets:
                    self.die(
                        f'no platforms found in {app / "sample.yaml"}, '
                        'use --board to select the board targets'
                    )
            targets.extend(app_targets)

        if args.soc:
            unknown = sorted(set(args.soc) - {t.soc for t in targets})
            if unknown:
                available = ', '.join(sorted({t.soc for t in targets}))
                self.die(f"unknown SoC: {', '.join(unknown)} (available: {available})")
            targets = [t for t in targets if t.soc in args.soc]

        for (app, soc), boards in find_duplicate_targets(targets).items():
            self.die(
                f"board targets {', '.join(boards)} would all generate the {soc} assembly "
                f'files of {app}, select only one of them'
            )

        return targets

    def _process(
        self, targets: list[Target], build_root: Path, args: argparse.Namespace
    ) -> list[AsmResult]:
        results: list[AsmResult] = []
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            futures = [
                pool.submit(self._build, target, *build_paths(build_root, index, target))
                for index, target in enumerate(targets)
            ]
            try:
                for target, future in zip(targets, futures, strict=True):
                    build_dir, log_file, ok = future.result()
                    results.extend(self._collect(target, build_dir, log_file, ok, args))
            except BaseException:
                pool.shutdown(cancel_futures=True)
                raise
        return results

    def _build(self, target: Target, build_dir: Path, log_file: Path) -> tuple[Path, Path, bool]:
        with self._output_lock:
            self.inf(f'Generating assembly for {target.app.name} on {target.board}')
        with log_file.open('w', encoding='utf-8') as log:
            proc = subprocess.run(
                build_command(target, build_dir),
                stdout=log,
                stderr=subprocess.STDOUT,
                check=False,
            )
        return build_dir, log_file, proc.returncode == 0

    def _collect(
        self,
        target: Target,
        build_dir: Path,
        log_file: Path,
        ok: bool,
        args: argparse.Namespace,
    ) -> list[AsmResult]:
        if not ok:
            with self._output_lock:
                self.err(f'Build for {target.app.name} on {target.board} failed, log: {log_file}')
            return [AsmResult(target, Status.FAILED, detail=f'build failed, see {log_file}')]

        if not self._toolchain_checked:
            issue = toolchain_issue(read_cmake_cache(build_dir))
            if issue and not args.force:
                self.die(
                    f'{issue}. The assembly files must be generated with the Zephyr SDK '
                    'required by Zephyr, use --force to proceed anyway.'
                )
            if issue:
                self.wrn(issue)
            self._toolchain_checked = True

        soc = read_kconfig(build_dir, 'CONFIG_SOC') or target.soc
        return update_asm_files(target, build_dir, soc, args.check)

    def _display_path(self, path: Path) -> str:
        if self.topdir and path.is_relative_to(self.topdir):
            return str(path.relative_to(self.topdir))
        return str(path)
