# tests/ConfigBlob

test-case: SetConfigBlob
    diff-state: true

test-case: GetConfigBlob
    start-state: actual/SetConfigBlob.ds