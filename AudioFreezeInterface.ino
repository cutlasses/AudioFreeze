#include "AudioFreezeInterface.h"
#include "CompileSwitches.h"


#define LENGTH_DIAL_PIN         20
#define POSITION_DIAL_PIN       17
#define SPEED_DIAL_PIN          21
#define MIX_DIAL_PIN            16
#define FREEZE_BUTTON_PIN       2
#define FREEZE_BUTTON_IS_TOGGLE true
  
AUDIO_FREEZE_INTERFACE::AUDIO_FREEZE_INTERFACE() :
  m_length_dial( LENGTH_DIAL_PIN ),
  m_position_dial( POSITION_DIAL_PIN ),
  m_speed_dial( SPEED_DIAL_PIN ),
  m_mix_dial( MIX_DIAL_PIN ),
  m_freeze_button( FREEZE_BUTTON_PIN, FREEZE_BUTTON_IS_TOGGLE )
{  
}

void AUDIO_FREEZE_INTERFACE::setup()
{
  m_freeze_button.setup();
}

void AUDIO_FREEZE_INTERFACE::update()
{
  m_length_dial.update() ;
  m_position_dial.update();
  m_speed_dial.update();
  m_mix_dial.update();
  
  m_freeze_button.update();

#ifdef DEBUG_OUTPUT
  /*
  if( m_speed_dial.update() )
  {
    Serial.print("Speed ");
    Serial.print(m_speed_dial.value());
    Serial.print("\n");
  }
  if( m_mix_dial.update() )
  {
    Serial.print("Mix ");
    Serial.print(m_mix_dial.value());
    Serial.print("\n");   
  }
  if( m_length_dial.update() )
  {
    Serial.print("Length ");
    Serial.print(m_length_dial.value());
    Serial.print("\n");
  }
  if( m_position_dial.update() )
  {
    Serial.print("Position ");
    Serial.print(m_position_dial.value());
    Serial.print("\n");   
  }
  m_freeze_button.update();

  if( m_freeze_button.active() )
  {
    Serial.print("on\n");
  }
  */
#endif // DEBUG_OUTPUT
}

const DIAL& AUDIO_FREEZE_INTERFACE::length_dial() const
{
  return m_length_dial;
}

const DIAL& AUDIO_FREEZE_INTERFACE::position_dial() const
{
  return m_position_dial;
}

const DIAL& AUDIO_FREEZE_INTERFACE::speed_dial() const
{
  return m_speed_dial;
}

const DIAL& AUDIO_FREEZE_INTERFACE::mix_dial() const
{
  return m_mix_dial;
}

const BUTTON& AUDIO_FREEZE_INTERFACE::freeze_button() const
{
  return m_freeze_button;
}

