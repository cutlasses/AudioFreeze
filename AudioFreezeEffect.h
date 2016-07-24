#pragma once

#include <AudioStream.h>

#define FREEZE_QUEUE_SIZE_IN_BYTES     1024*50      // 50k


class AUDIO_FREEZE_EFFECT : public AudioStream
{
  byte                  m_buffer[FREEZE_QUEUE_SIZE_IN_BYTES];
  audio_block_t*        m_input_queue_array[1];
  
  float                 m_head;     // read head when audio is frozen, write head when not frozen

  float                 m_speed;

  int                   m_loop_start;   // index of the first sample to play in loop
  int                   m_loop_end;     // index of the last sample to play in loop

  int                   m_sample_size_in_bits;
  int                   m_buffer_size_in_samples;

  bool                  m_freeze_active;
  bool                  m_reverse;
  
  bool                  m_one_shot;     // play the section once rather than loop
  bool                  m_paused;

  // store 'next' values, otherwise interrupt could be called during calculation of values
  float                 m_next_sample_size_in_bits;
  float                 m_next_length;
  float                 m_next_centre;
  float                 m_next_speed;
  


  int                   wrap_index_to_loop_section( int index ) const;

  void                  write_sample( int16_t sample, int index );
  int16_t               read_sample( int index ) const;
  
  void                  write_to_buffer( const int16_t* source, int size );
  void                  read_from_buffer( int16_t* dest, int size );
  void                  read_from_buffer_with_speed( int16_t* dest, int size );
  
  void                  advance_head( float inc );

  void                  set_bit_depth_impl( int sample_size_in_bits );
  void                  set_length_impl( float length );
  void                  set_centre_impl( float centre );
  void                  set_speed_impl( float speed );

  
public:

  AUDIO_FREEZE_EFFECT();

  virtual void          update();

  bool                  is_freeze_active() const;
  
  void                  set_freeze( bool active );
  void                  set_length( float length );
  void                  set_centre( float centre );
  void                  set_speed( float speed );
  void                  set_reverse( bool reverse );
  void                  set_bit_depth( int sample_size_in_bits );
};

