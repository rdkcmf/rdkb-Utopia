test-case: SetGuestNetwork
    diff-state: true

test-case: GetGuestNetwork
    start-state: actual/SetGuestNetwork.ds

test-case: set-ssid-good

test-case: set-ssid-good-2

test-case: set-ssid-bad

test-case: set-ssid-bad-2

test-case: set-ssid-bad-3

test-case: set-password-good

test-case: set-password-good-2

test-case: set-password-bad

test-case: set-password-bad-2

test-case: set-password-bad-3

test-case: set-maxguests-good

test-case: set-maxguests-bad

test-case: set-maxguests-bad-2
