//Toy Piano Synth Code, for front velostat switches only using multiplexers for the first and last 8 keys, with dedicated inputs for the middle two keys.
//TODO: improve this description
//*/

//TODO: document and DEFINE the inputs and outputs we are using for this code

//FIXME: some keys/sensors don't work as well as others - sensitivity is bad and can't reach high pressure values.
//If I can't fix these in the hardware, add fixes in the software (e.g. have upper limit pressure values for each key).

//REMEMBER THAT ANY SERIAL DEBUGGING HERE MAY SCREW UP THE SERIAL COMMS TO THE BBB!

//========================================================================================

//Number of keys on the piano
const int NUM_OF_KEYS = 18;

//MIDI channel we want to use
const int midiChan = 0;

//=======================================================================================

int INIT_PHASE_TIME = 10;

int PRESSURE_OFFSET = 100;

//Variables for storing certain values
int triggerVal[NUM_OF_KEYS] = {0};
bool noteIsOn[NUM_OF_KEYS] = {false};

unsigned long triggerTime[NUM_OF_KEYS] = {0};
int triggerInitVal[NUM_OF_KEYS] = {0};
bool inInitPhase[NUM_OF_KEYS] = {false};
byte prevPressureVal[NUM_OF_KEYS] = {0};

bool testLedState = false;

//array that holds the max analogue values they each sensor key can reach
const int maxSensorVals[NUM_OF_KEYS] =
{
  530, //0
  630, //1
  570, //2
  670, //3
  680, //4
  560, //5
  629, //6
  650, //7
  640, //8
  630, //9
  580, //10
  630, //11
  570, //12
  610, //13
  580, //14
  270, //15
  650, //16
  590 //17
};

void setup()
{
  //For sending MIDI messages to BBB.
  //We don't need to use the MIDI baud rate (31250) here, as we're sending the messages to a general
  //serial output rather than a MIDI-specific output.
  Serial.begin(38400);

  pinMode(2, OUTPUT);    // mux1 s0
  pinMode(3, OUTPUT);    // mux1 s1
  pinMode(4, OUTPUT);    // mux1 s2

  pinMode(5, OUTPUT);    // mux2 s0
  pinMode(6, OUTPUT);    // mux2 s1
  pinMode(7, OUTPUT);    // mux2 s2

  pinMode(13, OUTPUT);    // LED
}

