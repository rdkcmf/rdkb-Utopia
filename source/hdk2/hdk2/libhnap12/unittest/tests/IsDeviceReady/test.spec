# tests/IsDeviceReady

test-case: get-ready
    request: IsDeviceReady.request
    start-state: ready.ds

test-case: get-not-ready
    request: IsDeviceReady.request
    start-state: not-ready.ds
