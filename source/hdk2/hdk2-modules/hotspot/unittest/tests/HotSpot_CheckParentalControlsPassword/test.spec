test-case: CheckParentalControlsPassword

test-case: CheckParentalControlsPassword-Incorrect

test-case: CheckParentalControlsPassword-Unset
    request: CheckParentalControlsPassword.request
    start-state: hotspot-unset.ds

test-case: CheckParentalControlsPassword-Unset-Incorrect
    request: CheckParentalControlsPassword-Incorrect.request
    start-state: hotspot-unset.ds
