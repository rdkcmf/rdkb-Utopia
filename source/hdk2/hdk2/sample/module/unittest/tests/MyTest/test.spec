test-case: GetSum

test-case: SetValues
    diff-state: true

test-case: GetSum2
    request: GetSum.request
    start-state: actual/SetValues.ds
