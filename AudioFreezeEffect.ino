#include <Audio.h>
#include <Math.h>
#include "AudioFreezeEffect.h"
#include "CompileSwitches.h"



/////////////////////////////////////////////////////////////////////

int freeze_queue_size_in_samples( int sample_size_in_bits )
{
  const int bytes_per_sample = sample_size_in_bits / 8;
  return FREEZE_QUEUE_SIZE_IN_BYTES / bytes_per_sample;
}

/////////////////////////////////////////////////////////////////////

AUDIO_FREEZE_EFFECT::AUDIO_FREEZE_EFFECT() :
  AudioStream( 1, m_input_queue_array ),
  m_buffer(),
  m_input_queue_array(),
  m_head(0),
  m_loop_start(0),
  m_loop_end(freeze_queue_size_in_samples(16) - 1),
  m_sample_size_in_bits(16),
  m_freeze_active(false)
{
  memset( m_buffer, 0, sizeof(m_buffer) );
}

void AUDIO_FREEZE_EFFECT::write_to_buffer( const int16_t* source, int size )
{
  const int fqs     = freeze_queue_size_in_samples( m_sample_size_in_bits );

  for( int x = 0; x < size; ++x )
  {
    // DO CONVERSION TO 8-bit HERE
    int16_t* sample_buffer    = (int16_t*)m_buffer;
    sample_buffer[m_head]     = source[x];

    if( ++m_head == fqs )
    {
      m_head                = 0;
    }
  }
}

void AUDIO_FREEZE_EFFECT::read_from_buffer( int16_t* dest, int size )
{
    for( int x = 0; x < size; ++x )
    {
      int16_t* sample_buffer    = (int16_t*)m_buffer;
      dest[x]                   = sample_buffer[m_head];
      
      // head will have limited movement in freeze mode
      if( ++m_head >= m_loop_end )
      {
        m_head = m_loop_start;
      } 
    }
}

void AUDIO_FREEZE_EFFECT::update()
{
  if( m_freeze_active )
  {
    audio_block_t* block        = allocate();

    if( block != nullptr )
    {
      read_from_buffer( block->data, AUDIO_BLOCK_SAMPLES );
  
      transmit( block, 0 );
    
      release( block );    
    }
  }
  else
  {
    audio_block_t* block        = receiveReadOnly();

    if( block != nullptr )
    {
      write_to_buffer( block->data, AUDIO_BLOCK_SAMPLES );
  
      transmit( block, 0 );
  
      release( block );
    }
  }
  
  /*
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
  */
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
  int loop_length = roundf( length * freeze_queue_size_in_samples( m_sample_size_in_bits ) );
  m_loop_end      = m_loop_start + loop_length;
}

void AUDIO_FREEZE_EFFECT::set_centre( float centre )
{
  const int fqs     = freeze_queue_size_in_samples( m_sample_size_in_bits );
  int centre_index  = roundf( centre * fqs );

  int loop_length   = m_loop_end - m_loop_start;
  m_loop_start      = centre_index - loop_length;

  if( m_loop_start < 0 )
  {
    m_loop_start    = 0;
    m_loop_end      = loop_length - 1;
  }
  else if( m_loop_end > fqs - 1 )
  {
    m_loop_end      = fqs - 1;
    m_loop_start    = m_loop_end - loop_length;
  }
}


