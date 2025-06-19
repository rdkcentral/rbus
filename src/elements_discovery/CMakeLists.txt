include_directories(.
                    ../../include
                    ../core
                 ../rtmessage)

 add_executable(disc_elements disc_elements.c)
 add_dependencies(disc_elements rbus rbuscore)
 target_link_libraries(disc_elements rbus rbuscore)

 install (TARGETS disc_elements
         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})