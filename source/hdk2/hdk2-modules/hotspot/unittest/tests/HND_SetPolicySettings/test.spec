test-case: SetPolicySettings
    diff-state: true

test-case: SetPolicySettings-NoTM
    start-state: hotspot-notm.ds
    diff-state: true

test-case: GetPolicySettings
    start-state: actual/SetPolicySettings.ds

test-case: SetPolicySettings-Bad
    diff-state: true

test-case: set-policy-bad-2

test-case: set-policy-bad-3

test-case: set-status-bad

test-case: set-device-good

test-case: set-device-good-2

test-case: set-device-bad

test-case: set-device-bad-2

test-case: set-url-bad

test-case: set-url-bad-2

test-case: set-url-bad-3

test-case: set-keyword-bad

test-case: set-keyword-bad-2

test-case: set-keyword-bad-3

test-case: set-category-bad

test-case: set-schedule-bad

test-case: set-schedule-bad-2
