hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/cyjseagull/bcos-framework/archive/a7a15344062fd6040c3fae0a611551dd138b6e0a.tar.gz
	SHA1 aebfd5ffcc75902f88e5bb116a8a0bf4cd6e3fa0
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON HUNTER_KEEP_PACKAGE_SOURCES=ON
)

hunter_config(bcos-crypto
	VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-crypto/archive/25c8edb7d5cbadb514bbce9733573c8ffdb3600d.tar.gz
	SHA1 4a1649e7095f5db58a5ae0671b2278bcccc25f1d
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/4e5e4f37e50ead912dbc3225a0c2b0b5976f9882.tar.gz
    SHA1 978b3751d7a98a9eae9c9188ac1af3bcd21a97fd
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON URL_BASE=${URL_BASE}
)

hunter_config(bcos-boostssl
	VERSION 3.0.0-local
	URL "https://${URL_BASE}/FISCO-BCOS/bcos-boostssl/archive/1b972a6734ef49ac4ca56184d31fe54a08a97e82.tar.gz"
	SHA1 6d35d940eacb7f41db779fb8182cbebf50535574
)

hunter_config(bcos-ledger
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-ledger/archive/bfc54bbc1af88e010827eb95ae96c0b9e3eec2b7.tar.gz
    SHA1 6383cc6430871ed11225355f3ecab8518b59e043
    CMAKE_ARGS URL_BASE=${URL_BASE} HUNTER_KEEP_PACKAGE_SOURCES=ON
)