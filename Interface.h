#pragma once

#include <Bounce.h>

class DIAL
{
  int           m_data_pin;

  int           m_current_value;
  
public:

  DIAL( int data_pin );

  bool          update();
  float         value() const;  
};

//////////////////////////////////////

class BUTTON
{
  int16_t       m_data_pin;
  int16_t       m_is_toggle : 1;
  int16_t       m_prev_is_active : 1;
  int16_t       m_is_active : 1;
  int16_t       m_down_time_valid : 1;
  int32_t       m_down_time_stamp;
  int32_t       m_down_time_curr;

  Bounce        m_bounce;

public:

  BUTTON( int data_pin, bool is_toggle );

  bool          active() const;
  bool          single_click() const;

  int32_t       down_time_ms() const;

  void          setup();
  void          update( int32_t time_ms );
};

//////////////////////////////////////

class LED
{
  int           m_data_pin;
  int           m_brightness;
  bool          m_is_active;

public:

  LED();              // to allow for arrays
  LED( int data_pin );

  void          set_active( bool active );
  void          set_brightness( float brightness );
  
  void          setup();
  void          update();       
};

