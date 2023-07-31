

#ifndef _TYPES_H
#define _TYPES_H


typedef unsigned char	u8;
typedef signed char		s8;
typedef unsigned short	u16;
typedef signed short	s16;
typedef	unsigned long	u32;
typedef	signed long		s32;


#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#endif

#endif
