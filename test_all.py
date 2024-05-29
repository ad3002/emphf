#!/usr/bin/env python3

import os
import sys
from subprocess import check_call

DEFAULT_FILE = 'test.dat'

EXES = [
    ('compute_mphf_seq', 'test_mphf'),
    ('compute_mphf_scan', 'test_mphf'),
    ('compute_mphf_scan_mmap', 'test_mphf'),
    ('compute_mphf_hem', 'test_mphf_hem'),
]

def main(argv):
    if len(argv) == 1:
        filename = DEFAULT_FILE
        print(f"Using default file {filename}", file=sys.stderr)
        print("To use another file:", file=sys.stderr)
        print(f"\t{argv[0]} <filename>", file=sys.stderr)
    else:
        filename = argv[1]
        print(f"Using file {filename}", file=sys.stderr)

    for constructor, tester in EXES:
        print(file=sys.stderr)
        print(f"{'=' * 4} Testing {constructor} {'=' * 40}", file=sys.stderr)
        mphf_name = 'test.pf'

        check_call(['./' + constructor, filename, mphf_name])
        check_call(['./' + tester, filename, mphf_name, '--check'])
        os.remove(mphf_name)

if __name__ == '__main__':
    main(sys.argv)
