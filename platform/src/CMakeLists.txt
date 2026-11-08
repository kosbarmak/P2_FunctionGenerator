target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    startup_stm32l476xx.s
    stm32l4xx_hal_msp.c
    stm32l4xx_it.c
    system_stm32l4xx.c
    syscalls.c
    sysmem.c
)
