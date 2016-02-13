
const int NUM_OF_CONTROLS = 1;
const int NUM_OF_DEMUX = 1;
const int NUM_OF_MUX = 1;

int controlVal[NUM_OF_CONTROLS];

void setup()
{
  //For sending MIDI messages to BBB. 
  //We don't need to use the MIDI baud rate (31250) here, as we're sending the messages to a general
  //serial output rather than a MIDI-specific output.
  Serial.begin(38400);

  //demux 1...
  //common output
  pinMode(2, OUTPUT); 
  //select outputs  
  pinMode(3, OUTPUT);    
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);      
  
  //mux 1
  pinMode(A0, INPUT);  
  
  for (int i = 0; i < NUM_OF_CONTROLS; i++)
  {
    controlVal[i] = 0;
  }
}


void loop()
{
  int control_num = 0;
  
  //example for reading from the first input of the first mux

  //input pin number on mux 1
  int mux_in_pin_num = 7;
  //input pin number on mux 1 split into bit values
  int mb1 = bitRead (mux_in_pin_num, 0);   
  int mb2 = bitRead (mux_in_pin_num, 1);     
  int mb3 = bitRead (mux_in_pin_num, 2); 
  
  //output pin numbers on demux connect to the select pins on mux1
  int mux_s_pin_1 = 0;
  int mux_s_pin_2 = 1;
  int mux_s_pin_3 = 2;

  //select mux_pin_num input pin on mux 1, by sequentially selecting the 3
  //select mins on demux 1 with the values
  int r0 = bitRead (mux_s_pin_1, 0);   
  int r1 = bitRead (mux_s_pin_1, 1);     
  int r2 = bitRead (mux_s_pin_1, 2); 
  digitalWrite (3, r0);
  digitalWrite (4, r1);
  digitalWrite (5, r2);
  
  digitalWrite (2, mb1);
  
  r0 = bitRead (mux_s_pin_2, 0);   
  r1 = bitRead (mux_s_pin_2, 1);     
  r2 = bitRead (mux_s_pin_2, 2); 
  digitalWrite (3, r0);
  digitalWrite (4, r1);
  digitalWrite (5, r2);
  
  digitalWrite (2, mb2);
  
  r0 = bitRead (mux_s_pin_3, 0);   
  r1 = bitRead (mux_s_pin_3, 1);     
  r2 = bitRead (mux_s_pin_3, 2); 
  digitalWrite (3, r0);
  digitalWrite (4, r1);
  digitalWrite (5, r2);
  
  digitalWrite (2, mb3);
  
  Serial.println(analogRead(A0));
  
  
  delay(1);        // delay in between reads for stability
}
