
add_definitions( -DSQLITE_HAS_CODEC)

set(SQLITE_SRC
    sqlite3.c
)

add_library(sqlite STATIC ${SQLITE_SRC})
target_link_libraries(sqlite ${OPENSSL_LIBCRYPTO})

if(UNIX AND NOT ANDROID)
    target_link_libraries(sqlite dl pthread)
endif()

if(ANDROID_ABI STREQUAL "armeabi-v7a")
    target_link_libraries(sqlite $ENV{OPENSSL_ROOT_DIR_ANDROID}/armv7-shared/lib/libssl.a)
    target_link_libraries(sqlite $ENV{OPENSSL_ROOT_DIR_ANDROID}/armv7-shared/lib/libcrypto.a)
elseif(ANDROID_ABI STREQUAL "x86")
    target_link_libraries(sqlite $ENV{OPENSSL_ROOT_DIR_ANDROID}/x86-shared/lib/libssl.a)
    target_link_libraries(sqlite $ENV{OPENSSL_ROOT_DIR_ANDROID}/x86-shared/lib/libcrypto.a)
endif()
