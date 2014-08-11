test-case: ActivateTMSSS
    diff-state: true

test-case: ActivateTMSSS-NoTM
    request: ActivateTMSSS.request
    start-state: hotspot-notm.ds

test-case: ActivateTMSSS-Bad

test-case: ActivateTMSSS-New
    request: ActivateTMSSS.request
    start-state: hotspot-new.ds
    diff-state: true
