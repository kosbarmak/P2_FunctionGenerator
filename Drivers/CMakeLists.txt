# Vendor headers used by the firmware.
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    STM32L4xx_HAL_Driver/Inc
    STM32L4xx_HAL_Driver/Inc/Legacy
    BSP/STM32L4xx_Nucleo
    CMSIS/Device/ST/STM32L4xx/Include
    CMSIS/Include
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    BSP/STM32L4xx_Nucleo/stm32l4xx_nucleo.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_dma.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_dma_ex.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_i2c.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_i2c_ex.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rcc.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rcc_ex.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash_ex.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash_ramfunc.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_gpio.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr_ex.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_cortex.c
    STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_exti.c
)
