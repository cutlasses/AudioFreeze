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
  int           m_data_pin;
  bool          m_is_toggle;
  bool          m_prev_is_active;
  bool          m_is_active;

  Bounce        m_bounce;

public:

  BUTTON( int data_pin, bool is_toggle );

  bool          active() const;
  bool          single_click() const;

  void          setup();
  void          update();
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

