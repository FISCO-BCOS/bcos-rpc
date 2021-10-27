hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/cyjseagull/bcos-framework/archive/bbf6bae4058762177464ba94100e703b38452e2d.tar.gz
    SHA1 d8295b9d8b822c8c5a73b30797150f910c06af55
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)

hunter_config(bcos-crypto
	VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-crypto/archive/25c8edb7d5cbadb514bbce9733573c8ffdb3600d.tar.gz
	SHA1 4a1649e7095f5db58a5ae0671b2278bcccc25f1d
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/cyjseagull/bcos-tars-protocol/archive/f5f1ecfb128da727cc3874abba0e6364780cc394.tar.gz
    SHA1 670c09c512c0af6344a982c03b4c74eb90da8094
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON URL_BASE=${URL_BASE}
)

hunter_config(bcos-boostssl
	VERSION 3.0.0-local
	URL "https://${URL_BASE}/FISCO-BCOS/bcos-boostssl/archive/1b972a6734ef49ac4ca56184d31fe54a08a97e82.tar.gz"
	SHA1 6d35d940eacb7f41db779fb8182cbebf50535574
)