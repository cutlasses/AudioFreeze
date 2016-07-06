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

  AudioMemory(8);
  
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.8f);

  sgtl5000_1.lineInLevel( 10 );  // 0.56volts p-p
  sgtl5000_1.lineOutLevel( 13 );  // 3.16volts p-p
  
  SPI.setMOSI(7);
  SPI.setSCK(14);

  audio_freeze_interface.setup();

  delay(1000);
  
#ifdef DEBUG_OUTPUT
  Serial.print("Setup finished\n");
#endif // DEBUG_OUTPUT
}

void loop()
{  
  audio_freeze_interface.update();

  if( audio_freeze_interface.freeze_button().active() != audio_freeze_effect.is_freeze_active() )
  {
    audio_freeze_effect.set_freeze( audio_freeze_interface.freeze_button().active() );
  }

  audio_freeze_effect.set_length( audio_freeze_interface.length_dial().value() );
  audio_freeze_effect.set_centre( audio_freeze_interface.position_dial().value() );
  audio_freeze_effect.set_speed( audio_freeze_interface.speed_dial().value() );

  if( audio_freeze_interface.mode() == 1 )
  {
    audio_freeze_effect.set_reverse( true );
  }
  else
  {
    audio_freeze_effect.set_reverse( false );
  }

  if( audio_freeze_interface.freeze_button().active() )
  {
    const float freeze_mix_amount = clamp( audio_freeze_interface.mix_dial().value(), 0.0f, 1.0f );
    
    audio_mixer.gain( MIX_FREEZE_CHANNEL, freeze_mix_amount );
    audio_mixer.gain( MIX_ORIGINAL_CHANNEL, 1.0f - freeze_mix_amount );
  }
  else
  {
    audio_mixer.gain( MIX_FREEZE_CHANNEL, 0.0f );
    audio_mixer.gain( MIX_ORIGINAL_CHANNEL, 1.0f );

    // only adjust bit-depth when freeze is not active, need to write the data in the new bit-depth before it can be played
    if( audio_freeze_interface.reduced_bit_depth() )
    {
      audio_freeze_effect.set_bit_depth( 8 );
    }
    else
    {
      audio_freeze_effect.set_bit_depth( 16 );
    }
  }
}