void loop()
{
  //repeat the below code for each input/key
  for (int count; count < NUM_OF_KEYS; count++)
  {
    triggerVal[count] = 0;

    //==========================================
    //==========================================
    //==========================================
    //Read input
    //FIXME: this code could be tidier

    if (count < 8)
    {
      //select the bit/input
      int r0 = bitRead (count, 0);
      int r1 = bitRead (count, 1);
      int r2 = bitRead (count, 2);
      digitalWrite (2, r0);
      digitalWrite (3, r1);
      digitalWrite (4, r2);

      //read the input value
      triggerVal[count] = analogRead(A0);
    }

    else if (count == 8)
    {
      triggerVal[count] = analogRead(A1);
    }

    else if (count == 9)
    {
      triggerVal[count] = analogRead(A2);
    }

    else if (count > 9)
    {
      //select the bit/input

      int mux_input = count - 10;

      int r0 = bitRead (mux_input, 0);
      int r1 = bitRead (mux_input, 1);
      int r2 = bitRead (mux_input, 2);
      digitalWrite (5, r0);
      digitalWrite (6, r1);
      digitalWrite (7, r2);

      //read the input value
      triggerVal[count] = analogRead(A3);
    }

    //    if (triggerVal[count] > 0)
    //    {
    //      Serial.print(count);
    //      Serial.print(": ");
    //      Serial.println(triggerVal[count]);
    //    }

    //==========================================
    //==========================================
    //==========================================
    //Process input

    //==========================================
    //Initial press stuff (for working out velocity)

    //========================
    //if we've got the initial press of a key
    if (triggerVal[count] > 0 && inInitPhase[count] == false && noteIsOn[count] == false)
    {
      //stored the current time
      triggerTime[count] = millis();

      triggerInitVal[count] = 0;
      inInitPhase[count] = true;
    }

    //========================
    //if we're still holding down a key within the initial phase
    else if (triggerVal[count] > 0 && inInitPhase[count] == true && millis() < (triggerTime[count] + INIT_PHASE_TIME) && noteIsOn[count] == false)
    {
      //if the current val is higher than the current init val
      if (triggerVal[count] > triggerInitVal[count])
      {
        //store the current val
        triggerInitVal[count] = triggerVal[count];
      }
    }

    //========================
    //if a key is released before the initial phase is over
    else if (triggerVal[count] == 0 && inInitPhase[count] == true && noteIsOn[count] == false)
    {
      //reset inInitPhase
      inInitPhase[count] == false;
      triggerInitVal[count] = 0;
    }

    //==========================================
    //Process note-on's and note-off's

    //========================
    //if in the initial phase and current time is +INIT_PHASE_TIME of the initial trigger time
    else if (triggerVal[count] > 0 && inInitPhase[count] == true && millis() >= (triggerTime[count] + INIT_PHASE_TIME) && noteIsOn[count] == false)
    {
      //Work out a velocity value based on the init val...

      //Serial.print("Init val: ");
      //Serial.println(triggerInitVal[count]);

      //use two-thirds of the max sensor value as the top velocity value, as it seems to work well.
      int velocity = (127.0 * (float)triggerInitVal[count]) / (((float)maxSensorVals[count] / 3.0) * 2.0);

      if (velocity > 127)
      {
        velocity = 127;
      }

      inInitPhase[count] = false;

      if (velocity > 0)
      {
        //Send MIDI note-on message
        SendMidiMessage (0x90 + midiChan, count, velocity);

        //flag that the note is on
        noteIsOn[count] = true;

        //test feedback
        digitalWrite (13, HIGH);

      } //if (velocity > 0)

    }

    //========================
    //if the note is currently on and we get a key release
    else if (triggerVal[count] == 0 && noteIsOn[count] == true)
    {
      //Send MIDI note-on message
      SendMidiMessage (0x80 + midiChan, count, 0);

      noteIsOn[count] = false;

      //test feedback
      digitalWrite (13, LOW);
    }

    //Process poly pressure

    //========================
    //If we've got a key/pressure value of above the init input val whilst the note is on
    else if (triggerVal[count] >= (triggerInitVal[count] + PRESSURE_OFFSET) && triggerInitVal[count] != 0 && noteIsOn[count] == true)
    {
      //Work out pressure value based on the difference between the current val and the init val...
      //FIXME: this algorithm only really works for light presses.
      //If a heavy press, we don't want so much offset - implement an equation for this.

      //FIXME: make sure a pressure value of 0 is sent on key release if needed

      int init_pressure = triggerInitVal[count] + PRESSURE_OFFSET;

      int pressure = (((127.0 - 0) * (triggerVal[count] - init_pressure)) / ((float)maxSensorVals[count] - init_pressure)) + 0;

      if (pressure > 127)
      {
        pressure = 127;
        //Serial.print("Pressure: ");
        //Serial.println(pressure);
      }
      else if (pressure < 0)
      {
        pressure = 0;
        //Serial.print("Pressure: ");
        //Serial.println(pressure);
      }
      else
      {
        //Serial.print("Pressure: ");
        //Serial.println(pressure);
      }

      //if we have a new pressure value for this key
      if (pressure != prevPressureVal[count])
      {
        //Send MIDI poly aftertouch message...
        SendMidiMessage (0xA0 + midiChan, count, pressure);

        //store the pressure value
        prevPressureVal[count] = pressure;
      }

    }

    //==========================================

  } //for (int count; count < NUM_OF_KEYS; count++)

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

