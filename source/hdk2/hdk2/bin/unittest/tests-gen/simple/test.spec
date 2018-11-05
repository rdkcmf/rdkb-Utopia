test-case: server
    cmd: ../../../hdkmod.py -b actual --noid c0785edb-2255-4a38-9302-bb129ffb2f71 test.hsl
    diff
        actual: actual.h
        expected: expected.h
    diff
        actual: actual.c
        expected: expected.c

test-case: server_location
    cmd: ../../../hdkmod.py -b actual_server_location --action-location Other test.hsl
    diff
        actual: actual_server_location.h
        expected: expected_server_location.h
    diff
        actual: actual_server_location.c
        expected: expected_server_location.c

test-case: server_noauth
    cmd: ../../../hdkmod.py -b actual_server_noauth --noauth test.hsl
    diff
        actual: actual_server_noauth.h
        expected: expected_server_noauth.h
    diff
        actual: actual_server_noauth.c
        expected: expected_server_noauth.c

test-case: server_methods
    cmd: ../../../hdkmod.py -b actual --server-methods --noid c0785edb-2255-4a38-9302-bb129ffb2f71 test.hsl
    # Currently the server_methods.c file is not compared against an expected file

test-case: cpp_client
    cmd: ../../../hdkcli_cpp.py -b actual_client --module-name=actual_client_mod --no-inline test.hsl
    diff
        actual: actual_client.h
        expected: expected_client.h
    diff
        actual: actual_client.cpp
        expected: expected_client.cpp

test-case: client_location
    cmd: ../../../hdkcli_cpp.py -b actual_client_location --module-name=actual_client_mod --no-inline --action-location Other test.hsl
    diff
        actual: actual_client_location.h
        expected: expected_client_location.h
    diff
        actual: actual_client_location.cpp
        expected: expected_client_location.cpp

test-case: dom
    cmd: ../../../hdkmod.py -b actual_dom --dom-schema --noid c0785edb-2255-4a38-9302-bb129ffb2f71 test.hsl
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

