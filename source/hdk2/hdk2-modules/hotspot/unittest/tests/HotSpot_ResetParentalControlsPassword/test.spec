test-case: ResetParentalControlsPassword-Unset
    request: ResetParentalControlsPassword.request
    start-state: hotspot-unset.ds

test-case: ResetParentalControlsPassword-Unset-Incorrect
    request: ResetParentalControlsPassword-Incorrect.request
    start-state: hotspot-unset.ds

test-case: ResetParentalControlsPassword
    diff-state: true

test-case: ResetParentalControlsPassword-Incorrect
    request: ResetParentalControlsPassword-Incorrect.request
