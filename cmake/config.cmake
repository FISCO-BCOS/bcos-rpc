hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/bba5fd46533a3ec33cf608ad0bbef0d2bb822cc1.tar.gz
    SHA1 0d11165cb034cb1b6b7678c92266acea5c21f59b
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)

hunter_config(bcos-crypto
	VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-crypto/archive/25c8edb7d5cbadb514bbce9733573c8ffdb3600d.tar.gz
	SHA1 4a1649e7095f5db58a5ae0671b2278bcccc25f1d
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/41bdfe5675ccf831883deb9017a3bf11eff1792a.tar.gz
    SHA1 a1ecc48f032bd31ee3373eb85e0e2e092797bad8
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON URL_BASE=${URL_BASE}
)

hunter_config(bcos-boostssl
	VERSION 3.0.0-local
	URL "https://${URL_BASE}/FISCO-BCOS/bcos-boostssl/archive/24b005d5eae39d332f9ca2a4eae12ba3e4a69ed2.tar.gz"
	SHA1 ca925ddbe75d1773a79d2f2744863a936c97f2b8
)