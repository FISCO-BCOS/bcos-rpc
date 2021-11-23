hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/24a356f4f827dfd4ee852a61c257c12e0beb8bf9.tar.gz
    SHA1 57b8a4609a1ce8040bf31e726a645160bbd00332
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)

hunter_config(bcos-crypto
	VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-crypto/archive/25c8edb7d5cbadb514bbce9733573c8ffdb3600d.tar.gz
	SHA1 4a1649e7095f5db58a5ae0671b2278bcccc25f1d
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/41156225bfe10d4edd0404a43aa1fbe59f1b5262.tar.gz
    SHA1 d42ff094d4bc5f12fa0bc7c778f6bb02d6494676
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON URL_BASE=${URL_BASE}
)

hunter_config(bcos-boostssl
	VERSION 3.0.0-local
	URL "https://${URL_BASE}/FISCO-BCOS/bcos-boostssl/archive/24b005d5eae39d332f9ca2a4eae12ba3e4a69ed2.tar.gz"
	SHA1 ca925ddbe75d1773a79d2f2744863a936c97f2b8
)