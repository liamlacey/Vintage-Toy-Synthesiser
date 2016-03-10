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
const byte NUM_OF_CONTROLS = 43;

//for dev
const byte FIRST_CONTROL = 0;
const byte LAST_CONTROL = 42;

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
   {.cc_num = 74, .cc_min_val = 0, .cc_max_val = 127}, //0 - PARAM_FILTER_CUTOFF
   {.cc_num = 19, .cc_min_val = 0, .cc_max_val = 127}, //1 - PARAM_FILTER_RESO
   {.cc_num = 26, .cc_min_val = 0, .cc_max_val = 127}, //2 - PARAM_FILTER_LP_MIX
   {.cc_num = 28, .cc_min_val = 0, .cc_max_val = 127}, //3 - PARAM_FILTER_HP_MIX
   {.cc_num = 27, .cc_min_val = 0, .cc_max_val = 127}, //4 - PARAM_FILTER_BP_MIX
   {.cc_num = 29, .cc_min_val = 0, .cc_max_val = 127}, //5 - PARAM_FILTER_NOTCH_MIX
   {.cc_num = 50, .cc_min_val = 0, .cc_max_val = 3}, //6 - PARAM_LFO_SHAPE
   {.cc_num = 47, .cc_min_val = 0, .cc_max_val = 127}, //7 - PARAM_LFO_RATE
   {.cc_num = 48, .cc_min_val = 0, .cc_max_val = 127}, //8 - PARAM_LFO_DEPTH
   {.cc_num = 14, .cc_min_val = 0, .cc_max_val = 127}, //9 - PARAM_OSC_SINE_LEVEL
   {.cc_num = 15, .cc_min_val = 0, .cc_max_val = 127}, //10 - PARAM_OSC_TRI_LEVEL
   {.cc_num = 16, .cc_min_val = 0, .cc_max_val = 127}, //11 - PARAM_OSC_SAW_LEVEL
   {.cc_num = 18, .cc_min_val = 0, .cc_max_val = 127}, //12 - PARAM_OSC_SQUARE_LEVEL
   {.cc_num = 17, .cc_min_val = 0, .cc_max_val = 127}, //13 - PARAM_OSC_PULSE_LEVEL
   {.cc_num = 3, .cc_min_val = 0, .cc_max_val = 127}, //14 - PARAM_OSC_PULSE_AMOUNT
   {.cc_num = 7, .cc_min_val = 0, .cc_max_val = 127}, //15 - PARAM_AEG_AMOUNT
   {.cc_num = 73, .cc_min_val = 0, .cc_max_val = 127}, //16 - PARAM_AEG_ATTACK
   {.cc_num = 75, .cc_min_val = 0, .cc_max_val = 127}, //17 - PARAM_AEG_DECAY
   {.cc_num = 79, .cc_min_val = 0, .cc_max_val = 127}, //18 - PARAM_AEG_SUSTAIN
   {.cc_num = 72, .cc_min_val = 0, .cc_max_val = 127}, //19 - PARAM_AEG_RELEASE
   {.cc_num = 13, .cc_min_val = 0, .cc_max_val = 127}, //20 - PARAM_FX_DISTORTION_AMOUNT
   {.cc_num = 33, .cc_min_val = 40, .cc_max_val = 88}, //21 - PARAM_OSC_SINE_NOTE
   {.cc_num = 34, .cc_min_val = 40, .cc_max_val = 88}, //22 - PARAM_OSC_TRI_NOTE
   {.cc_num = 35, .cc_min_val = 40, .cc_max_val = 88}, //23 - PARAM_OSC_SAW_NOTE
   {.cc_num = 37, .cc_min_val = 40, .cc_max_val = 88}, //24 - PARAM_OSC_SQUARE_NOTE
   {.cc_num = 36, .cc_min_val = 40, .cc_max_val = 88}, //25 - PARAM_OSC_PULSE_NOTE
   {.cc_num = 20, .cc_min_val = 0, .cc_max_val = 127}, //26 - PARAM_OSC_PHASE_SPREAD
   {.cc_num = 22, .cc_min_val = 0, .cc_max_val = 127}, //27 - PARAM_FEG_ATTACK
   {.cc_num = 23, .cc_min_val = 0, .cc_max_val = 127}, //28 - PARAM_FEG_DECAY
   {.cc_num = 24, .cc_min_val = 0, .cc_max_val = 127}, //29 - PARAM_FEG_SUSTAIN
   {.cc_num = 25, .cc_min_val = 0, .cc_max_val = 127}, //30 - PARAM_FEG_RELEASE
   {.cc_num = 107, .cc_min_val = 0, .cc_max_val = 127}, //31 - PARAM_GLOBAL_VINTAGE_AMOUNT
   {.cc_num = 102, .cc_min_val = 0, .cc_max_val = 7}, //32 - PARAM_KEYS_SCALE
   {.cc_num = 114, .cc_min_val = 61, .cc_max_val = 67}, //33 - PARAM_KEYS_OCTAVE
   {.cc_num = 106, .cc_min_val = 58, .cc_max_val = 70}, //34 - PARAM_KEYS_TRANSPOSE
   {.cc_num = 103, .cc_min_val = 0, .cc_max_val = 127}, //35 - PARAM_VOICE_MODE
   {.cc_num = 58, .cc_min_val = 0, .cc_max_val = 127}, //36 - PARAM_MOD_LFO_AMP
   {.cc_num = 112, .cc_min_val = 0, .cc_max_val = 127}, //37 - PARAM_MOD_LFO_CUTOFF
   {.cc_num = 56, .cc_min_val = 0, .cc_max_val = 127}, //38 - PARAM_MOD_LFO_RESO
   {.cc_num = 9, .cc_min_val = 0, .cc_max_val = 100}, //39 - PARAM_GLOBAL_VOLUME
   {.cc_num = 63, .cc_min_val = 0, .cc_max_val = 127}, //40 - PARAM_MOD_VEL_AMP
   {.cc_num = 109, .cc_min_val = 0, .cc_max_val = 127}, //41 - PARAM_MOD_VEL_CUTOFF
   {.cc_num = 110, .cc_min_val = 0, .cc_max_val = 127}, //42 - PARAM_MOD_VEL_RESO
};

