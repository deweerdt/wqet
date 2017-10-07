cmake_minimum_required(VERSION 2.8.11)
cmake_policy(SET CMP0003 NEW)

project(wqet)

find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND AND (OPENSSL_VERSION VERSION_LESS "1.0.2"))
    message(FATAL "OpenSSL 1.0.2 or above is missing")
endif ()

FIND_PROGRAM(CLANG_FORMAT_BIN clang-format)
IF (CLANG_FORMAT_BIN)
    ADD_CUSTOM_TARGET(fmt ${CMAKE_SOURCE_DIR}/misc/clang_format_all.sh ${CLANG_FORMAT_BIN})
ENDIF()

set(CMAKE_C_FLAGS "-std=c99 -Wall -O2 -g ${CC_WARNING_FLAGS} ${CMAKE_C_FLAGS}")
set(PICOTLS_OPENSSL_FILES
	deps/picotls/lib/openssl.c
	deps/picotls/lib/pembase64.c
	deps/picotls/lib/picotls.c)

include_directories(${OPENSSL_INCLUDE_DIR} deps/picotls/include include)
add_executable(wqet ${PICOTLS_OPENSSL_FILES} src/wqet.c)
target_link_libraries(wqet ${OPENSSL_LIBRARIES} ${CMAKE_DL_LIBS})
