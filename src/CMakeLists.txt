aux_source_directory(. FZ_NET_SOURCES)

add_library(fz_net ${FZ_NET_SOURCES})

target_compile_options(fz_net PRIVATE -Wall -Wextra -Wpedantic)

target_include_directories(fz_net PUBLIC $<BUILD_INTERFACE:${FZ_NET_PUBLIC_INCLUDE_DIR}>
                                        $<INSTALL_INTERFACE:include>)

target_include_directories(fz_net PUBLIC $<BUILD_INTERFACE:${FZ_NET_EXTERNAL_INCLUDE_DIR}>
                                        $<INSTALL_INTERFACE:include>)

target_link_libraries(fz_net PUBLIC ${FZ_NET_EXTERNAL_LIBRARIES})

install(TARGETS fz_net
        EXPORT fz_net-targets
        LIBRARY DESTINATION ${FZ_NET_INSTALL_PREFIX}/lib
        ARCHIVE DESTINATION ${FZ_NET_INSTALL_PREFIX}/lib
        RUNTIME DESTINATION ${FZ_NET_INSTALL_PREFIX}/bin
        INCLUDES DESTINATION ${FZ_NET_INSTALL_PREFIX}/include
)

install(
  DIRECTORY ${FZ_NET_PUBLIC_INCLUDE_DIR}/fz
  DESTINATION ${FZ_NET_INSTALL_PREFIX}/include
)
