
byte counter = 60;

void setup() 
{
  Serial.begin(31250);

}

void loop() 
{
  SendMidiMessage (144, counter, 127);
  digitalWrite (13, HIGH);
  delay (100);
  SendMidiMessage (128, counter, 64);
  digitalWrite (13, LOW);
  delay (100);

  counter++;

  if (counter >= 72)
  {
    counter = 60;
  }
}



void SendMidiMessage (int cmd_byte, int data_byte_1, int data_byte_2) 
{
  byte buf[3] = {cmd_byte, data_byte_1, data_byte_2};
  
  Serial.write (buf, 3);
  
//  Serial.print(buf[0]);
//  Serial.print(" ");
//  Serial.print(buf[1]);
//  Serial.print(" ");
//  Serial.println(buf[2]);
}
