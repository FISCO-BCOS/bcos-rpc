hunter_config(bcos-crypto
	VERSION 3.0.0-local
	URL "https://${URL_BASE}/FISCO-BCOS/bcos-crypto/archive/25c8edb7d5cbadb514bbce9733573c8ffdb3600d.tar.gz"
	SHA1 4a1649e7095f5db58a5ae0671b2278bcccc25f1d
)

hunter_config(bcos-framework VERSION 3.0.0-003f26b1
URL "https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/3e111b7ab7ce9db1911b4359710e0aa5923a4565.tar.gz"
SHA1 "c4e92cb2002b39edfccc2310d13ebd7e1800ae75"
CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)