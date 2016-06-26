#pragma once

#include "Interface.h"

class AUDIO_FREEZE_INTERFACE
{
  static const int  LENGTH_DIAL_PIN         = 20;
  static const int  POSITION_DIAL_PIN       = 17;
  static const int  SPEED_DIAL_PIN          = 21;
  static const int  MIX_DIAL_PIN            = 16;
  static const int  FREEZE_BUTTON_PIN       = 2;
  static const int  MODE_BUTTON_PIN         = 1;
  static const int  LED_1_PIN               = 4;
  static const int  LED_2_PIN               = 3;
  static const int  LED_3_PIN               = 5;

  static const bool FREEZE_BUTTON_IS_TOGGLE = true;
  static const int  NUM_LEDS                = 3;
  
  DIAL              m_length_dial;
  DIAL              m_position_dial;
  DIAL              m_speed_dial;
  DIAL              m_mix_dial;

  BUTTON            m_freeze_button;
  BUTTON            m_mode_button;
  
  LED               m_leds[NUM_LEDS];

  int               m_current_mode;

public:

  AUDIO_FREEZE_INTERFACE();

  void          setup();
  void          update();

  const DIAL&   length_dial() const;
  const DIAL&   position_dial() const;
  const DIAL&   speed_dial() const;
  const DIAL&   mix_dial() const;
  const BUTTON& freeze_button() const;

  int           mode() const;
};

