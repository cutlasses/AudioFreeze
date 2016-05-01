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
  bool          m_is_active;

  Bounce        m_bounce;

public:

  BUTTON( int data_pin, bool is_toggle );

  bool          active() const;

  void          setup();
  void          update();
};

