test-case: server
    cmd: ../../../hdkmod.py -b actual --noid 6c4d0bba-0679-46de-a43b-9da35b3ba1e4 test.hsl
    diff
        actual: actual.h
        expected: expected.h
    diff
        actual: actual.c
        expected: expected.c

test-case: server_methods
    cmd: ../../../hdkmod.py -b actual --server-methods --noid 6c4d0bba-0679-46de-a43b-9da35b3ba1e4 test.hsl
    # Currently the server_methods.c file is not compared against an expected file

test-case: cpp_client
    cmd: ../../../hdkcli_cpp.py -b actual_client --module-name=actual_client_mod --no-inline test.hsl
    diff
        actual: actual_client.h
        expected: expected_client.h
    diff
        actual: actual_client.cpp
        expected: expected_client.cpp

test-case: dom
    cmd: ../../../hdkmod.py -b actual_dom --dom-schema --noid 6c4d0bba-0679-46de-a43b-9da35b3ba1e4 test.hsl
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

test-case: wsdl
    cmd: ../../../hdkwsdl.py -b actual test.hsl
    diff
        actual: actual.wsdl
        expected: expected.wsdl

test-case: server_action_exclude
    cmd: ../../../hdkmod.py -b actual_server_action_exclude --action-exclude CiscoAction test.hsl
    diff
        actual: actual_server_action_exclude.h
        expected: expected_server_action_exclude.h
    diff
        actual: actual_server_action_exclude.c
        expected: expected_server_action_exclude.c

test-case: server_bad_action_exclude
    cmd: ../../../hdkmod.py -b actual_server_bad_action_exclude --action-exclude FooAction test.hsl

test-case: server_event_exclude
    cmd: ../../../hdkmod.py -b actual_server_event_exclude --event-exclude CiscoEvent test.hsl
    diff
        actual: actual_server_event_exclude.h
        expected: expected_server_event_exclude.h
    diff
        actual: actual_server_event_exclude.c
        expected: expected_server_event_exclude.c

test-case: server_bad_event_exclude
    cmd: ../../../hdkmod.py -b actual_server_bad_event_exclude --event-exclude FooEvent test.hsl

test-case: client_module
    cmd: ../../../hdkmod.py -b actual_client_module --client test.hsl
    diff
        actual: actual_client_module.h
        expected: expected_client_module.h
    diff
        actual: actual_client_module.c
        expected: expected_client_module.c
