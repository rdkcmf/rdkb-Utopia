test-case: sections
    cmd: ../../../hdkdocs.py -o actual_sections.html test_sections.hsl
    diff
        actual: actual_sections.html
        expected: expected_sections.html

test-case: sections_bad
    cmd: ../../../hdkdocs.py -o actual_sections_bad.html test_sections_bad.hsl

test-case: error_unmatchedtag
    cmd: ../../../hdkdocs.py -o actual_error_unmatchedtag.html test_error_unmatchedtag.hsl

test-case: simple
    cmd: ../../../hdkdocs.py -o actual_simple.html test_simple.hsl
    diff
        actual: actual_simple.html
        expected: expected_simple.html

test-case: event
    cmd: ../../../hdkdocs.py -o actual_event.html test_event.hsl
    diff
        actual: actual_event.html
        expected: expected_event.html
