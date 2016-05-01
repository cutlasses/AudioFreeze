#include <Audio.h>
#include <Wire.h>
#include <SPI.h>

#include "AudioFreezeEffect.h"
#include "AudioFreezeInterface.h"


AudioInputI2S            audio_input;
AUDIO_FREEZE_EFFECT      audio_freeze_effect;
AudioOutputI2S           audio_output;

AudioConnection          patch_cord_L1( audio_input, 0, audio_freeze_effect, 0 );
AudioConnection          patch_cord_L2( audio_freeze_effect, 0, audio_output, 0 );
//AudioConnection          patch_cord_L1( audio_input, 0, audio_output, 0 );
AudioConnection          patch_cord_R1( audio_input, 1, audio_output, 1 );      // right channel passes straight through
AudioControlSGTL5000     sgtl5000_1;

AUDIO_FREEZE_INTERFACE   audio_freeze_interface;

//////////////////////////////////////

void setup()
{
  Serial.begin(9600);
  AudioMemory(FREEZE_QUEUE_SIZE+8);
  
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.85);
  
  SPI.setMOSI(7);
  SPI.setSCK(14);

  audio_freeze_interface.setup();

  delay(1000);
}

void loop()
{
  // adding a test comment
  
  audio_freeze_interface.update();

  if( audio_freeze_interface.freeze_button().active() != audio_freeze_effect.is_freeze_active() )
  {
    audio_freeze_effect.set_freeze( audio_freeze_interface.freeze_button().active() );

    if( audio_freeze_interface.freeze_button().active() )
    {
      Serial.print("on\n");
      Serial.print(AudioMemoryUsage());
      Serial.print(" ");
      Serial.print(AudioMemoryUsageMax());
      Serial.print("\n");
    }
    else
    {
      Serial.print("off\n");
    }
  }

  audio_freeze_effect.set_length( audio_freeze_interface.length_dial().value() );
  audio_freeze_effect.set_centre( audio_freeze_interface.position_dial().value() );
  
  /*
  float length = m_length_dial.update_current_value();

  m_freeze_button.update();

  if( m_freeze_button.active() )
  {
    Serial.print("on\n");
  }
  else
  {
    Serial.print("off\n");
  }

  Serial.print("length is: ");
  Serial.print(length);
  Serial.print("\n");

  delay(100);
  */
}




