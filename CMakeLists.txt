set(PICO_DEFAULT_BINARY_TYPE copy_to_ram)

add_executable(n64_eeprom
        eeprom.c
        )

# pull in common dependencies
target_link_libraries(n64_eeprom pico_stdlib hardware_flash hardware_pwm)

# create map/bin/hex file etc.
pico_add_extra_outputs(n64_eeprom)

#pico_enable_stdio_usb(n64_eeprom 1)
#pico_enable_stdio_uart(GC_hid 0)

# add url via pico_set_program_url
example_auto_set_url(n64_eeprom)

pico_set_binary_type(n64_eeprom copy_to_ram)
