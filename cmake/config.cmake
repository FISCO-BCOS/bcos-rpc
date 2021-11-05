hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/9022efe15e0e6536773338471ab5b3843f002e87.tar.gz
    SHA1 bcf1d2726dbe16113310a9d3892611b06cb7d38c
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)

hunter_config(bcos-crypto
	VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-crypto/archive/25c8edb7d5cbadb514bbce9733573c8ffdb3600d.tar.gz
	SHA1 4a1649e7095f5db58a5ae0671b2278bcccc25f1d
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/325b1c6971e3f42f527996cdf990b722a5eb4d50.tar.gz
    SHA1 06b904725472b1063c6cfc34894cd1979a22ad8a
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON URL_BASE=${URL_BASE}
)

hunter_config(bcos-boostssl
	VERSION 3.0.0-local
	URL "https://${URL_BASE}/FISCO-BCOS/bcos-boostssl/archive/24b005d5eae39d332f9ca2a4eae12ba3e4a69ed2.tar.gz"
	SHA1 ca925ddbe75d1773a79d2f2744863a936c97f2b8
)