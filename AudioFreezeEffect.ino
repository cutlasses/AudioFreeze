#include <Audio.h>
#include <Math.h>
#include "AudioFreezeEffect.h"

AUDIO_FREEZE_EFFECT::AUDIO_FREEZE_EFFECT() :
  AudioStream( 1, m_input_queue_array ),
  m_buffer(),
  m_input_queue_array(),
  m_head(0),
  m_loop_start(0),
  m_loop_end(FREEZE_QUEUE_SIZE),
  m_freeze_active(false)
{
  memset( m_buffer, 0, sizeof(m_buffer) );
}

void AUDIO_FREEZE_EFFECT::update()
{
  if( m_freeze_active )
  {
    // head will have limited movement in freeze mode
    if( ++m_head >= m_loop_end )
    {
      m_head = m_loop_start;
    }

    audio_block_t* block = m_buffer[ m_head ];
    if( block != nullptr )
    {
      transmit( block, 0  );
    }
  }
  else
  {
    audio_block_t* block        = receiveReadOnly();

    if( block != nullptr )
    {    
      m_buffer[ m_head ]        = block;
  
      transmit( block, 0 );
  
      // advance the head
      if( ++m_head == FREEZE_QUEUE_SIZE )
      {
        m_head                  = 0;
      }
  
      if( m_buffer[ m_head ] != nullptr )
      {
        release( m_buffer[ m_head ] );
      }
    }
  }
}

bool AUDIO_FREEZE_EFFECT::is_freeze_active() const
{
  return m_freeze_active; 
}

void AUDIO_FREEZE_EFFECT::set_freeze( bool active )
{
  m_freeze_active = active;  
}

void AUDIO_FREEZE_EFFECT::set_length( float length )
{
  int loop_length = roundf( length * FREEZE_QUEUE_SIZE );
  m_loop_end    = m_loop_start + loop_length;
}

void AUDIO_FREEZE_EFFECT::set_centre( float centre )
{
  int centre_index  = roundf( centre * FREEZE_QUEUE_SIZE );

  int loop_length   = m_loop_end - m_loop_start;
  m_loop_start      = centre_index - loop_length;

  if( m_loop_start < 0 )
  {
    m_loop_start    = 0;
    m_loop_end      = loop_length - 1;
  }
  else if( m_loop_end > FREEZE_QUEUE_SIZE - 1 )
  {
    m_loop_end      = FREEZE_QUEUE_SIZE - 1;
    m_loop_start    = m_loop_end - loop_length;
  }
}


