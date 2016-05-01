#include "AudioFreezeInterface.h"


#define LENGTH_DIAL_PIN         16
#define POSITION_DIAL_PIN       17
#define FREEZE_BUTTON_PIN       0
#define FREEZE_BUTTON_IS_TOGGLE false
  
AUDIO_FREEZE_INTERFACE::AUDIO_FREEZE_INTERFACE() :
  m_length_dial( LENGTH_DIAL_PIN ),
  m_position_dial( POSITION_DIAL_PIN ),
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
    m_freeze_button.update();
  /* 
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
}

const DIAL& AUDIO_FREEZE_INTERFACE::length_dial() const
{
  return m_length_dial;
}

const DIAL& AUDIO_FREEZE_INTERFACE::position_dial() const
{
  return m_position_dial;
}

const BUTTON& AUDIO_FREEZE_INTERFACE::freeze_button() const
{
  return m_freeze_button;
}
