# tests/DeviceSettings2

test-case: get

test-case: set-all
    diff-state: true
    end-state: set-all.ds

test-case: get-all
    request: get.request
    start-state: actual/set-all.ds

test-case: set-username_not_allowed
    start-state: username_not_allowed.ds

test-case: set-timezone_not_allowed
    start-state: timezone_not_allowed.ds

test-case: set-ssl_disable_allowed
    request: set-ssl.request
    start-state: ssl_disable_allowed.ds

test-case: set-ssl_disable_not_allowed
    request: set-ssl.request
    start-state: ssl_disable_not_allowed.ds