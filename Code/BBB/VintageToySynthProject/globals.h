///
//  globals.h
//
//  Created by Liam Lacey on 28/01/2016.
//
//
//

//==========================================================================
//Socket stuff.
//I needed to create the vintageToySynthProject directory in order for these sockets to be setup

//server socket
#define SOCK_BRAIN_FILENAME "/var/vintageToySynthProject/vintageBrain.serv"

//client sockets
#define SOCK_SOUND_ENGINE_FILENAME "/var/vintageToySynthProject/vintageSoundEngine.cli"

//==========================================================================
#define NUM_OF_VOICES 2

//==========================================================================
//MIDI defines

#define MIDI_NOTEOFF 0x80
#define MIDI_NOTEON 0x90
#define MIDI_PAT 0xA0
#define MIDI_CC 0xB0
#define MIDI_PROGRAM_CHANGE 0xC0
#define MIDI_CAT 0xD0
#define MIDI_PITCH_BEND 0xE0

#define MIDI_NOTEOFF_MAX 0x8F
#define MIDI_NOTEON_MAX 0x9F
#define MIDI_PAT_MAX 0xAF
#define MIDI_CC_MAX 0xBF
#define MIDI_PROGRAM_CHANGE_MAX 0xCF
#define MIDI_CAT_MAX 0xDF
#define MIDI_PITCH_BEND_MAX 0xEF

#define MIDI_CLOCK 0xF8
#define MIDI_CLOCK_START 0xFA
#define MIDI_CLOCK_CONTINUE 0xFB
#define MIDI_CLOCK_STOP 0xFC

#define MIDI_SYSEX_START 0xF0
#define MIDI_SYSEX_END 0xF7

#define MIDI_CHANNEL_BITS 0b00001111
#define MIDI_STATUS_BITS 0b11110000

//==========================================================================
//Patch parameter CC numbers

//Osc
#define PARAM_OSC_SINE_LEVEL 14
#define PARAM_OSC_TRI_LEVEL 15
#define PARAM_OSC_SAW_LEVEL 16
#define PARAM_OSC_PULSE_LEVEL 17
#define PARAM_OSC_PULSE_AMOUNT 3
#define PARAM_OSC_SQUARE_LEVEL 18

//Filter
#define PARAM_FILTER_FREQ 74
#define PARAM_FILTER_RESO 19
#define PARAM_FILTER_LP_MIX 26
#define PARAM_FILTER_BP_MIX 27
#define PARAM_FILTER_HP_MIX 28
#define PARAM_FILTER_NOTCH_MIX 29

//AEG
#define PARAM_AEG_ATTACK 73
#define PARAM_AEG_DECAY 75
#define PARAM_AEG_SUSTAIN 79
#define PARAM_AEG_RELEASE 72
#define PARAM_AEG_AMOUNT 7

//FEG
#define PARAM_FEG_ATTACK 22
#define PARAM_FEG_DECAY 23
#define PARAM_FEG_SUSTAIN 24
#define PARAM_FEG_RELEASE 25

//LFO
#define PARAM_LFO_SHAPE 50
#define PARAM_LFO_RATE 47
#define PARAM_LFO_DEPTH 48

//Keys
#define PARAM_KEYS_OCTAVE 114
#define PARAM_KEYS_SCALE 102
#define PARAM_KEYS_TRANSPOSE 106

//Voice
#define PARAM_VOICE_MODE 103

//Modulation depths
#define PARAM_MOD_VEL_AMP 63
#define PARAM_MOD_LFO_AMP 58
#define PARAM_MOD_LFO_FREQ 112
#define PARAM_MOD_LFO_RESO 56
#define PARAM_MOD_AT_FREQ 90
#define PARAM_MOD_AT_LFO_DEPTH 89

//Effects
#define PARAM_FX_DISTORTION_AMOUNT 13

//Global
#define PARAM_GLOBAL_VINTAGE_AMOUNT 107
#define PARAM_GLOBAL_VOLUME 9


//==========================================================================
//structure that stores info about a patch parameter

typedef struct
{
    uint8_t user_val;
    uint8_t user_min_val;
    uint8_t user_max_val;
    bool sound_param;
    bool patch_param;
    //I think only vintageSoundEngine cares about these 3 variables
    double voice_val;
    double voice_min_val;
    double voice_max_val;

} PatchParameterData;

//array that stores the default data for all patch parameters

