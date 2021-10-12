hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/e2b7884a5da4f6a695a89660e75fe620aac4eca6.tar.gz
	SHA1 7979f68f6996134517df9f8e67ed6924f924eb6e
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

hunter_config(bcos-ledger
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-ledger/archive/bfc54bbc1af88e010827eb95ae96c0b9e3eec2b7.tar.gz
    SHA1 6383cc6430871ed11225355f3ecab8518b59e043
    CMAKE_ARGS URL_BASE=${URL_BASE} HUNTER_KEEP_PACKAGE_SOURCES=ON
)