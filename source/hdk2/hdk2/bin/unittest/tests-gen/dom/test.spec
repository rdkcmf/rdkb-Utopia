test-case: server_module
    cmd: ../../../hdkmod.py -b actual --noid 9f379b4e-4048-45fd-9102-54a0976bbedb test.hsl
    # This cmd should produce an error output and not generate any files (hence no diffs)

test-case: server_methods
    cmd: ../../../hdkmod.py -b actual --server-methods --noid 9f379b4e-4048-45fd-9102-54a0976bbedb test.hsl
    # This cmd should produce an error output and not generate any files (hence no diffs)

test-case: cpp_client
    cmd: ../../../hdkcli_cpp.py -b actual_client --module-name=actual_client_mod --no-inline test.hsl
    # This cmd should produce an error output and not generate any files (hence no diffs)

test-case: dom
    cmd: ../../../hdkmod.py -b actual_dom --dom-schema --noid 9f379b4e-4048-45fd-9102-54a0976bbedb test.hsl
    diff
        actual: actual_dom.h
        expected: expected_dom.h
    diff
        actual: actual_dom.c
        expected: expected_dom.c

test-case: cpp_client_dom
    cmd: ../../../hdkcli_cpp.py -b actual_client_dom --module-name=actual_client_dom_mod --dom-schema --no-inline test.hsl
    diff
        actual: actual_client_dom.h
        expected: expected_client_dom.h
    diff
        actual: actual_client_dom.cpp
        expected: expected_client_dom.cpp

test-case: cpp_client_dom_inline
    cmd: ../../../hdkcli_cpp.py -b actual_client_dom_inline --module-name=actual_client_dom_mod --dom-schema test.hsl
    diff
        actual: actual_client_dom_inline.h
        expected: expected_client_dom_inline.h
    diff
        actual: actual_client_dom_inline.cpp
        expected: expected_client_dom_inline.cpp

test-case: wsdl
    cmd: ../../../hdkwsdl.py -b actual test.hsl
    # This cmd should produce an error output and not generate any files (hence no diffs)

test-case: empty_dom
    cmd: ../../../hdkmod.py -b actual_empty_dom --dom-schema --noid 9f379b4e-4048-45fd-9102-54a0976bbedb empty.hsl
    diff
        actual: actual_empty_dom.h
        expected: expected_empty_dom.h
    diff
        actual: actual_empty_dom.c
        expected: expected_empty_dom.c
