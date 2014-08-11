test-case: module
    cmd: ../../../hdkmod.py -b actual test.hsl
    diff
        actual: actual.h
        expected: expected.h
    diff
        actual: actual.c
        expected: expected.c
