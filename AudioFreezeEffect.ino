#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include <math.h>
#include "AudioFreezeEffect.h"
#include "CompileSwitches.h"
#include "Util.h"

const float MIN_SPEED( 0.25f );
const float MAX_SPEED( 4.0f );

constexpr float MIN_WOW_FREQ( 1.0f / 4.0f );
constexpr float MAX_WOW_FREQ( 1.0f / 0.25f );
constexpr float MIN_FLUTTER_FREQ( 1.0f / 0.03f );
constexpr float MAX_FLUTTER_FREQ( 1.0f / 0.02f );


constexpr int CROSS_FADE_SAMPLES( round_to_int(( AUDIO_SAMPLE_RATE / 1000.0f ) * 5) );


/////////////////////////////////////////////////////////////////////

int freeze_queue_size_in_samples( int sample_size_in_bits )
{
  const int bytes_per_sample = sample_size_in_bits / 8;
  return FREEZE_QUEUE_SIZE_IN_BYTES / bytes_per_sample;
}

/////////////////////////////////////////////////////////////////////

RANDOM_LFO::RANDOM_LFO( float min_frequency, float max_frequency ) :
  m_min_frequency( min_frequency ),
  m_max_frequency( max_frequency ),
  m_p_ratio( 0.0f ),
  m_time( 0.0f ),
  m_prev_value( 0.0f ),
  m_num_cycles( 0 )
{
  choose_next_frequency();
}

void RANDOM_LFO::set_period( float seconds )
{
  m_p_ratio = ( 2.0f * M_PI ) / seconds;
}

void RANDOM_LFO::set_frequency( float hz )
{
  m_p_ratio = ( 2.0f * M_PI ) * hz;
}

void RANDOM_LFO::choose_next_frequency()
{
  set_frequency( random_ranged( m_min_frequency, m_max_frequency ) );
}

void RANDOM_LFO::set_frequency_range( float min_frequency, float max_frequency )
{
  m_min_frequency = min_frequency;
  m_max_frequency = max_frequency;
}

float RANDOM_LFO::next( float time_inc )
{
  const float next_value = sin( m_time * m_p_ratio );
  
  if( (m_prev_value >= 0.0f) != (next_value >= 0.0f) )
  {
    ++m_num_cycles;
  }
  
  if( m_num_cycles >= 2 )
  {
    m_time      = 0.0f;
    m_num_cycles  = 0;
    choose_next_frequency();
  }
  
  m_prev_value = next_value;
    
  m_time      += time_inc;
  
  return next_value;
}

/////////////////////////////////////////////////////////////////////

AUDIO_FREEZE_EFFECT::AUDIO_FREEZE_EFFECT() :
  AudioStream( 1, m_input_queue_array ),
  m_buffer(),
  m_head(0),
  m_speed(0.5f),
  m_loop_start(0),
  m_loop_end(freeze_queue_size_in_samples(16) - 1),
  m_sample_size_in_bits(16),
  m_buffer_size_in_samples( freeze_queue_size_in_samples( 16 ) ),
  m_freeze_active(false),
  m_reverse(false),
  m_cross_fade(true),
  m_next_sample_size_in_bits(16),
  m_next_length(1.0f),
  m_next_centre(0.5f),
  m_next_speed(0.5f),
  m_next_freeze_active(false),
  m_wow_lfo( MIN_WOW_FREQ, MAX_WOW_FREQ ),
  m_flutter_lfo( MIN_FLUTTER_FREQ, MAX_FLUTTER_FREQ ),
  m_wow_amount( 0.0f ),
  m_flutter_amount( 0.0f )
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
	  
	const int diff  = m_head == 0 ? abs( read_sample( m_buffer_size_in_samples - 1 ) - read_sample( trunc_to_int(m_head) ) ) : abs( read_sample( trunc_to_int(m_head) - 1 ) - read_sample( trunc_to_int(m_head) ) );
	if( trunc_to_int(m_head) > 0 && diff > 2000 )
	{
		printf( "WHOOPS" );
	}

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
      m_head                  = static_cast<float>(m_loop_start);
    } 
  }
}

int16_t AUDIO_FREEZE_EFFECT::read_sample_linear( float current ) const
{
	// linearly interpolate between the current sample and its neighbour
	// (previous neighbour if frac is less than 0.5, otherwise next)
	const int int_part			= trunc_to_int( current );
	const float frac_part		= current - int_part;
	
	const int16_t curr_samp		= read_sample( int_part );
	
	if( frac_part < 0.5f )
	{
		int prev				= int_part - 1;
		if( prev < 0 )
		{
			// at the beginning of the buffer, assume next sample was the same and use that (e.g. no interpolation)
			return curr_samp;
		}
		
		const float t			= frac_part / 0.5f;
		
		const int16_t prev_samp	= read_sample( prev );
		
		return lerp( prev_samp, curr_samp, t );
	}
	else
	{
		int next				= int_part + 1;
		if( next >= m_buffer_size_in_samples )
		{
			// at the end of the buffer, assume next sample was the same and use that (e.g. no interpolation)
			return curr_samp;
		}
		
		const float t			= ( frac_part - 0.5f ) / 0.5f;
		
		const int16_t next_samp	= read_sample( next );
		
		return lerp( curr_samp, next_samp, t );
	}
}

