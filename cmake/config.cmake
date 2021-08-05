hunter_config(bcos-crypto
	VERSION 3.0.0-local
	URL "https://${URL_BASE}/FISCO-BCOS/bcos-crypto/archive/25c8edb7d5cbadb514bbce9733573c8ffdb3600d.tar.gz"
	SHA1 4a1649e7095f5db58a5ae0671b2278bcccc25f1d
)

hunter_config(bcos-framework VERSION 3.0.0-33486b94
	URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/c3f15ac89241f5d658b4e34482378151756b5dcb.tar.gz
	SHA1 1c8acc259d955cfec0f3102278889a3fe3fda850
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)
