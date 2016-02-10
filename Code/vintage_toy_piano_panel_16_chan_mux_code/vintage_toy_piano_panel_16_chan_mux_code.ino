/*
   Vintage Toy Synthesiser Project - panel code.

   This the code for the Arduino Pro Mini attached to the piano's panel.
   This particular code is for using up to 4 16-channel multiplexers.

   All pins are used for the following:
   2 - 5: Mux1 select output pins
   6 - 9: Mux2 select output pins
   10 - 13: Mux3 select output pins
   A4 - A7 (as digital outputs): Mux4 select output pins
   A0: Mux1 input pin
   A1: Mux2 input pin
   A3: Mux3 input pin
   A4: Mux4 input pin

   Note that Mux4 may not be connected, but this code allows for it to be
   used. Mux4 mist be connected if NUM_OF_CONTROLS is greater than 16 * 3.

   //REMEMBER THAT ANY SERIAL DEBUGGING HERE MAY SCREW UP THE SERIAL COMMS TO THE BBB!
*/

//==========================================

//The number of pots/switches attached
const byte NUM_OF_CONTROLS = 39;

//The previous anologue value received from each control
int prevAnalogueValue[NUM_OF_CONTROLS] = {0};
//The previous param/MIDI value sent by each control
byte prevParamValue[NUM_OF_CONTROLS] = {0};

//MIDI channel we want to use
const byte midiChan = 0;

//==========================================
//param data for each control

struct ControlParamData 
{
  const byte cc_num;
  const byte cc_min_val;
  const byte cc_max_val;
};

ControlParamData controlParamData[NUM_OF_CONTROLS] = 
{
  //TODO: for each control/input, lists the param data
  
   {.cc_num = 24, .cc_min_val = 0, .cc_max_val = 127}, //1 - example param
};


void setup()
{
  //Setup serial comms for sending MIDI messages to BBB. 
  //We don't need to use the MIDI baud rate (31250) here, as we're sending the messages to a general
  //serial output rather than a MIDI-specific output.
  Serial.begin(38400);

  //set all needed digital output pins
  for (byte i = 2; i <= 13; i++)
  {
    pinMode (i, OUTPUT);
  }

  pinMode (A4, OUTPUT);
  pinMode (A5, OUTPUT);
  pinMode (A6, OUTPUT);
  pinMode (A7, OUTPUT);

}

void loop()
{
  byte input_to_read;
  byte mux_input_pin;
  byte first_select_pin;
  
  //for each control
  for (byte control_num; control_num < NUM_OF_CONTROLS; control_num++)
  {
    //==========================================
    //==========================================
    //==========================================
    //Read analogue control input...

    //Select the mux/analogue pin we want to read from based on the control number
    //FIXME: there are probably equations I can use here instead.
    if (control_num < 16)
    {
      input_to_read = A0;
      mux_input_pin = control_num;
      first_select_pin = 2;
    }
    else if (control_num < 32)
    {
      input_to_read = A1;
      mux_input_pin = control_num - 16;
      first_select_pin = 6;
    }
    else if (control_num < 48)
    {
      input_to_read = A3;
      mux_input_pin = control_num - 32;
      first_select_pin = 10;
    }
    else
    {
      input_to_read = A3;
      mux_input_pin = control_num - 48;
      first_select_pin = A4;
    }

      //select the input pin on the mux we want to read from, by splitting
      //the mux input pin into bits and sending the bit values to mux select pins.   
      int b0 = bitRead (mux_input_pin, 0);   
      int b1 = bitRead (mux_input_pin, 1);     
      int b2 = bitRead (mux_input_pin, 2); 
      int b3 = bitRead (mux_input_pin, 2); 
      digitalWrite (first_select_pin, b0);
      digitalWrite (first_select_pin + 1, b1);
      digitalWrite (first_select_pin + 2, b2);
      digitalWrite (first_select_pin + 3, b2);

      //read the input value
      int read_val = analogRead (input_to_read);

      //==========================================
      //==========================================
      //==========================================
      //Process analogue control input...

      //if the read control value is different from it's last reading
      if (prevAnalogueValue[control_num] != read_val)
      {
        //store the value
        prevAnalogueValue[control_num] = read_val;

        //convert the control value into a param/MIDI CC value 
        byte param_val = ConvertControlValToParamVal (control_num); 

        //if the param val is different from the last param val
        if (prevParamValue[control_num] != param_val)
        {
          //store the value
          prevParamValue[control_num] = param_val;

          //Send the param value as a MIDI CC message
          SendMidiMessage (0xB0 + midiChan, controlParamData[control_num].cc_num, prevParamValue[control_num]);
        
        } //if (prevParamValue[control_num] != param_val)
        
      } //if (prevAnalogueValue[control_num] != read_val)

  } //for (byte control_num; control_num < NUM_OF_CONTROLS; control_num++)

  //==========================================
  //==========================================
  //==========================================
  //Read serial input...

  //if there is something to read on the serial port
  if (Serial.available()) 
  {
    //TODO: process message for sending back all current control values.
 
  } //if (Serial.available()) 

}

//=====================================================
//=====================================================
//=====================================================
//Converts a control value into a param/MIDI CC value

byte ConvertControlValToParamVal (byte control_num)
{
  byte result;

  result = (((controlParamData[control_num].cc_max_val - controlParamData[control_num].cc_min_val) * prevAnalogueValue[control_num]) / 1023.0) + controlParamData[control_num].cc_min_val;

  return result;
}

//=====================================================
//=====================================================
//=====================================================
//Sends a 3 byte MIDI message to the serial output

void SendMidiMessage (byte cmd_byte, byte data_byte_1, byte data_byte_2) 
{
  byte buf[3] = {cmd_byte, data_byte_1, data_byte_2};
  Serial.write (buf, 3);
  
//  Serial.print(buf[0]);
//  Serial.print(" ");
//  Serial.print(buf[1]);
//  Serial.print(" ");
//  Serial.println(buf[2]);
}

