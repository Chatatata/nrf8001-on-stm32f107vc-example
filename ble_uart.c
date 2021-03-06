/* Copyright (c) 2014, Nordic Semiconductor ASA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


/** @defgroup ble_uart_project_template ble_uart_project_template
@{
@ingroup projects
@brief Empty project that can be used as a template for new projects.

@details
The following instructions describe the steps to be made on the Windows PC:

  -# Install the Master Control Panel on your computer. Connect the Master Emulator
    (nRF2739) and make sure the hardware drivers are installed.

  -# You can use the nRF UART app in the Apple iOS app store and Google Play for Android 4.3 for Samsung Galaxy S4
    with this UART template app

  -# You can send data from a serial monitor, maximum length of a string is 19 bytes
    Set the line ending to "Newline" in the Serial monitor (The newline is also sent over the air)

 *
 * The setup() function is called first and is called only once for each reset of the device.
 * The loop() function as the name implies is called in a loop.
 *
 * The setup() and loop() function are called in this way.
 * main()
 *  {
 *   setup();
 *   while(1)
 *   {
 *     loop();
 *   }
 * }
 *
 */
#include "ble_uart.h"
#include <stddef.h>
#include "blep_hal.h"

 /*
Description:

In this template we are using the BTLE as a UART and can send and receive packets.
The maximum size of a packet is 20 bytes.
When a command it received a response(s) are transmitted back.
Since the response is done using a Notification the peer must have opened it(subscribed to it) before any packet is transmitted.
The pipe for the UART_TX becomes available once the peer opens it.
See section 20.4.1 -> Opening a Transmit pipe
In the master control panel, clicking Enable Services will open all the pipes on the nRF8001.

The ACI Evt Data Credit provides the radio level ack of a transmitted packet.
*/


#ifdef SERVICES_PIPE_TYPE_MAPPING_CONTENT
    static services_pipe_type_mapping_t
        services_pipe_type_mapping[NUMBER_OF_PIPES] = SERVICES_PIPE_TYPE_MAPPING_CONTENT;
#else
    #define NUMBER_OF_PIPES 0
    static services_pipe_type_mapping_t * services_pipe_type_mapping = NULL;
#endif


static const hal_aci_data_t setup_msgs[NB_SETUP_MESSAGES] = SETUP_MESSAGES_CONTENT;


void ble_uart_setup(uint8_t pin_mosi, uint8_t pin_miso, uint8_t pin_sck, uint8_t pin_reqn, uint8_t pin_rdyn, uint8_t pin_reset)
{
        UART1_Write_Text("BLE P Click Setup");
        UART1_Write_Text("Set line ending to newline to send data from the serial monitor");

  /**
  Point ACI data structures to the the setup data that the nRFgo studio generated for the nRF8001
  */
  if (NULL != services_pipe_type_mapping)
  {
    aci_state.aci_setup_info.services_pipe_type_mapping = &services_pipe_type_mapping[0];
  }
  else
  {
    aci_state.aci_setup_info.services_pipe_type_mapping = NULL;
  }
  aci_state.aci_setup_info.number_of_pipes    = NUMBER_OF_PIPES;
  aci_state.aci_setup_info.setup_msgs         = (hal_aci_data_t*) setup_msgs;
  aci_state.aci_setup_info.num_setup_msgs     = NB_SETUP_MESSAGES;

  /*
  Tell the ACI library, the MCU to nRF8001 pin connections.
  The Active pin is optional and can be marked NRF_UNUSED
  */
  aci_state.aci_pins.board_name = BOARD_DEFAULT; //See board.h for details
  aci_state.aci_pins.mosi_pin   = pin_mosi;
  aci_state.aci_pins.miso_pin   = pin_miso;
  aci_state.aci_pins.reqn_pin   = pin_reqn;
  aci_state.aci_pins.rdyn_pin   = pin_rdyn;
  aci_state.aci_pins.sck_pin    = pin_sck;
  aci_state.aci_pins.reset_pin  = pin_reset;

//  aci_state.aci_pins.spi_clock_divider      = SPI_CLOCK_DIV8;//SPI_CLOCK_DIV8  = 2MHz SPI speed
                                                             //SPI_CLOCK_DIV16 = 1MHz SPI speed

  aci_state.aci_pins.active_pin             = ACTIVE_PIN;
//  aci_state.aci_pins.optional_chip_sel_pin  = SS;

  aci_state.aci_pins.interface_is_interrupt = false;
  aci_state.aci_pins.interrupt_number       = 1;

  //We reset the nRF8001 here by toggling the RESET line connected to the nRF8001
  //If the RESET line is not available we call the ACI Radio Reset to soft reset the nRF8001
  //then we initialize the data structures required to setup the nRF8001
  //The second parameter is for turning debug printing on for the ACI Commands and Events so they be printed on the Serial
  lib_aci_init(&aci_state, false);

        UART1_Write_Text("SETUP DONE");
}

void ble_uart_name_set(const char* device_name, uint8_t name_length)
{
  uint8_t i;
        if (lib_aci_set_local_data(&aci_state, PIPE_GAP_DEVICE_NAME_SET, (uint8_t*) device_name, min(name_length, 16)))
        {
                UART1_Write_Text("Name set. New name: ");
                for ( i = 0; i < min(name_length, 16); ++i)
                {
                        UART1_Write(device_name[i]);
                }
//                Serial.println();
        }
        else
        {
                UART1_Write_Text("Name setting failed.");
        }
}



bool ble_uart_tx(uint8_t *buffer, uint8_t buffer_len)
{
  bool status = false;

  if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_TX_TX) &&
      (aci_state.data_credit_available >= 1))
  {
    status = lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, buffer, buffer_len);
    if (status)
    {
      aci_state.data_credit_available--;
    }
  }

  return status;
}

void ble_uart_loop() {

  //Process any ACI commands or events
  aci_loop();
}

void ble_uart_rx(uint8_t *buffer, uint8_t len)
{
        uint8_t i;
        UART1_Write_Text("Received data: ");

        for (i = 0; i < len; ++i)
        {
                UART1_Write((char) buffer[i]);
        }
}