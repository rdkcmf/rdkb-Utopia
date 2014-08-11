# tests/PortMappings

test-case: get0
    request: get.request

test-case: delete1
    end-state: port-mapping.ds

test-case: get1
    request: get.request
    start-state: actual/port-mapping.ds

test-case: delete2
    start-state: actual/port-mapping.ds
    end-state: port-mapping.ds

test-case: get2
    request: get.request
    start-state: actual/port-mapping.ds

test-case: add
    start-state: actual/port-mapping.ds
    end-state: port-mapping.ds

test-case: get3
    request: get.request
    start-state: actual/port-mapping.ds

test-case: add-no-protocol
    start-state: actual/port-mapping.ds
    end-state: port-mapping.ds

test-case: delete-not-there
    start-state: actual/port-mapping.ds
    end-state: port-mapping.ds

test-case: get4
    request: get.request
    start-state: actual/port-mapping.ds

test-case: add-duplicate-bad

test-case: add-ip-bad

test-case: add-unknown-protocol