#pragma once

#include "CompileSwitches.h"

#ifdef DEBUG_OUTPUT

bool _assert_fail(const char* msg )
{
  Serial.print(msg);
  Serial.print("\n");

  return true;
}

#define ASSERT_MSG(x, msg) ((void)((x) || (_assert_fail(msg))))
#else
#define ASSERT_MSG(x, msg)
#endif

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

template <typename T>
T lerp( const T& v1, const T& v2, float t )
{
  return v1 + ( (v2 - v1) * t );
}

int trunc_to_int( float v )
{
  return static_cast<int>( trunc(v) );
}