static const PatchParameterData defaultPatchParameterData[128] =
{
    {}, //0
    {}, //1
    {}, //2
    {90, 0, 127, true, true, 0.7, 0., 1.}, //3 - PARAM_OSC_PULSE_AMOUNT
    {}, //4
    {}, //5
    {}, //6
    {127, 0, 127, true, true, 1., 0., 1.}, //7 - PARAM_AEG_AMOUNT
    {}, //8
    {64, 0, 127, false, false, 50, 0, 100}, //9 - PARAM_GLOBAL_VOLUME
    {}, //10
    {}, //11
    {}, //12
    {10, 0, 127, true, true, 0.1, 0., 1.}, //13 - PARAM_FX_DISTORTION_AMOUNT
    {127, 0, 127, true, true, 1., 0., 1.}, //14 - PARAM_OSC_SINE_LEVEL
    {127, 0, 127, true, true, 1., 0., 1.}, //15 - PARAM_OSC_TRI_LEVEL
    {127, 0, 127, true, true, 1., 0., 1.}, //16 - PARAM_OSC_SAW_LEVEL
    {127, 0, 127, true, true, 1., 0., 1.}, //17 - PARAM_OSC_PULSE_LEVEL
    {127, 0, 127, true, true, 1., 0., 1.}, //18 - PARAM_OSC_SQUARE_LEVEL
    {5, 0, 127, true, true, 0.1, 0.1, 50.}, //19 - PARAM_FILTER_RESO
    {}, //20
    {}, //21
    {0, 0, 127, true, true, 0, 0, 5000}, //22 - PARAM_FEG_ATTACK
    {2, 0, 127, true, true, 400, 0, 5000}, //23 - PARAM_FEG_DECAY
    {20, 0, 127, true, true, 0.7, 0., 1.}, //24 - PARAM_FEG_SUSTAIN
    {20, 0, 127, true, true, 3000, 0, 5000}, //25 - PARAM_FEG_RELEASE
    {127, 0, 127, true, true, 1., 0, 1.}, //26 - PARAM_FILTER_LP_MIX
    {0, 0, 127, true, true, 0., 0, 1.}, //27 - PARAM_FILTER_BP_MIX
    {0, 0, 127, true, true, 0., 0, 1.}, //28 - PARAM_FILTER_HP_MIX
    {0, 0, 127, true, true, 0., 0, 1.}, //29 - PARAM_FILTER_NOTCH_MIX
    {}, //30
    {}, //31
    {}, //32
    {}, //33
    {}, //34
    {}, //35
    {}, //36
    {}, //37
    {}, //38
    {}, //39
    {}, //40
    {}, //41
    {}, //42
    {}, //43
    {}, //44
    {}, //45
    {}, //46
    {3, 0, 127, true, true, 200, 20, 30000}, //47 - PARAM_LFO_RATE
    {64, 0, 127, true, true, 0., -1., 1.}, //48 - PARAM_LFO_DEPTH
    {}, //49
    {0, 0, 3, true, true, 0, 0, 3}, //50 - PARAM_LFO_SHAPE
    {}, //51
    {}, //52
    {}, //53
    {}, //54
    {}, //55
    {64, 0, 127, true, true, 0., -1., 1.}, //56 - PARAM_MOD_LFO_RESO
    {}, //57
    {64, 0, 127, true, true, 0., -1., 1.}, //58 - PARAM_MOD_LFO_AMP
    {}, //59
    {}, //60
    {}, //61
    {}, //62
    {127, 0, 127, true, true, 1., -1., 1.}, //63 - PARAM_MOD_VEL_AMP
    {}, //64
    {}, //65
    {}, //66
    {}, //67
    {}, //68
    {}, //69
    {}, //70
    {}, //71
    {20, 0, 127, true, true, 2000, 0, 5000}, //72 - PARAM_AEG_RELEASE
    {0, 0, 127, true, true, 0, 0, 5000}, //73 - PARAM_AEG_ATTACK
    {20, 0, 127, true, true, 5000, 50, 20000}, //74 - PARAM_FILTER_FREQ
    {2, 0, 127, true, true, 200, 0, 5000}, //75 - PARAM_AEG_DECAY
    {}, //76
    {}, //77
    {}, //78
    {20, 0, 127, true, true, 0.2, 0., 1.}, //79 - PARAM_AEG_SUSTAIN
    {}, //80
    {}, //81
    {}, //82
    {}, //83
    {}, //84
    {}, //85
    {}, //86
    {}, //87
    {}, //88
    {64, 0, 127, true, true, 0., -1., 1.}, //89 - PARAM_MOD_AT_LFO_DEPTH
    {64, 0, 127, true, true, 0., -1., 1.}, //90 - PARAM_MOD_AT_FREQ
    {}, //91
    {}, //92
    {}, //93
    {}, //94
    {}, //95
    {}, //96
    {}, //97
    {}, //98
    {}, //99
    {}, //100
    {}, //101
    {0, 0, 2, false, true, 0, 0, 0}, //102 - PARAM_KEYS_SCALE
    {0, 0, 127, false, true, 0, 0, 0}, //103 - PARAM_VOICE_MODE
    {}, //104
    {}, //105
    {6, 0, 12, false, true, 0, 0, 0}, //106 - PARAM_KEYS_TRANSPOSE
    {0, 0, 127, false, true, 0, 0, 0}, //107 - PARAM_GLOBAL_VINTAGE_AMOUNT
    {}, //108
    {}, //109
    {}, //110
    {}, //111
    {64, 0, 127, true, true, 0., -1., 1.}, //112 - PARAM_MOD_LFO_FREQ
    {}, //113
    {2, 0, 4, false, true, 0, 0, 0}, //114 - PARAM_KEYS_OCTAVE
};


//===================================================================
//Function that scales a number to a new number based on a new range

static float scaleValue (float value, float minValue, float maxValue, float minRange, float maxRange)
{
    //minValue is the min range of the value coming in.
    //maxValue is the max range of the value coming in.
    //minRange is the min range of the value going out.
    //maxRange is the max range of the value going out.
    
    return (((maxRange - minRange) *
             (value - minValue)) /
            (maxValue - minValue)) + minRange;
}