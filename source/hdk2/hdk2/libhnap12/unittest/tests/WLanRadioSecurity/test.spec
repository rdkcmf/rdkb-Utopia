# tests/WLanRadioSecurity

test-case: get0
    request: get.request

test-case: get-bad-radioid
    request: get-bad-radioid.request

test-case: set-bad-radioid
    request: set-bad-radioid.request

test-case: set-disable
    request: set-disable.request
    diff-state: true
    end-state: disable.ds

test-case: get-disable
    request: get.request
    start-state: actual/disable.ds

test-case: set-disable2
    request: set-disable2.request
    end-state: disable2.ds

test-case: get-disable2
    request: get.request
    start-state: actual/disable2.ds

test-case: set-enable
    request: set-enable.request
    end-state: enable.ds

test-case: get-enable
    request: get.request
    start-state: actual/enable.ds

test-case: set-enable-bad
    request: set-enable-bad.request

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

test-case: set-all-radio24-nostate
    diff-state: true
    start-state: all-radio24-nostate.ds
    end-state: all-radio24-nostate.ds

test-case: get-all-radio24-nostate
    request: get.request
    start-state: actual/all-radio24-nostate.ds

test-case: set-type
    request: set-type.request
    end-state: type.ds

test-case: get-type
    request: get.request
    start-state: actual/type.ds

test-case: set-type-bad

test-case: set-type-bad-mode
    start-state: type-bad-mode.ds

test-case: set-type-good-mode
    start-state: type-bad-mode.ds

test-case: set-no-type-bad

test-case: set-encryption
    request: set-encryption.request
    end-state: encryption.ds

test-case: get-encryption
    request: get.request
    start-state: actual/encryption.ds

test-case: set-encryption-bad

test-case: set-encryption-bad-mode
    start-state: type-bad-mode.ds

test-case: set-encryption-good-mode
    start-state: type-bad-mode.ds

test-case: set-no-encryption-bad

test-case: set-key-wep64
    request: set-key-wep64.request
    end-state: key-wep64.ds

test-case: get-key-wep64
    request: get.request
    start-state: actual/key-wep64.ds

test-case: set-key-wep128
    request: set-key-wep128.request
    end-state: key-wep128.ds

test-case: get-key-wep128
    request: get.request
    start-state: actual/key-wep128.ds

test-case: set-key-wpa
    request: set-key-wpa.request
    end-state: key-wpa.ds

test-case: get-key-wpa
    request: get.request
    start-state: actual/key-wpa.ds

test-case: set-key-wpahex
    request: set-key-wpahex.request
    end-state: key-wpahex.ds

test-case: get-key-wpahex
    request: get.request
    start-state: actual/key-wpahex.ds

test-case: set-key-wep-bad-hex
    request: set-key-wep-bad-hex.request

test-case: set-key-wep-bad-length
    request: set-key-wep-bad-length.request

test-case: set-key-wpa-bad-hex
    request: set-key-wpa-bad-hex.request

test-case: set-key-wpa-bad-length
    request: set-key-wpa-bad-length.request

test-case: set-key-renewal-wep
    request: set-key-renewal-wep.request
    end-state: key-renewal-wep.ds

test-case: get-key-renewal-wep
    request: get.request
    start-state: actual/key-renewal-wep.ds

test-case: set-radius1
    request: set-radius1.request
    end-state: radius1.ds

test-case: get-radius1
    request: get.request
    start-state: actual/radius1.ds

test-case: set-radius2
    request: set-radius2.request
    end-state: radius2.ds

test-case: get-radius2
    request: get.request
    start-state: actual/radius2.ds

test-case: set-radius2-not-supported
    request: set-radius2.request
    start-state: radius2-not-allowed.ds

test-case: set-radius-disable
    request: set-radius-disable.request
    end-state: radius-disable.ds

test-case: get-radius-disable
    request: get.request
    start-state: actual/radius-disable.ds

test-case: set-key-wpa-radius
    request: set-key-wpa-radius.request
    end-state: key-wpa-radius.st

test-case: get-key-wpa-radius
    request: get.request
    start-state: actual/key-wpa-radius.st

test-case: set-key-wpa2-radius
    request: set-key-wpa2-radius.request
    end-state: key-wpa2-radius.st

test-case: get-key-wpa2-radius
    request: get.request
    start-state: actual/key-wpa2-radius.st