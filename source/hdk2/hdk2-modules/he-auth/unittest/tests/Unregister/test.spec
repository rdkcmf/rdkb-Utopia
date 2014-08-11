test-case: Unregister-1
    diff-state: true

test-case: Unregister-2
    diff-state: true
    start-state: actual/Unregister-1.ds

test-case: Unregister-3
    diff-state: true
    start-state: actual/Unregister-2.ds

test-case: Unregister-Bad
    diff-state: true
    start-state: actual/Unregister-3.ds
    request: Unregister-3.request
