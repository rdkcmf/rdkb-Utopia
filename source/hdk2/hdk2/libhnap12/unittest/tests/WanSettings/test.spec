# tests/WanSettings
test-case: get0
    request: get.request

test-case: set-dhcp
    end-state: dhcp.ds

test-case: get-dhcp
    request: get.request
    start-state: actual/dhcp.ds

test-case: set-static
    end-state: static.ds

test-case: get-static
    request: get.request
    start-state: actual/static.ds

test-case: set-dhcppppoe
    end-state: dhcppppoe.ds

test-case: get-dhcppppoe
    request: get.request
    start-state: actual/dhcppppoe.ds

test-case: set-dhcppppoe-mit-0
    end-state: dhcppppoe-mit-0.ds

test-case: get-dhcppppoe-mit-0
    request: get.request
    start-state: actual/dhcppppoe-mit-0.ds

test-case: set-dhcppppoe-bad-mit

test-case: set-staticpppoe
    end-state: staticpppoe.ds
    diff-state: true

test-case: get-staticpppoe
    request: get.request
    start-state: actual/staticpppoe.ds

test-case: set-bigpond
    end-state: bigpond.ds

test-case: get-bigpond
    request: get.request
    start-state: actual/bigpond.ds

test-case: set-dynamicl2tp
    end-state: dynamicl2tp.ds

test-case: get-dynamicl2tp
    request: get.request
    start-state: actual/dynamicl2tp.ds

test-case: set-staticl2tp
    end-state: staticl2tp.ds

test-case: get-staticl2tp
    request: get.request
    start-state: actual/staticl2tp.ds

test-case: set-dynamicpptp
    end-state: dynamicpptp.ds

test-case: get-dynamicpptp
    request: get.request
    start-state: actual/dynamicpptp.ds

test-case: set-staticpptp
    end-state: staticpptp.ds

test-case: get-staticpptp
    request: get.request
    start-state: actual/staticpptp.ds

test-case: set-bridgedonly
    end-state: bridgedonly.ds

test-case: get-bridgedonly
    request: get.request
    start-state: actual/bridgedonly.ds

test-case: set-dynamic1483bridged
    end-state: dynamic1483bridged.ds

test-case: get-dynamic1483bridged
    request: get.request
    start-state: actual/dynamic1483bridged.ds

test-case: set-static1483bridged
    end-state: static1483bridged.ds

test-case: get-static1483bridged
    request: get.request
    start-state: actual/static1483bridged.ds

test-case: set-static1483routed
    end-state: static1483routed.ds

test-case: get-static1483routed
    request: get.request
    start-state: actual/static1483routed.ds

test-case: set-dynamicpppoa
    end-state: dynamicpppoa.ds

test-case: get-dynamicpppoa
    request: get.request
    start-state: actual/dynamicpppoa.ds

test-case: set-staticpppoa
    end-state: staticpppoa.ds

test-case: get-staticpppoa
    request: get.request
    start-state: actual/staticpppoa.ds

test-case: set-staticipoa
    end-state: staticipoa.ds

test-case: get-staticipoa
    request: get.request
    start-state: actual/staticipoa.ds

test-case: set-type-not-supported
    request: set-dhcp.request
    start-state: dhcp-not-supported.ds

test-case: set-automtu
    end-state: set-automtu.ds

test-case: get-automtu
    request: get.request
    start-state: actual/set-automtu.ds

test-case: set-automtu-not-allowed
    request: set-automtu.request
    start-state: automtu-not-allowed.ds

test-case: set-bad-mtu

test-case: set-static-bad-mtu

test-case: set-dhcp-bad-mtu

test-case: set-staticpppoe-bad-mtu

test-case: set-dhcppppoe-bad-mtu

test-case: set-bigpond-bad-mtu

test-case: set-staticl2tp-bad-mtu

test-case: set-dynamicl2tp-bad-mtu

test-case: set-staticpptp-bad-mtu

test-case: set-dynamicpptp-bad-mtu

test-case: set-bridgedonly-bad-mtu

test-case: set-dynamic1483bridged-bad-mtu

test-case: set-static1483bridged-bad-mtu

test-case: set-static1483routed-bad-mtu

test-case: set-staticpppoa-bad-mtu

test-case: set-dynamicpppoa-bad-mtu

test-case: set-staticipoa-bad-mtu

test-case: set-static-bad-no-dns

test-case: set-bigpond-bad-autoreconnect

test-case: set-dhcppppoe-bad-autoreconnect

test-case: set-dynamicpppoa-bad-autoreconnect

test-case: set-staticpppoe-bad-autoreconnect

test-case: set-staticpppoa-bad-autoreconnect

test-case: set-empty-password

test-case: set-empty-username

test-case: set-ip-bad

test-case: set-ip-bad-broadcast

test-case: set-gateway-bad-broadcast

test-case: set-static-bad-gateway

test-case: set-bad-subnet1

test-case: set-bad-subnet2

test-case: set-bad-subnet3

test-case: set-bridgedonly-dns
    end-state: bridgedonly-dns.ds

test-case: get-bridgedonly-dns
    request: get.request
    start-state: actual/bridgedonly-dns.ds

test-case: set-bridgedonly-dns-bad

test-case: set-static-dns-bad