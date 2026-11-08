target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE include)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    USE_NUCLEO_64
    USE_HAL_DRIVER
    STM32L476xx
    $<$<CONFIG:Debug>:DEBUG>
)

target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    LINKER:-Map=${CMAKE_PROJECT_NAME}.map
    LINKER:--gc-sections
    LINKER:--print-memory-usage
)

add_subdirectory(src)
