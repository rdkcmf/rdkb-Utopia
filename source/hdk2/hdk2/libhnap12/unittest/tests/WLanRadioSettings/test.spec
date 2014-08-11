# tests/WLanRadioSettings

test-case: get0
    request: get.request

test-case: get-bad-radioid
    request: get-bad-radioid.request

test-case: set-bad-radioid
    request: set-bad-radioid.request

test-case: set-disable
    request: set-disable.request
    end-state: disable.ds

test-case: get-disable
    request: get.request
    start-state: actual/disable.ds

test-case: set-enable
    request: set-enable.request
    end-state: enable.ds

test-case: get-enable
    request: get.request
    start-state: actual/enable.ds

test-case: set-all-radio24
    diff-state: true
    end-state: all-radio24.ds

test-case: get-all-radio24
    request: get.request
    start-state: actual/all-radio24.ds

test-case: set-all-radio5
    diff-state: true
    end-state: all-radio5.ds

test-case: get-all-radio5
    request: get-radio5.request
    start-state: actual/all-radio5.ds

test-case: set-channelwidth-bad
   request: set-channelwidth-bad.request

test-case: set-channel-0
   request: set-channel-0.request
   end-state: channel-0.ds

test-case: get-channel-0
    request: get.request
    start-state: actual/channel-0.ds

test-case: set-secchannel-0
   request: set-secchannel-0.request
   end-state: secchannel-0.ds

test-case: get-secchannel-0
    request: get.request
    start-state: actual/secchannel-0.ds

test-case: set-channel-bad
   request: set-channel-bad.request

test-case: set-channel-bad2
   request: set-channel-bad2.request

test-case: set-channel-bad3
   request: set-channel-bad3.request

test-case: set-secondarychannel-bad
   request: set-secondarychannel-bad.request

test-case: set-secondarychannel-bad2
   request: set-secondarychannel-bad2.request

test-case: set-secondarychannel-bad3
   request: set-secondarychannel-bad3.request

test-case: set-mode-bad
   request: set-mode-bad.request

test-case: set-mode-bad-width1
   request: set-mode-bad-width1.request

test-case: set-mode-bad-width2
   request: set-mode-bad-width2.request

test-case: set-mode-bad-width3
   request: set-mode-bad-width3.request

test-case: set-mode-bad-width4
   request: set-mode-bad-width4.request

test-case: set-mode-bad-security
    request: set-mode-bad-security.request
    start-state: mode-bad-security.ds

test-case: set-mode-bad-encryption
    request: set-mode-bad-security.request
    start-state: mode-bad-encryption.ds

test-case: set-mode-bad-encryption2
    request: set-mode-bad-security.request
    start-state: mode-bad-encryption2.ds