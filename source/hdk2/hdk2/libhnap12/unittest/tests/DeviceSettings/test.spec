# tests/DeviceSettings

test-case: get

test-case: set-all
    end-state: set-all.ds
    diff-state: true

test-case: get-all
    request: get.request
    start-state: actual/set-all.ds
