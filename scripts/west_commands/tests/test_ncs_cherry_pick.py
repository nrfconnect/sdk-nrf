# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

import subprocess
import unittest
from pathlib import Path


def find_west_workspace() -> str | None:
    root = Path(__file__).parents[4]
    config = root / '.west' / 'config'
    if config.exists():
        return str(root)
    else:
        return None


# Each test: [[argv], [cherry pick summary lines], [manual pick lines]]
fake_repo_tests = [
    [
        # Cherry pick only 0002 which conflicts with a single downstream commit, but does
        # not depend on any missing upstream commit since 0000 is already cherry picked.
        [
            'west',
            'ncs-cherry-pick',
            '--test',
            '-p',
            'testbranch',
            '--commit',
            '0002000000000000000000000000000000000000',
        ],
        [
            'revert 0101000000000000 # modify a more downstream',
            'pick 0002000000000000 # modify a',
        ],
        [
            'pick 01010000000000000000 # [nrf noup] modify a more downstream',
        ],
    ],
    [
        # Commits 0200 and 0201 from PR1 depend on 0002 and 0004. 0001 touches the same
        # files as 0004 so it will be cherry picked even though it actually is not needed.
        [
            'west',
            'ncs-cherry-pick',
            '--test',
            '-p',
            'testbranch',
            '--pr-number',
            '1',
        ],
        [
            'revert 0101000000000000 # modify a more downstream',
            'pick 0001000000000000 # modify b',
            'pick 0002000000000000 # modify a',
            'pick 0004000000000000 # modify a more and add to b',
            'pick 0200000000000000 # modify a and b more',
            'pick 0201000000000000 # modify a and b more more',
        ],
        [
            'pick 01010000000000000000 # [nrf noup] modify a more downstream',
        ],
    ],
    [
        # 0300 from PR2 depends on 0003, so it will be included along with the commits
        # from above test.
        [
            'west',
            'ncs-cherry-pick',
            '--test',
            '-p',
            'testbranch',
            '--pr-number',
            '1',
            '--pr-number',
            '2',
        ],
        [
            'revert 0101000000000000 # modify a more downstream',
            'pick 0001000000000000 # modify b',
            'pick 0002000000000000 # modify a',
            'pick 0003000000000000 # create c',
            'pick 0004000000000000 # modify a more and add to b',
            'pick 0300000000000000 # modify c',
            'pick 0200000000000000 # modify a and b more',
            'pick 0201000000000000 # modify a and b more more',
        ],
        [
            'pick 01010000000000000000 # [nrf noup] modify a more downstream',
        ],
    ],
    [
        # 0300 from PR2 depends only on 0003, and 0003 does not conflict with 0101 so
        # it will not be reverted.
        [
            'west',
            'ncs-cherry-pick',
            '--test',
            '-p',
            'testbranch',
            '--pr-number',
            '2',
        ],
        [
            'pick 0003000000000000 # create c',
            'pick 0300000000000000 # modify c',
        ],
        [],
    ],
    [
        # Two commit args in reverse order.
        [
            'west',
            'ncs-cherry-pick',
            '--test',
            '-p',
            'testbranch',
            '--commit',
            '0003000000000000000000000000000000000000',
            '--commit',
            '0002000000000000000000000000000000000000',
        ],
        [
            'revert 0101000000000000 # modify a more downstream',
            'pick 0002000000000000 # modify a',
            'pick 0003000000000000 # create c',
        ],
        [
            'pick 01010000000000000000 # [nrf noup] modify a more downstream',
        ],
    ],
    [
        # Rebase branch. Expect 0404 to be cherry picked back at the end, and have
        # 0403 to be promoted to fromtree, by cherry picking 0003.
        [
            'west',
            'ncs-cherry-pick',
            '--test',
            'testbranch',
        ],
        [
            'revert 0101000000000000 # modify a more downstream',
            'pick 0002000000000000 # modify a',
            'pick 0003000000000000 # create c',
            'pick 0300000000000000 # modify c',
            'pick 0404000000000000 # modify a more downstream',
        ],
        [
            'pick 01010000000000000000 # [nrf noup] modify a more downstream',
        ],
    ],
]


@unittest.skipUnless(find_west_workspace() is not None, 'Not in west workspace')
class TestNcsCherryPick(unittest.TestCase):
    def test_fake_repo(self):
        cwd = find_west_workspace()

        for test in fake_repo_tests:
            cmd = test[0]
            result = subprocess.run(cmd, capture_output=True, cwd=cwd)
            self.assertEqual(result.returncode, 0)
            lines = result.stdout.decode().splitlines()
            applied = self._get_applied_(lines)
            dropped = self._get_dropped_(lines)
            self.assertListEqual(applied, test[1])
            self.assertListEqual(dropped, test[2])

    def _get_applied_(self, lines) -> list[str]:
        keep = False
        applied = []

        for line in lines:
            if keep:
                if line == '':
                    return applied

                applied.append(line)
                continue

            if line == 'Cherry pick summary':
                keep = True

        return applied

    def _get_dropped_(self, lines) -> list[str]:
        keep = False
        dropped = []

        for line in lines:
            if keep:
                dropped.append(line)
                continue

            if line == 'The following commits must be cherry picked manually':
                keep = True

        return dropped


if __name__ == '__main__':
    unittest.main()
