#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>     // Arduino compiler can get confused if you don't include include all required headers in this file?!?

#include "AudioFreezeEffect.h"
#include "AudioFreezeInterface.h"
#include "CompileSwitches.h"
#include "Util.h"

#define MIX_FREEZE_CHANNEL    0
#define MIX_ORIGINAL_CHANNEL  1


AudioInputI2S            audio_input;
AUDIO_FREEZE_EFFECT      audio_freeze_effect;
AudioMixer4              audio_mixer;
AudioOutputI2S           audio_output;

AudioConnection          patch_cord_L1( audio_input, 0, audio_freeze_effect, MIX_FREEZE_CHANNEL );
AudioConnection          patch_cord_L2( audio_freeze_effect, 0, audio_mixer, 0 );
AudioConnection          patch_cord_L3( audio_input, 0, audio_mixer, MIX_ORIGINAL_CHANNEL );
AudioConnection          patch_cord_L4( audio_mixer, 0, audio_output, 0 );
//AudioConnection          patch_cord_L1( audio_input, 0, audio_output, 0 );    // left channel passes straight through (for testing)
AudioConnection          patch_cord_R1( audio_input, 1, audio_output, 1 );      // right channel passes straight through
AudioControlSGTL5000     sgtl5000_1;

AUDIO_FREEZE_INTERFACE   audio_freeze_interface;

//////////////////////////////////////

void setup()
{
  Serial.begin(9600);

  serial_port_initialised = true;

  AudioMemory(8);
  
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.8f);

  sgtl5000_1.lineInLevel( 10 );  // 0.56volts p-p
  sgtl5000_1.lineOutLevel( 13 );  // 3.16volts p-p
  
  SPI.setMOSI(7);
  SPI.setSCK(14);

  audio_freeze_interface.setup();

  //audio_freeze_effect.set_wow_frequency_range( 1.25f, 3.85f );
  //audio_freeze_effect.set_flutter_frequency_range( 14.0f, 25.0f );

  delay(1000);
  
  DEBUG_OUTPUT("Setup finished\n");
}

void loop()
{  
  audio_freeze_interface.update();

  if( audio_freeze_interface.freeze_button().active() != audio_freeze_effect.is_freeze_active() )
  {
    audio_freeze_effect.set_freeze( audio_freeze_interface.freeze_button().active() );

    // no mix dial, so set mix directly
    if( audio_freeze_interface.freeze_button().active() )
    {
      audio_mixer.gain( MIX_FREEZE_CHANNEL,1.0f );
      audio_mixer.gain( MIX_ORIGINAL_CHANNEL, 0.0f );
    }
    else
    {
      audio_mixer.gain( MIX_FREEZE_CHANNEL,0.0f );
      audio_mixer.gain( MIX_ORIGINAL_CHANNEL, 1.0f );
    }
  }
  
   // use the mix dial to control wow/flutter
  const float wow_flutter_amount = clamp( audio_freeze_interface.mix_dial().value(), 0.0f, 1.0f );
  const float max_wow( 1.0f );
  const float max_flutter( 0.4f );
  audio_freeze_effect.set_wow_amount( wow_flutter_amount * max_wow ); 
  audio_freeze_effect.set_flutter_amount( wow_flutter_amount * max_flutter ); 

  audio_freeze_effect.set_length( audio_freeze_interface.length_dial().value() );
  audio_freeze_effect.set_centre( audio_freeze_interface.position_dial().value() );
  audio_freeze_effect.set_speed( audio_freeze_interface.speed_dial().value() );

#ifdef DEBUG_OUTPUT
  const int processor_usage = AudioProcessorUsage();
  if( processor_usage > 30 )
  {
    Serial.print( "Performance spike: " );
    Serial.print( processor_usage );
    Serial.print( "\n" );
  }
#endif
}




