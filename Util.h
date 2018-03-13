#pragma once

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>     // Arduino compiler can get confused if you don't include include all required headers in this file?!?

#include "CompileSwitches.h"

#ifdef DEBUG_OUTPUT

extern bool serial_port_initialised;

inline bool _assert_fail( const char* assert, const char* msg )
{
  if( serial_port_initialised )
  {
    Serial.print(assert);
    Serial.print(" ");
    Serial.print(msg);
    Serial.print("\n");
  }
  
  return true;
}

#define ASSERT_MSG(x, msg) ((void)((x) || (_assert_fail(#x,msg))))
#define DEBUG_TEXT(x) if(serial_port_initialised) {Serial.print(x);}

#else // !DEBUG_OUPUT

#define ASSERT_MSG(x, msg)
#define DEBUG_TEXT(x)

#endif // !DEBUG_OUTPUT

/////////////////////////////////////////////////////

template <typename T>
T clamp( const T& value, const T& min, const T& max )
{
  if( value < min )
  {
    return min;
  }
  if( value > max )
  {
    return max;
  }
  return value;
}

template <typename T>
T max_val( const T& v1, const T& v2 )
{
  if( v1 > v2 )
  {
    return v1;
  }
  else
  {
    return v2;
  }
}

template <typename T>
T min_val( const T& v1, const T& v2 )
{
  if( v1 < v2 )
  {
    return v1;
  }
  else
  {
    return v2;
  }
}

/////////////////////////////////////////////////////

template <typename T>
T lerp( const T& v1, const T& v2, float t )
{
  return v1 + ( (v2 - v1) * t );
}

// from http://polymathprogrammer.com/2008/09/29/linear-and-cubic-interpolation/
inline float cubic_interpolation( float p0, float p1, float p2, float p3, float t )
{
  const float one_minus_t = 1.0f - t;
  return ( one_minus_t * one_minus_t * one_minus_t * p0 ) + ( 3 * one_minus_t * one_minus_t * t * p1 ) + ( 3 * one_minus_t * t * t * p2 ) + ( t * t * t * p3 );
}

/////////////////////////////////////////////////////

inline constexpr int trunc_to_int( float v )
{
  return static_cast<int>( v );
}

inline constexpr int round_to_int( float v )
{
	return static_cast<int>( v + 0.5f );
}

/////////////////////////////////////////////////////

template < typename TYPE, int CAPACITY >
class RUNNING_AVERAGE
{
  TYPE                    m_values[ CAPACITY ];
  int                     m_current;
  int                     m_size;

public:

  RUNNING_AVERAGE() :
    m_values(),
    m_current(0),
    m_size(0)
  {
  }

  void add( TYPE value )
  {
    m_values[ m_current ] = value;
    m_current             = ( m_current + 1 ) % CAPACITY;
    ++m_size;
    if( m_size > CAPACITY )
    {
      m_size              = CAPACITY;
    }
  }

  void reset()
  {
    m_size                = 0;
    m_current             = 0;
  }
  
  TYPE average() const
  {
    if( m_size == 0 )
    {
      return 0;  
    }
    
    TYPE avg = 0;
    for( int x = 0; x < m_size; ++x )
    {
      avg += m_values[ x ];
    }

    return avg / m_size;
  }

  int size() const
  {
    return m_size;
  }
};

inline float random_ranged( float min, float max )
{
	const float range = max - min;
	return ( ( static_cast<float>( rand() ) / RAND_MAX ) * range ) + min;
}

/////////////////////////////////////////////////////
