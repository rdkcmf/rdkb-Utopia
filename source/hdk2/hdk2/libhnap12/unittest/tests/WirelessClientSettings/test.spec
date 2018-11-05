# tests/WirelessClientSettings

test-case: get0
    request: get.request

test-case: set-disable
    request: set-disable.request
    end-state: disable.ds

test-case: get-disable
    request: get.request
    start-state: actual/disable.ds

test-case: set-networktype
    request: set-networktype.request
    end-state: networktype.ds

test-case: set-networktype-bad
    request: set-networktype-bad.request

test-case: get-networktype
    request: get.request
    start-state: actual/networktype.ds

test-case: set-securitytype
    request: set-securitytype.request
    end-state: securitytype.ds

test-case: get-securitytype
    request: get.request
    start-state: actual/securitytype.ds

test-case: set-securitytype-bad
    request: set-securitytype-bad.request

test-case: set-securitytype-bad2
    request: set-securitytype-bad2.request

test-case: set-encryption
    request: set-encryption.request
    end-state: encryption.ds

test-case: get-encryption
    request: get.request
    start-state: actual/encryption.ds

test-case: set-encryption-bad
    request: set-encryption-bad.request

test-case: set-encryption-bad2
    request: set-encryption-bad2.request

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
