/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"

unsigned char save_file[2048UL + 1];                                          // 16Kilobits or 2KB
#define FLASH_TARGET_OFFSET (1048576)                                         // Location in Flash Rom
const uint8_t *flash_loc = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET); // flash_loc[i] for reading

int Get_Page(int data_pin)
{
  int page = 0;
  for (int page_bit = 7; page_bit >= 0; page_bit--)
  {
    while (!gpio_get(data_pin))
    {
    }
    while (gpio_get(data_pin))
    {
    }
    sleep_us(2); // delay needed to make sure bit is aligned to clock
    page += (gpio_get(data_pin) << page_bit);
  }
  return page;
}

int main()
{
 // 0x00C000 for 16 Kilobit EEPROM
  int response = 0x008000; // 4Kilobit EEPROM
  int data_pin = 5;

   int flash_counter = 0;
  int command_in = 0x00;
  int page = 0;
  int data_out = 0;
  int write_to_flash = 0;
  int memory_location = 0;
  int i = 0;
  const uint LED_PIN = PICO_DEFAULT_LED_PIN;
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);
  gpio_init(data_pin);             // DATA
  gpio_set_dir(data_pin, GPIO_IN); // DATA
  gpio_put(data_pin, 1);
  gpio_set_drive_strength(data_pin, GPIO_DRIVE_STRENGTH_2MA);
  gpio_set_pulls(data_pin, 1, 0);
  gpio_put(LED_PIN, 0);
 

  // int bit_number = 8;
  //  load save data from QSPI FLASH
  for (int i = 0; i <= 2048; i++)
  {
    save_file[i] = flash_loc[i]; // Load Flash Memory into Ram
  }

  while (true)
  {
    //--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    for (int i = 7; i >= 0; i--)
    {
      while (!gpio_get(data_pin))
      {
      } // wait for falling edge
      while (gpio_get(data_pin))
      {
        flash_counter = flash_counter + 1;
        if (flash_counter >= 30000000) // quick and dirty delay to write to eeprom if the console isn't trying to access it, probably will break a save if the console sends a command in between
        {
          if (write_to_flash == 1)
          {
            write_to_flash = 0;
            flash_counter = 0;
            gpio_put(LED_PIN, 1);
            flash_counter = 0;
            uint32_t ints = save_and_disable_interrupts();
            flash_range_erase(FLASH_TARGET_OFFSET, 1); // Multiple of 4KB, we only need 1 4KB block to fit 4 Kilobit and 16Kilobit files
            sleep_us(1);
            flash_range_program(FLASH_TARGET_OFFSET, save_file, 2048); // size_t is array size of data, must be a multiple of 256 bytes
            restore_interrupts(ints);
            sleep_ms(100);
            gpio_put(LED_PIN, 0);
          }
          write_to_flash = 0;
          flash_counter = 0;
        }
      }
      // delay needed to align to clock
      sleep_us(2);
      // READ COMMAND
      command_in += (gpio_get(data_pin) << i);
    }
    //--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    switch (command_in)
    {
      // READ EEPROM -- This Command reads the data in the EEPROM and serves the information to the console
    // EEPROM -> N64
    case 0x04:
      flash_counter = 0;
      page = 0;
      memory_location = 0;
      // read 1 byte in for the page 0-63
      page = Get_Page(data_pin);
      // spit out 8 bytes based on page
      memory_location = page * 8;
      gpio_set_dir(data_pin, GPIO_OUT);
      //====== CLOCK SYNC ==============================
      sleep_us(5);
      //====== CLOCK SYNC ==========================
      for (int byte_number = 0; byte_number <= 7; byte_number++) // Reads 8 bytes at a time
      {
        int bit_number = 8; // Which bit are we reading
        while (bit_number)
        // for (int read_bit = 7; read_bit >= 0; read_bit--)
        {
          gpio_xor_mask(1ul << data_pin);
          sleep_us(1);
          gpio_put(data_pin, 0x01 & (save_file[memory_location + byte_number]) >> (bit_number - 1));
          sleep_us(2);
          gpio_put(data_pin, 1);
          sleep_us(1);
          bit_number--; // decreases which bit is being read by the console
        }
      }
      // This is our stop bit, only needed at the end
      gpio_xor_mask(1ul << data_pin);
      sleep_us(3);
      gpio_xor_mask(1ul << data_pin);
      gpio_set_dir(data_pin, GPIO_IN);
      // command_in = 0x00;
      break;

    //--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // WRITE TO EEPROM -- this commands stores data from the console to the EEPROM
    // N64 -> EEPROM
    case 0x05:
      // else if (command_in == 0x05) // write eeprom
      //{
      flash_counter = 0;
      write_to_flash = 1;
      page = 0;
      // read 1 byte in for the page 0-63
      page = Get_Page(data_pin);
      // write 8 bytes of data
      for (int write_byte = 0; write_byte <= 7; write_byte++)
      {
        data_out = 0;
        for (int write_bit = 7; write_bit >= 0; write_bit--)
        {
          while (!gpio_get(data_pin)) // than it will go low
          {
          }                          // wait for falling edge
          while (gpio_get(data_pin)) // than it will go low
          {
          } // wait for falling edge
          sleep_us(2);
          data_out += (gpio_get(data_pin) << write_bit);
        }
        save_file[(page * 8) + write_byte] = data_out;
      }
      //====== CLOCK SYNC ==============================
      sleep_us(5);
      //====== CLOCK SYNC ==========================
      i = 8;
      gpio_set_dir(data_pin, GPIO_OUT);
      // THIS IS THE EEPROM REPLY BYTE. 0x00 if not busy, 0x80 IF BUSY.
      while (i)
      {
        gpio_xor_mask(1ul << data_pin);
        sleep_us(1);
        gpio_put(data_pin, 0x01 & (0x00) >> (i - 1));
        sleep_us(2);
        gpio_put(data_pin, 1);
        sleep_us(1);
        i--;
      }
      // This is our stop bit, only needed at the end
      gpio_xor_mask(1ul << data_pin);
      sleep_us(3);
      gpio_xor_mask(1ul << data_pin);
      gpio_set_dir(data_pin, GPIO_IN);
      // command_in = 0x00;
      break;

    case 0x00:
    case 0xFF:
      //--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
      // POLL EEPROM
      // else if (command_in == 0x00)
      // {
      flash_counter = 0;
      //====== CLOCK SYNC ==============================
      sleep_us(5);
      //====== CLOCK SYNC ==========================
      
      gpio_set_dir(data_pin, GPIO_OUT);
      i = 24;
      while (i)
      {
        gpio_xor_mask(1ul << data_pin);
        sleep_us(1);
        gpio_put(data_pin, 0x01 & (response) >> (i - 1));
        sleep_us(2);
        gpio_put(data_pin, 1);
        sleep_us(1);
        i--;
      }
      // This is our stop bit
      gpio_xor_mask(1ul << data_pin);
      sleep_us(3);
      gpio_xor_mask(1ul << data_pin);
      gpio_set_dir(data_pin, GPIO_IN);
    }
    //--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    command_in = 0x00;
  }
}
