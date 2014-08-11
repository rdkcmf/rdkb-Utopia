# tests/RouterSettings

test-case: get0
    request: get.request

test-case: set-all
    diff-state: true
    end-state: set-all.ds

test-case: get-all
    request: get.request
    start-state: actual/set-all.ds

test-case: set-manageremote-notallowed
    request: set-manageremote-disable.request
    start-state: manageremote-notallowed.ds

test-case: set-managewireless-notallowed
    request: set-managewireless-disable.request
    start-state: manageremote-notallowed.ds

test-case: set-remoteport-notallowed
    request: set-remoteport.request
    start-state: manageremote-notallowed.ds

test-case: set-remotessl-notallowed
    request: set-remotessl-disable.request
    start-state: manageremote-notallowed.ds

test-case: set-remotessl-enable-notallowed
    request: set-remotessl-enable.request
    start-state: manageviassl-enable-notallowed.ds

test-case: set-remotessl-disable-notallowed
    request: set-remotessl-disable.request
    start-state: manageonlyviassl-disable-notallowed.ds

test-case: set-remotessl-enable-notallowed2
    request: set-remotessl-enable.request
    start-state: remotesslneedsssl-enable-notallowed.ds

test-case: set-domainname-notallowed
    request: set-domainname.request
    start-state: domainname-notallowed.ds

test-case: set-wiredqos-notallowed
    request: set-wiredqos-enable.request
    start-state: wiredqos-notallowed.ds

test-case: set-manageremote-enable-with-defaultpsswd
    request: set-manageremote-enable.request
