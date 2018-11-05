test-case: GetGuestNetwork

test-case: GetGuestNetwork-NoState
    request: GetGuestNetwork.request
    start-state: hotspot-nostate.ds

test-case: GetGuestNetwork-WLanState
    request: GetGuestNetwork.request
    start-state: hotspot-wlanstate.ds

test-case: GetGuestNetwork-WLanState-maxssid
    request: GetGuestNetwork.request
    start-state: hotspot-maxssid.ds

test-case: GetGuestNetwork-WLanState-maxssid2
    request: GetGuestNetwork.request
    start-state: hotspot-maxssid2.ds
