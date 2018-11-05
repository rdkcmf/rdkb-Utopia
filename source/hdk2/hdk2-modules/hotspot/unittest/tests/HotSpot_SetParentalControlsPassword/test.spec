test-case: HasParentalControlsPassword
    start-state: hotspot.ds

test-case: SetParentalControlsPassword
    start-state: hotspot.ds
    diff-state: true

test-case: HasParentalControlsPassword-2
    request: HasParentalControlsPassword.request
    start-state: actual/SetParentalControlsPassword.ds

test-case: CheckParentalControlsPassword
    start-state: actual/SetParentalControlsPassword.ds

test-case: CheckParentalControlsPassword-bad
    start-state: actual/SetParentalControlsPassword.ds

test-case: set-password-incorrect
    start-state: actual/SetParentalControlsPassword.ds
    diff-state: true

test-case: set-password-good
    start-state: hotspot.ds

test-case: set-password-good-2
    start-state: hotspot.ds

test-case: set-password-bad
    start-state: hotspot.ds

test-case: set-password-bad-2
    start-state: hotspot.ds

test-case: set-password-bad-3
    start-state: hotspot.ds