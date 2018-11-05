# tests/RouterLanSettings2

test-case: get0
    request: get.request

test-case: set-routerip
    end-state: routerip.ds

test-case: get-routerip
    request: get.request
    start-state: actual/routerip.ds

test-case: set-routerip-bad

test-case: set-routerip-broadcast-bad1

test-case: set-routerip-broadcast-bad2

test-case: set-routersubnet
    end-state: routersubnet.ds

test-case: get-routersubnet
    request: get.request
    start-state: actual/routersubnet.ds

test-case: set-routersubnet-bad

test-case: set-dhcp-disable
    end-state: dhcp-disable.ds

test-case: get-dhcp-disable
    request: get.request
    start-state: actual/dhcp-disable.ds

test-case: set-dhcp-enable
    end-state: dhcp-enable.ds

test-case: get-dhcp-enable
    request: get.request
    start-state: actual/dhcp-enable.ds

test-case: set-dhcp-range
    end-state: dhcp-range.ds

test-case: get-dhcp-range
    request: get.request
    start-state: actual/dhcp-range.ds

test-case: set-dhcp-range2
    end-state: dhcp-range2.ds

test-case: get-dhcp-range2
    request: get.request
    start-state: actual/dhcp-range2.ds

test-case: set-dhcp-range3
    end-state: dhcp-range3.ds

test-case: get-dhcp-range3
    request: get.request
    start-state: actual/dhcp-range3.ds

test-case: set-dhcp-range4
    end-state: dhcp-range4.ds

test-case: get-dhcp-range4
    request: get.request
    start-state: actual/dhcp-range4.ds

test-case: set-dhcp-range-bad

test-case: set-dhcp-range-bad2

test-case: set-dhcp-range-bad3

test-case: set-dhcp-range-bad4

test-case: set-dhcp-range-bad5

test-case: set-leasetime
    end-state: leasetime.ds

test-case: get-leasetime
    request: get.request
    start-state: actual/leasetime.ds

test-case: set-leasetime-bad

test-case: set-leasetime-bad2

test-case: set-leasetime-bad3

test-case: set-dhcpres
    end-state: dhcpres.ds

test-case: get-dhcpres
    request: get.request
    start-state: actual/dhcpres.ds

test-case: set-dhcpres2
    end-state: dhcpres2.ds

test-case: get-dhcpres2
    request: get.request
    start-state: actual/dhcpres2.ds

test-case: set-dhcpres3
    end-state: dhcpres3.ds

test-case: get-dhcpres3
    request: get.request
    start-state: actual/dhcpres3.ds

test-case: set-dhcpres-bad

test-case: set-dhcpres-bad2

test-case: set-dhcpres-bad3

test-case: set-dhcpres-bad4

test-case: set-dhcpres-notallowed
    request: set-dhcpres.request
    start-state: dhcpres-notallowed.ds

test-case: set-bad-subnet1

test-case: set-bad-subnet2

test-case: set-bad-subnet3