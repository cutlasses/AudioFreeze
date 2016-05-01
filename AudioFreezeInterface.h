#pragma once

#include "Interface.h"

class AUDIO_FREEZE_INTERFACE
{
  DIAL          m_length_dial;
  DIAL          m_position_dial;

  BUTTON        m_freeze_button;

public:

  AUDIO_FREEZE_INTERFACE();

  void          setup();
  void          update();

  const DIAL&   length_dial() const;
  const DIAL&   position_dial() const;
  const BUTTON& freeze_button() const;
};

