/*
 * FlashUpdate.ino can be used with uploader.py to update the firmware
 * of Nuvoton ChipCorder ISD2360 devices.
 * Copyright (C) 2020 Mathis Dedial
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <ISD2360.h>

// #define PIN_LED_UPDATE 8
#define PIN_RDY GPIO_NUM_14
#define PIN_SSB GPIO_NUM_48
#define PIN_MOSI GPIO_NUM_11
#define PIN_MISO GPIO_NUM_9
#define PIN_SCK  GPIO_NUM_12

#define PACKET_HELLO 0x01
#define PACKET_HELLO_ACK 0x02
#define PACKET_ACK 0x03
#define PACKET_ERR 0x04
#define PACKET_STOP 0x05
#define PACKET_RDY 0x06

#define VIBRATOR_PIN GPIO_NUM_39

ISD2360 isd(PIN_RDY, PIN_SSB, PIN_MOSI, PIN_MISO, PIN_SCK, true);


byte n_sectors = 0;
byte response = 0;
byte chunk[128] = {0};
size_t bytes_read = 0;

void setup()
{
  delay(5000);
  pinMode(VIBRATOR_PIN, OUTPUT);
  digitalWrite(VIBRATOR_PIN, HIGH);

  Serial.begin(115200);
  delay(1000);

  Serial.println("FlashUpdate started");
  
  isd.begin();
  isd.reset();
  isd.powerUp();
  digitalWrite(VIBRATOR_PIN, LOW); // Vibrator OFF to indicate ready
  Serial.write(PACKET_RDY);

  // handshake
  while (Serial.available() == 0)
    ;
  if (Serial.read() != PACKET_HELLO)
  {
    Serial.write(PACKET_ERR);
    digitalWrite(VIBRATOR_PIN, HIGH); 
    return;
  }

  // erase chip, then acknowledge
  isd.eraseChip();

  Serial.write(PACKET_HELLO_ACK);
  while (Serial.available() == 0)
    ;
  n_sectors = Serial.read();
  Serial.write(PACKET_ACK);

  // write sectors
  for (uint8_t i = 0; i < n_sectors; ++i)
  {

    // read sector index
    while (Serial.available() == 0)
      ;
    response = Serial.read();
    if (response != i)
    {
      Serial.write(PACKET_ERR);
      digitalWrite(VIBRATOR_PIN, HIGH); 
      return;
    }
    Serial.write(PACKET_ACK);

    // read sector chunks
    for (uint8_t j = 0; j < 8; ++j)
    {
      while (Serial.available() == 0)
        ;
      Serial.readBytes(chunk, 128);

      // write to chip
      isd.flashWrite(i * 0x400UL + j * 0x80UL, chunk, 128);

      Serial.write(PACKET_ACK);
    }
  }

  // phase handshake
  Serial.write(PACKET_STOP);
  while (Serial.available() == 0)
    ;
  response = Serial.read();
  if (response != PACKET_ACK)
  {
    Serial.write(PACKET_ERR);
    digitalWrite(VIBRATOR_PIN, HIGH); 
    return;
  }

  // verify
  for (uint8_t i = 0; i < n_sectors; ++i)
  {
    while (Serial.available() == 0)
      ;
    response = Serial.read();
    if (response != i)
    {
      Serial.write(PACKET_ERR);
      digitalWrite(VIBRATOR_PIN, HIGH); 
      return;
    }
    Serial.write(PACKET_ACK);

    for (uint8_t j = 0; j < 8; ++j)
    {
      // load sector chunks from chip
      isd.flashRead(i * 0x400UL + j * 0x80UL, chunk, 128);

      Serial.write(chunk, 128);

      while (Serial.available() == 0)
        ;
      response = Serial.read();
      if (response != PACKET_ACK)
      {
        Serial.write(PACKET_ERR);
        digitalWrite(VIBRATOR_PIN, HIGH); 
        return;
      }
    }
  }

  // phase handshake
  Serial.write(PACKET_STOP);
  while (Serial.available() == 0)
    ;
  response = Serial.read();
  if (response != PACKET_ACK)
  {
    Serial.write(PACKET_ERR);
    digitalWrite(VIBRATOR_PIN, HIGH); 
    return;
  }

  // cleanup
  isd.powerDown();
  digitalWrite(VIBRATOR_PIN, HIGH); // Vibrator ON at end
}

void loop()
{
}
