/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Abhijith Soman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "spdif_rx.h"
#include "usb_spdif.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

// variables
uint32_t sample_buffer[SAMPLE_BUFFER_SIZE];

// callback functions
void on_usb_spdif_tx_ready();

/*------------- MAIN -------------*/
int main(void)
{
  board_init();

  spdif_rx_config_t config = {
      .data_pin = PIN_PICO_SPDIF_RX_DATA,
      .pio_sm = 0,
      .dma_channel0 = 0,
      .dma_channel1 = 1,
      .alarm = 0,
      .flags = SPDIF_RX_FLAGS_ALL
  };
  // initialize and start the SPDIF input
  // spdif_init(&config);
  // spdif_set_samples_ready_handler(on_pdm_samples_ready);
  spdif_start(&config);

  // initialize the USB microphone interface
  usb_spdif_init();
  usb_spdif_set_tx_ready_handler(on_usb_spdif_tx_ready);

  while (1) {
    // run the USB spdif task continuously
    usb_spdif_task();
  }

  return 0;
}

void on_usb_spdif_tx_ready()
{
  // // Callback from TinyUSB library when all data is ready
  // // to be transmitted.
  // //
  // // Write local buffer to the USB spdif
  // usb_spdif_write(fifo_buff, sizeof(SAMPLE_BUFFER_SIZE));
  // // tud_audio_write ((uint8_t *)test_buffer_audio, CFG_TUD_AUDIO_EP_SZ_IN - 2);
  decode();
}



void decode()
{
  static bool mute_flag = true;

  uint32_t sample_count = SAMPLE_BUFFER_SIZE
  // ab->sample_count = ab->max_sample_count;
  // int32_t *samples = (int32_t *) ab->buffer->bytes;

  uint32_t fifo_count = spdif_rx_get_fifo_count();
  if (spdif_rx_get_state() == SPDIF_RX_STATE_STABLE) {
      if (mute_flag && fifo_count >= SPDIF_RX_FIFO_SIZE / 2) {
          mute_flag = false;
      }
  } else {
      mute_flag = true;
  }

  if (mute_flag) {
      for (int i = 0; i < sample_count; i++) {
          sample_buffer[i*2+0] = DAC_ZERO;
          sample_buffer[i*2+1] = DAC_ZERO;
      }
  } else {
      //printf("%d,", fifo_count);
      if (sample_count > fifo_count / 2) {
          sample_count = fifo_count / 2;
      }
      uint32_t total_count = sample_count * 2;
      int i = 0;
      uint32_t read_count = 0;
      uint32_t* buff;
      while (read_count < total_count) {
          uint32_t get_count = spdif_rx_read_fifo(&buff, total_count - read_count);
          for (int j = 0; j < get_count / 2; j++) {
              sample_buffer[i*2+0] = (int32_t) ((buff[j*2+0] & 0x0ffffff0) << 4) + DAC_ZERO;
              sample_buffer[i*2+1] = (int32_t) ((buff[j*2+1] & 0x0ffffff0) << 4) + DAC_ZERO;
              i++;
          }
          read_count += get_count;
      }
  }
  usb_spdif_write((uint8_t *)sample_buffer, get_count * 4);

}