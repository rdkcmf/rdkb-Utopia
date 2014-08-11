# tests/MACFilters2

test-case: get0
    request: get.request

test-case: set-disabled
    end-state: disable.ds

test-case: get-disabled
    request: get.request
    start-state: actual/disable.ds

test-case: set-exclude
    end-state: exclude.ds

test-case: get-exclude
    request: get.request
    start-state: actual/exclude.ds

test-case: set-clear-macinfo
    end-state: clear-macinfo.ds

test-case: get-clear-macinfo
    request: get.request
    start-state: actual/clear-macinfo.ds

test-case: set-one-macinfo
    end-state: one-macinfo.ds

test-case: get-one-macinfo
    request: get.request
    start-state: actual/one-macinfo.ds

test-case: set-many-macinfo
    end-state: many-macinfo.ds

test-case: get-many-macinfo
    request: get.request
    start-state: actual/many-macinfo.ds

test-case: set-duplicate-bad