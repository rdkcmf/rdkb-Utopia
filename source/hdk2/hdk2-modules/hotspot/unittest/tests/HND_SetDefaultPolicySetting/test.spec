test-case: SetDefaultPolicySetting
    diff-state: true

test-case: SetDefaultPolicySetting-NoTM
    request: SetDefaultPolicySetting.request
    start-state: hotspot-notm.ds

test-case: set-policy-bad

test-case: set-policy-bad-2
