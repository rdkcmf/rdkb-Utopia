test-case: GetTMSSSLicense

test-case: GetTMSSSLicense-New
    request: GetTMSSSLicense.request
    start-state: hotspot-new.ds

test-case: GetTMSSSLicense-NoTM
    request: GetTMSSSLicense.request
    start-state: hotspot-notm.ds
