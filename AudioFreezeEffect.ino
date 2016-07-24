#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include <Math.h>
#include "AudioFreezeEffect.h"
#include "CompileSwitches.h"

const float MIN_SPEED( 0.25f );
const float MAX_SPEED( 4.0f );


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
  m_speed(1.0f),
  m_loop_start(0),
  m_loop_end(freeze_queue_size_in_samples(16) - 1),
  m_sample_size_in_bits(16),
  m_buffer_size_in_samples( freeze_queue_size_in_samples( 16 ) ),
  m_freeze_active(false),
  m_reverse(false),
  m_one_shot( false ),
  m_paused( false ),
  m_next_sample_size_in_bits(16),
  m_next_length(1.0f),
  m_next_centre(0.5f),
  m_next_speed(1.0f)
{
  memset( m_buffer, 0, sizeof(m_buffer) );
}

int AUDIO_FREEZE_EFFECT::wrap_index_to_loop_section( int index ) const
{
  if( index > m_loop_end )
  {
    return m_loop_start + ( index - m_loop_end ) - 1;
  }
  else if( index < m_loop_start )
  {
    return m_loop_end - ( m_loop_start - index ) - 1;
  }
  else
  {
    return index;
  }
}

void AUDIO_FREEZE_EFFECT::write_sample( int16_t sample, int index )
{
  ASSERT_MSG( index >= 0 && index < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::write_sample() writing outside buffer" );

  switch( m_sample_size_in_bits )
  {
    case 8:
    {
      int8_t sample8                      = (sample >> 8) & 0xff;
      int8_t* sample_buffer               = reinterpret_cast<int8_t*>(m_buffer);
      sample_buffer[ index ]              = sample8;
      break;
    }
    case 16:
    {
      int16_t* sample_buffer             = reinterpret_cast<int16_t*>(m_buffer);
      sample_buffer[ index ]             = sample;
      break;
    }
  }
}

int16_t AUDIO_FREEZE_EFFECT::read_sample( int index ) const
{
  ASSERT_MSG( index >= 0 && index < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::read_sample() writing outside buffer" );
 
  switch( m_sample_size_in_bits )
  {
    case 8:
    {
        const int8_t* sample_buffer    = reinterpret_cast<const int8_t*>(m_buffer);
        const int8_t sample            = sample_buffer[ index ];

        int16_t sample16               = sample;
        sample16                       <<= 8;

        return sample16;
    }
    case 16:
    {
        const int16_t* sample_buffer    = reinterpret_cast<const int16_t*>(m_buffer);
        const int16_t sample            = sample_buffer[ index ];
        return sample;
    }
  }

  return 0;
}

void AUDIO_FREEZE_EFFECT::write_to_buffer( const int16_t* source, int size )
{
  ASSERT_MSG( trunc_to_int(m_head) >= 0 && trunc_to_int(m_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::write_to_buffer()" );
  
  for( int x = 0; x < size; ++x )
  {
    write_sample( source[x], trunc_to_int(m_head) );

    if( trunc_to_int(++m_head) == m_buffer_size_in_samples )
    {
      m_head                        = 0.0f;
    }
  }
}

void AUDIO_FREEZE_EFFECT::read_from_buffer( int16_t* dest, int size )
{
   ASSERT_MSG( trunc_to_int(m_head) >= 0 && trunc_to_int(m_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::read_from_buffer()" );
 
  for( int x = 0; x < size; ++x )
  {
    dest[x]                   = read_sample( trunc_to_int(m_head) );
    
    // head will have limited movement in freeze mode
    if( ++m_head >= m_loop_end )
    {
      m_head                  = m_loop_start;
    } 
  }
}

void AUDIO_FREEZE_EFFECT::read_from_buffer_with_speed( int16_t* dest, int size )
{          
    ASSERT_MSG( trunc_to_int(m_head) >= 0 && trunc_to_int(m_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::read_from_buffer_with_speed()" );

    for( int x = 0; x < size; ++x )
    {
      if( m_speed < 1.0f )
      {
        float prev_head         = m_head;
        int curr_index          = truncf( m_head );
        advance_head( m_speed );
        int next_index          = truncf( m_head );

        ASSERT_MSG( curr_index >= 0 && curr_index < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::read_from_buffer_with_speed()" );
        ASSERT_MSG( next_index >= 0 && next_index < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::read_from_buffer_with_speed()" );

        int16_t sample( 0 );

        if( curr_index == next_index )
        {
          // both current and next are in the same sample
          sample                = read_sample(curr_index);
        }
        else
        {
          // crossing 2 samples - calculate how much of each sample to use, then lerp between them
          // use the fractional part - if 0.3 'into' next sample, then we mix 0.3 of next and 0.7 of current
          double int_part;
          float rem             = m_reverse ? modf( prev_head, &int_part ) : modf( m_head, &int_part );
          const float t         = rem / m_speed;      
          sample                = lerp( read_sample(curr_index), read_sample(next_index), t );          
       }

        dest[x]                 = sample;
      }
      else
      {
        dest[x]                 = read_sample( trunc_to_int(m_head) );
        
        advance_head( m_speed );
      }
    }
}

void AUDIO_FREEZE_EFFECT::advance_head( float inc )
{
  // advance the read head - clamped between start and end
  // will read backwards when in reverse mode

  bool end_reached    = false;

  if( m_loop_start == m_loop_end )
  {
    // play head cannot be incremented
#ifdef DEBUG_OUTPUT
    Serial.print("loop length is 1");
#endif
    m_head      = m_loop_start;   
    end_reached = true;
  }
  else
  {
    ASSERT_MSG( truncf(m_head) >= 0 && truncf(m_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::next_head()" );
  
    inc = min( inc, m_loop_end - m_loop_start - 1 ); // clamp the increment to the loop length
    //ASSERT_MSG( inc > 0 && inc < m_loop_end - m_loop_start, "Invalid inc AUDIO_FREEZE_EFFECT::next_head()" );
    
    if( m_reverse )
    {
      m_head               -= inc;
      if( m_head < m_loop_start )
      {
        const float rem       = m_loop_start - m_head - 1.0f;
        m_head                = m_loop_end - rem;
      }
  
      ASSERT_MSG( truncf(m_head) >= 0 && truncf(m_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::next_head()" );
    }
    else
    {
      m_head                += inc;
      if( m_head > m_loop_end )
      {
        const float rem     = m_head - m_loop_end - 1.0f;
        m_head              = m_loop_start + rem;
        end_reached         = true;
      }
  
      ASSERT_MSG( truncf(m_head) >= 0 && truncf(m_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::next_head()" );
  
#ifdef DEBUG_OUTPUT
      if( truncf(m_head) < 0 || truncf(m_head) >= m_buffer_size_in_samples )
      {
        Serial.print("next_head ");
        Serial.print(m_head);
        Serial.print(" rem  ");
        Serial.print(m_head - m_loop_end - 1.0f);
        Serial.print( " loop start ");
        Serial.print(m_loop_start);
        Serial.print( " loop end ");
        Serial.print(m_loop_end);
        Serial.print( " inc ");
        Serial.print(inc);
        Serial.print( "\n");          
      }
#endif
    }
  }

  if( end_reached && m_one_shot )
  {
    m_paused = true;
  }
}

void AUDIO_FREEZE_EFFECT::update()
{      
  set_bit_depth_impl( m_next_sample_size_in_bits);
  set_length_impl( m_next_length );
  set_centre_impl( m_next_centre );
  set_speed_impl( m_next_speed );

  ASSERT_MSG( trunc_to_int(m_head) >= 0 && trunc_to_int(m_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::update()" );
  ASSERT_MSG( m_loop_start >= 0 && m_loop_start < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::update()" );
  ASSERT_MSG( m_loop_end >= 0 && m_loop_end < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::update()" );
  
  if( m_freeze_active )
  {
    if( !m_paused )
    {    
      audio_block_t* block        = allocate();
  
      if( block != nullptr )
      {
        //read_from_buffer( block->data, AUDIO_BLOCK_SAMPLES );
        read_from_buffer_with_speed( block->data, AUDIO_BLOCK_SAMPLES );
    
        transmit( block, 0 );
      
        release( block );    
      }
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
}

void AUDIO_FREEZE_EFFECT::set_bit_depth_impl( int sample_size_in_bits )
{
  if( sample_size_in_bits != m_sample_size_in_bits )
  {
      m_sample_size_in_bits       = sample_size_in_bits;
      m_buffer_size_in_samples    = freeze_queue_size_in_samples( m_sample_size_in_bits );
      
      m_head                      = 0.0f;
      m_loop_start                = 0;
      m_loop_end                  = 0;
    
      memset( m_buffer, 0, sizeof(m_buffer) );

#ifdef DEBUG_OUTPUT
      Serial.print("Set bit depth:");
      Serial.print( m_sample_size_in_bits );
      Serial.print("\n");
#endif
  }
}

void AUDIO_FREEZE_EFFECT::set_length_impl( float length )
{  
  ASSERT_MSG( m_loop_start >= 0 && m_loop_start < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::set_length_impl() pre" );
  ASSERT_MSG( m_loop_end >= 0 && m_loop_end < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::set_length_impl() pre" ); 
  ASSERT_MSG( length >= 0 && length <= 1.0f, "AUDIO_FREEZE_EFFECT::set_length_impl()" );

  const int loop_length = roundf( length * m_buffer_size_in_samples );
  m_loop_end            = min( m_loop_start + loop_length, m_buffer_size_in_samples - 1 );

#ifdef DEBUG_OUTPUT
  if( m_loop_end < m_loop_start || m_loop_start < 0 || m_loop_start > m_buffer_size_in_samples - 1 || m_loop_end < 0 || m_loop_end > m_buffer_size_in_samples - 1 )
  {
    Serial.print( "set length** loop_start:" );
    Serial.print( m_loop_start );
    Serial.print( " loop_end:" );
    Serial.print( m_loop_end );
    Serial.print( " loop_length:" );
    Serial.print( loop_length );
    Serial.print( " length:" );
    Serial.print( length );
    Serial.print( " buffer size:");
    Serial.print( m_buffer_size_in_samples );
    Serial.print( "\n" );
  }
#endif

  ASSERT_MSG( m_loop_end >= m_loop_start, "AUDIO_FREEZE_EFFECT::set_length_impl()" );
  ASSERT_MSG( m_loop_start >= 0 && m_loop_start < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::set_length_impl() post" );
  ASSERT_MSG( m_loop_end >= 0 && m_loop_end < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::set_length_impl() post" );
}

void AUDIO_FREEZE_EFFECT::set_speed_impl( float speed )
{
  if( speed < 0.5f )
  {
    // put in the range 0..1 
    float r = speed * 2.0f;
    m_speed = lerp( MIN_SPEED, 1.0f, r ); 
  }
  else
  {
    // put in the range 0..1
    float r = ( speed - 0.5f ) * 2.0f;
    m_speed = lerp( 1.0f, MAX_SPEED, r );    
  }
}

void AUDIO_FREEZE_EFFECT::set_centre_impl( float centre )
{
  ASSERT_MSG( centre >= 0 && centre < 1.0f, "AUDIO_FREEZE_EFFECT::set_centre_impl()" );
  
  int centre_index      = roundf( centre * m_buffer_size_in_samples );

  int loop_length       = min( m_loop_end - m_loop_start + 1, m_buffer_size_in_samples - 1 );
  ASSERT_MSG( loop_length < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::set_centre() loop too long" );
  int half_loop_length  = loop_length / 2;
  m_loop_start          = centre_index - half_loop_length;
  m_loop_end            = m_loop_start + loop_length;

  if( m_loop_start < 0 )
  {
    m_loop_start      = 0;
    m_loop_end        = loop_length - 1;
  }
  else if( m_loop_end > m_buffer_size_in_samples - 1 )
  {
    m_loop_end        = m_buffer_size_in_samples - 1;
    m_loop_start      = m_loop_end - loop_length - 1;
  }

  // not sure why these clamps are required, the code above should ensure they are correct, but it asserts without it - investigate
  m_loop_start        = max( m_loop_start, 0 );
  m_loop_end          = min( m_loop_end, m_buffer_size_in_samples - 1 );

#ifdef DEBUG_OUTPUT
  if( m_loop_end < m_loop_start || m_loop_start < 0 || m_loop_start > m_buffer_size_in_samples - 1 || m_loop_end < 0 || m_loop_end > m_buffer_size_in_samples - 1 )
  {
    Serial.print( "set set_centre_impl*** loop_start:" );
    Serial.print( m_loop_start );
    Serial.print( " loop_end:" );
    Serial.print( m_loop_end );
    Serial.print( " loop_length:" );
    Serial.print( loop_length );
    Serial.print( " buffer size:");
    Serial.print( m_buffer_size_in_samples );
    Serial.print( " centre:" );
    Serial.print( centre_index );
    Serial.print( "\n" );
  }
#endif

  ASSERT_MSG( m_loop_end >= m_loop_start, "AUDIO_FREEZE_EFFECT::set_centre()" );
  ASSERT_MSG( m_loop_start >= 0 && m_loop_start < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::set_centre()" );
  ASSERT_MSG( m_loop_end >= 0 && m_loop_end < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::set_centre()" );
}

bool AUDIO_FREEZE_EFFECT::is_freeze_active() const
{
  return m_freeze_active; 
}

void AUDIO_FREEZE_EFFECT::set_freeze( bool active )
{
  m_freeze_active = active;  
}

void AUDIO_FREEZE_EFFECT::set_reverse( bool reverse )
{
  m_reverse = reverse;
}

void AUDIO_FREEZE_EFFECT::set_length( float length )
{
  m_next_length = length;
  //set_length_impl(length);
}

void AUDIO_FREEZE_EFFECT::set_centre( float centre )
{
  m_next_centre = centre;
  //set_centre_impl(centre);
}

void AUDIO_FREEZE_EFFECT::set_speed( float speed )
{
  m_next_speed = speed;
  //set_speed_impl(speed);
}

void AUDIO_FREEZE_EFFECT::set_bit_depth( int sample_size_in_bits )
{
  m_next_sample_size_in_bits = sample_size_in_bits;
  //set_bit_depth_impl( sample_size_in_bits );
}