int16_t AUDIO_FREEZE_EFFECT::read_sample_cubic( float current ) const
{
	const int int_part			= trunc_to_int( current );
	const float frac_part		= current - int_part;
	
	float p0;
	if( int_part >= 2 )
	{
		p0						= read_sample( int_part - 2 );
	}
	else
	{
		// at the beginning of the buffer, assume previous sample was the same
		p0						= read_sample( 0 );
	}
	
	float p1;
	if( int_part <= 2 )
	{
		// reuse p0
		p1						= p0;
	}
	else
	{
		p1						= read_sample( int_part - 1 );
	}
	
	float p2					= read_sample( int_part );
	
	float p3;
	if( int_part < m_sample_size_in_bits - 1)
	{
		p3						= read_sample( int_part + 1 );
	}
	else
	{
		p3						= p2;
	}
	
	const float	t				= lerp( 0.33333f, 0.66666f, frac_part );
	
	float sampf					= cubic_interpolation( p0, p1, p2, p3, t );
	return round_to_int( sampf );
}

void AUDIO_FREEZE_EFFECT::read_from_buffer_with_speed( int16_t* dest, int size )
{          
    ASSERT_MSG( trunc_to_int(m_head) >= 0 && trunc_to_int(m_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::read_from_buffer_with_speed()" );

    for( int x = 0; x < size; ++x )
    {
		dest[x]                 = read_sample_linear( m_head );

		m_head                  = next_head( m_speed );
    }
}

void AUDIO_FREEZE_EFFECT::read_from_buffer_with_speed_and_cross_fade( int16_t* dest, int size )
{
  // NOTE - does not currently support reverse
  ASSERT_MSG( trunc_to_int(m_head) >= 0 && trunc_to_int(m_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::read_from_buffer_with_speed()" );
  
  const int cross_fade_start  = m_loop_end - CROSS_FADE_SAMPLES;
  const int cf_offset     	  = cross_fade_start - m_loop_start; // cross fade start before the loop starts and fades in

/*
  DEBUG_TEXT( "read_from_buffer_with_speed_and_cross_fade() " );
  DEBUG_TEXT( m_head );
  DEBUG_TEXT( " " );
  DEBUG_TEXT( m_loop_start );
  DEBUG_TEXT( " " );
  DEBUG_TEXT( m_loop_end );
  DEBUG_TEXT( " " );
  DEBUG_TEXT( cross_fade_start );
  DEBUG_TEXT( " " );
  DEBUG_TEXT( "\n" );
 */
  
  for( int x = 0; x < size; ++x )
  {
    const int headi			      = trunc_to_int( m_head );
    const int16_t sample      = read_sample_cubic( m_head );
    
    if( headi >= cross_fade_start )
    {
      const float cf_head     = m_head - cf_offset;
      const int16_t cf_sample = read_sample_cubic( cf_head );
  
      const float cf_t	= ( m_head - cross_fade_start ) / static_cast<float>(CROSS_FADE_SAMPLES);
      ASSERT_MSG( cf_t >= -0.1f && cf_t <= 1.1f, "Invalid t" );
  
      dest[x]          	      = lerp( sample, cf_sample, cf_t );
    }
    else
    {
      dest[x]                 = sample;
    }

    m_head					          += m_speed;

    if( m_head >= m_loop_end )
    {
      m_head				          -= cf_offset;
    }
  }  
}

float AUDIO_FREEZE_EFFECT::next_head( float inc ) const
{
  // advance the read head - clamped between start and end
  // will read backwards when in reverse mode

  if( m_loop_start == m_loop_end )
  {
    // play head cannot be incremented
#ifdef DEBUG_OUTPUT
    DEBUG_TEXT("loop length is 1");
#endif
    return static_cast<float>(m_loop_start);
  }

  ASSERT_MSG( truncf(m_head) >= 0 && truncf(m_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::next_head()" );

  inc = min_val<float>( inc, m_loop_end - m_loop_start - 1 ); // clamp the increment to the loop length
  //ASSERT_MSG( inc > 0 && inc < m_loop_end - m_loop_start, "Invalid inc AUDIO_FREEZE_EFFECT::next_head()" );
  
  if( m_reverse )
  {
    float next_head( m_head );
    next_head               -= inc;
    if( next_head < m_loop_start )
    {
      const float rem       = m_loop_start - next_head - 1.0f;
      next_head             = m_loop_end - rem;
    }

    ASSERT_MSG( truncf(next_head) >= 0 && truncf(next_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::next_head()" );
    return next_head;    
  }
  else
  {
    float next_head( m_head );
    next_head               += inc;
    if( next_head > m_loop_end )
    {
      const float rem       = next_head - m_loop_end - 1.0f;
      next_head             = m_loop_start + rem;
    }

    ASSERT_MSG( truncf(next_head) >= 0 && truncf(next_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::next_head()" );

#ifdef DEBUG_OUTPUT
    if( truncf(next_head) < 0 || truncf(next_head) >= m_buffer_size_in_samples )
    {
      DEBUG_TEXT("next_head ");
      DEBUG_TEXT(next_head);
      DEBUG_TEXT(" rem  ");
      DEBUG_TEXT(next_head - m_loop_end - 1.0f);
      DEBUG_TEXT( " loop start ");
      DEBUG_TEXT(m_loop_start);
      DEBUG_TEXT( " loop end ");
      DEBUG_TEXT(m_loop_end);
      DEBUG_TEXT( " inc ");
      DEBUG_TEXT(inc);
      DEBUG_TEXT( "\n");
    }
#endif
    
    return next_head;
  }
}

void AUDIO_FREEZE_EFFECT::update()
{        
  set_bit_depth_impl( m_next_sample_size_in_bits );
  set_length_impl( m_next_length );
  set_centre_impl( m_next_centre );
  set_speed_impl( m_next_speed );
  set_freeze_impl( m_next_freeze_active );


  ASSERT_MSG( trunc_to_int(m_head) >= 0 && trunc_to_int(m_head) < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::update()" );
  ASSERT_MSG( m_loop_start >= 0 && m_loop_start < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::update()" );
  ASSERT_MSG( m_loop_end >= 0 && m_loop_end < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::update()" );

  const float old_speed = m_speed;
	
  const float time_inc    = AUDIO_BLOCK_SAMPLES * ( 1.0f / AUDIO_SAMPLE_RATE );
  const float wow_lfo     = m_wow_lfo.next( time_inc );
  const float flutter_lfo = m_flutter_lfo.next( time_inc );
	
  constexpr float MAX_ADJ_WOW( ( 1.0f / 12.0f ) ); // 1 semitone
  constexpr float MAX_ADJ_FLUTTER( ( 1.0f / 12.0f ) ); // 1 semitone
  const float wow	      = ( wow_lfo * m_wow_amount )  * MAX_ADJ_WOW;
  const float flutter   = ( flutter_lfo * m_flutter_amount ) * MAX_ADJ_FLUTTER;
  m_speed				        += wow + flutter;

  DEBUG_TEXT( m_speed );
  DEBUG_TEXT( "\n" );
	
  if( m_freeze_active )
  {        
    audio_block_t* block        = allocate();

    if( block != nullptr )
    {
      if( m_cross_fade )
      {
        read_from_buffer_with_speed_and_cross_fade( block->data, AUDIO_BLOCK_SAMPLES );   
      }
      else
      {
        read_from_buffer_with_speed( block->data, AUDIO_BLOCK_SAMPLES );
      }
  
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
	
  m_speed = old_speed;
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
      DEBUG_TEXT("Set bit depth:");
      DEBUG_TEXT( m_sample_size_in_bits );
      DEBUG_TEXT("\n");
#endif
  }
}

void AUDIO_FREEZE_EFFECT::set_length_impl( float length )
{  
  ASSERT_MSG( m_loop_start >= 0 && m_loop_start < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::set_length_impl() pre" );
  ASSERT_MSG( m_loop_end >= 0 && m_loop_end < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::set_length_impl() pre" ); 
  ASSERT_MSG( length >= 0 && length <= 1.0f, "AUDIO_FREEZE_EFFECT::set_length_impl()" );

  const int loop_length = max_val<int>( CROSS_FADE_SAMPLES*2, round_to_int( length * m_buffer_size_in_samples ) );
  m_loop_end            = min_val<int>( m_loop_start + loop_length, m_buffer_size_in_samples - 1 );

#ifdef DEBUG_OUTPUT
  if( m_loop_end < m_loop_start || m_loop_start < 0 || m_loop_start > m_buffer_size_in_samples - 1 || m_loop_end < 0 || m_loop_end > m_buffer_size_in_samples - 1 )
  {
    DEBUG_TEXT( "set length** loop_start:" );
    DEBUG_TEXT( m_loop_start );
    DEBUG_TEXT( " loop_end:" );
    DEBUG_TEXT( m_loop_end );
    DEBUG_TEXT( " loop_length:" );
    DEBUG_TEXT( loop_length );
    DEBUG_TEXT( " length:" );
    DEBUG_TEXT( length );
    DEBUG_TEXT( " buffer size:");
    DEBUG_TEXT( m_buffer_size_in_samples );
    DEBUG_TEXT( "\n" );
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
  
  int centre_index      = round_to_int( centre * m_buffer_size_in_samples );

  int loop_length       = min_val<int>( m_loop_end - m_loop_start + 1, m_buffer_size_in_samples - 1 );
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
  m_loop_start        = max_val<int>( m_loop_start, 0 );
  m_loop_end          = min_val<int>( m_loop_end, m_buffer_size_in_samples - 1 );

#ifdef DEBUG_OUTPUT
  if( m_loop_end < m_loop_start || m_loop_start < 0 || m_loop_start > m_buffer_size_in_samples - 1 || m_loop_end < 0 || m_loop_end > m_buffer_size_in_samples - 1 )
  {
    DEBUG_TEXT( "set set_centre_impl*** loop_start:" );
    DEBUG_TEXT( m_loop_start );
    DEBUG_TEXT( " loop_end:" );
    DEBUG_TEXT( m_loop_end );
    DEBUG_TEXT( " loop_length:" );
    DEBUG_TEXT( loop_length );
    DEBUG_TEXT( " buffer size:");
    DEBUG_TEXT( m_buffer_size_in_samples );
    DEBUG_TEXT( " centre:" );
    DEBUG_TEXT( centre_index );
   	DEBUG_TEXT( "\n" );
  }
#endif

  ASSERT_MSG( m_loop_end >= m_loop_start, "AUDIO_FREEZE_EFFECT::set_centre()" );
  ASSERT_MSG( m_loop_start >= 0 && m_loop_start < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::set_centre()" );
  ASSERT_MSG( m_loop_end >= 0 && m_loop_end < m_buffer_size_in_samples, "AUDIO_FREEZE_EFFECT::set_centre()" );
}

void AUDIO_FREEZE_EFFECT::set_freeze_impl( bool active )
{
  if( active != m_freeze_active )
  {
    if( active )
    {
      DEBUG_TEXT("FREEZE\n");
    }
    else
    {
      DEBUG_TEXT("ANTI-FREEZE\n");
    }
  }
  
	if( active && active != m_freeze_active )
	{
		if( m_cross_fade )
		{ 
			// cross fade where the new audio (at the head) meets old
			// blend the final new sample into the old section
			int headi = trunc_to_int( m_head ) - 1;
			
			if( headi < 0 )
			{
				headi = m_buffer_size_in_samples - 1;
			}
			
			const int cross_fade_start = headi;
			
			const int16_t new_sample = read_sample( headi );
			
			for( int x = 0; x < CROSS_FADE_SAMPLES; ++x )
			{
				if( ++headi >= m_buffer_size_in_samples )
				{
					headi = 0;
				}
				
				const int16_t old_sample	= read_sample( headi );
				
				float cf_t( 0.0f );
				if( headi >= cross_fade_start )
				{
					cf_t					= ( headi - cross_fade_start ) / static_cast<float>(CROSS_FADE_SAMPLES);
				}
				else
				{
					// head has wrapped around
					cf_t					= ( headi + ( m_buffer_size_in_samples - cross_fade_start ) ) / static_cast<float>(CROSS_FADE_SAMPLES);
				}
				
				const int16_t cf_sample		= static_cast<int16_t>( round_to_int( lerp( new_sample, old_sample, cf_t ) ) );
				
				write_sample( cf_sample, headi );
			}
		}
	}
	
	m_freeze_active = active;
}

bool AUDIO_FREEZE_EFFECT::is_freeze_active() const
{
  return m_freeze_active; 
}

void AUDIO_FREEZE_EFFECT::set_freeze( bool active )
{
  m_next_freeze_active = active;
}

void AUDIO_FREEZE_EFFECT::set_reverse( bool reverse )
{
  m_reverse = reverse;
}

void AUDIO_FREEZE_EFFECT::set_cross_fade( bool cross_fade )
{
  m_cross_fade = cross_fade;
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

void AUDIO_FREEZE_EFFECT::set_wow_frequency_range( float min_frequency, float max_frequency )
{
	m_wow_lfo.set_frequency_range( min_frequency, max_frequency );
}

void AUDIO_FREEZE_EFFECT::set_wow_amount( float amount )
{
	m_wow_amount = amount;
}

void AUDIO_FREEZE_EFFECT::set_flutter_amount( float amount )
{
	m_flutter_amount = amount;
}

void AUDIO_FREEZE_EFFECT::set_flutter_frequency_range( float min_frequency, float max_frequency )
{
	m_flutter_lfo.set_frequency_range( min_frequency, max_frequency );
}