//FOR DEVELOPMENT
//ControlParamData controlParamData[NUM_OF_CONTROLS] =
//{
//
//   {.cc_num = 0, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 1, .cc_min_val = 0, .cc_max_val = 127}, 
//   {.cc_num = 2, .cc_min_val = 0, .cc_max_val = 127}, 
//   {.cc_num = 3, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 4, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 5, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 6, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 7, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 8, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 9, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 10, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 11, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 12, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 13, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 14, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 15, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 16, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 17, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 18, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 19, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 20, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 21, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 22, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 23, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 24, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 25, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 26, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 27, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 28, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 29, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 30, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 31, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 32, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 33, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 34, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 35, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 36, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 37, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 38, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 39, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 40, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 41, .cc_min_val = 0, .cc_max_val = 127},
//   {.cc_num = 42, .cc_min_val = 0, .cc_max_val = 127},
//};

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
  for (byte control_num = FIRST_CONTROL; control_num <= LAST_CONTROL; control_num++)
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
      input_to_read = A2;
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
      int b3 = bitRead (mux_input_pin, 3); 
      digitalWrite (first_select_pin, b0);
      digitalWrite (first_select_pin + 1, b1);
      digitalWrite (first_select_pin + 2, b2);
      digitalWrite (first_select_pin + 3, b3);

      //read the input value
      int read_val = analogRead (input_to_read);

      //==========================================
      //==========================================
      //==========================================
      //Process analogue control input...

      //if the read control value is greater that +/-5 from the last value
      //this is a quick dirty hack to prevent jitter
      if ((read_val > prevAnalogueValue[control_num] + 5) || 
          (read_val < prevAnalogueValue[control_num] - 5) || 
          (read_val == 0 && prevAnalogueValue[control_num] != 0) ||
          (read_val == 1023 && prevAnalogueValue[control_num] != 1023)/* ||
          (read_val == 512 && prevAnalogueValue[control_num] != 512)*/)
      {

        // Serial.print(control_num);
        // Serial.print(" ");
        // Serial.println(read_val);
        
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

      delay (1);

  } //for (byte control_num; control_num < NUM_OF_CONTROLS; control_num++)

  //==========================================
  //==========================================
  //==========================================
  //Read serial input...

  //if there is something to read on the serial port
  if (Serial.available()) 
  {
      Serial.println ("Received messages from serial input");
      
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

  result = ((((float)controlParamData[control_num].cc_max_val - (float)controlParamData[control_num].cc_min_val) * (float)prevAnalogueValue[control_num]) / 1023.0) + (float)controlParamData[control_num].cc_min_val;

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

