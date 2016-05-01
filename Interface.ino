
#include "Interface.h"

#define BOUNCE_TIME         10

DIAL::DIAL( int data_pin ) :
  m_data_pin( data_pin ),
  m_current_value( 0 )
{

}

bool DIAL::update()
{
  int new_value = analogRead( m_data_pin );
  
  if( new_value != m_current_value )
  {
    m_current_value = new_value;
    return true;
  }

  return false;
}

float DIAL::value() const
{
  return m_current_value / 1024.0f;
}

//////////////////////////////////////

BUTTON::BUTTON( int data_pin, bool is_toggle ) :
  m_data_pin( data_pin ),
  m_is_toggle( is_toggle ),
  m_is_active( false ),
  m_bounce( m_data_pin, BOUNCE_TIME )
{
}

bool BUTTON::active() const
{
  return m_is_active;
}

void BUTTON::setup()
{
  pinMode( m_data_pin, INPUT_PULLUP );
}

void BUTTON::update()
{
  m_bounce.update();

  if( m_bounce.fallingEdge() )
  {
    if( m_is_toggle )
    {
      m_is_active = !m_is_active;
    }
    else
    {
      m_is_active = true;
    }
  }
  else if( m_bounce.risingEdge() )
  {
    if( !m_is_toggle )
    {
      m_is_active = false;
    }
  }
}

