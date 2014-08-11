test-case: Register-New
    diff-state: true

test-case: Register-Update
    diff-state: true
    start-state: actual/Register-New.ds

test-case: Register-NoState
    diff-state: true
    start-state: NoState.ds
    request: Register-New.request