/*
 Mega serial thru test code

 Receives from serial port 1 (the keyscan controller), 
 and sends to the main serial port. When the mega is modified
 with HIDuino this allows us to convert MIDI messages from the
 keyscan controller into USB MIDI messages.

 The circuit: 
 * Any serial device attached to Serial port 1

 */

void setup() 
{
  //main serial port (MIDI output)
  Serial.begin(31250);

  //keyscan controller serial port
  Serial1.begin(38400);
}

void loop() 
{
  // read from port 1, send to port 0:
  if (Serial1.available()) 
  {
    int inByte = Serial1.read();
    Serial.write(inByte); 
  }
}
