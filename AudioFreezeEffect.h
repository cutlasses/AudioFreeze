#pragma once

#include <AudioStream.h>

#define FREEZE_QUEUE_SIZE     185


class AUDIO_FREEZE_EFFECT : public AudioStream
{
  audio_block_t*        m_buffer[FREEZE_QUEUE_SIZE];
  audio_block_t*        m_input_queue_array[1];
  
  int                   m_head;

  int                   m_loop_start;
  int                   m_loop_end;

  bool                  m_freeze_active;
  
public:

  AUDIO_FREEZE_EFFECT();

  virtual void update();

  bool                  is_freeze_active() const;
  void                  set_freeze( bool active );
  void                  set_length( float length );
  void                  set_centre( float centre );
};

