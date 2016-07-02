#pragma once

#include <AudioStream.h>

#define FREEZE_QUEUE_SIZE_IN_BYTES     1024*40      // 40k


class AUDIO_FREEZE_EFFECT : public AudioStream
{
  byte                  m_buffer[FREEZE_QUEUE_SIZE_IN_BYTES];
  audio_block_t*        m_input_queue_array[1];
  
  float                 m_head;     // read head when audio is frozen, write head when not frozen

  float                 m_speed;

  int                   m_loop_start;
  int                   m_loop_end;

  int                   m_sample_size_in_bits;

  bool                  m_freeze_active;
  bool                  m_reverse;

  int                   wrap_index_to_loop_section( int index ) const;

  int16_t               read_sample( int index ) const;
  void                  write_to_buffer( const int16_t* source, int size );
  void                  read_from_buffer( int16_t* dest, int size );
  void                  read_from_buffer_with_speed( int16_t* dest, int size );
  
  float                 next_head( float inc ) const;
  
public:

  AUDIO_FREEZE_EFFECT();

  virtual void update();

  bool                  is_freeze_active() const;
  void                  set_freeze( bool active );
  void                  set_length( float length );
  void                  set_centre( float centre );
  void                  set_speed( float speed );
  void                  set_reverse( bool reverse );
};

