hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/d74ae25cbc0b4c84fcf97664e4ea8f751a530df4.tar.gz
    SHA1 d8476b5bceb8fb169e58a389e4d0688a01e70821
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)

hunter_config(bcos-crypto
	VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-crypto/archive/25c8edb7d5cbadb514bbce9733573c8ffdb3600d.tar.gz
	SHA1 4a1649e7095f5db58a5ae0671b2278bcccc25f1d
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/502c17b20558f4fd3809135622ca68c71b8ff16d.tar.gz
    SHA1 7f94aa82374b3dbbeaa10af056fcea14bd72720a
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON URL_BASE=${URL_BASE}
)

hunter_config(bcos-boostssl
	VERSION 3.0.0-local
	URL "https://${URL_BASE}/FISCO-BCOS/bcos-boostssl/archive/24b005d5eae39d332f9ca2a4eae12ba3e4a69ed2.tar.gz"
	SHA1 ca925ddbe75d1773a79d2f2744863a936c97f2b8
)