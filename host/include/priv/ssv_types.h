/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/


#ifndef _SSV_TYPES_H_
#define _SSV_TYPES_H_

#ifdef __linux__
#include <linux/types.h>
#endif


#ifndef __SS8_T_
#define __SS8_T_

#if 0 //ndef s8
typedef char             s8;
#endif

#ifndef u8
typedef unsigned char               u8;
#endif

#ifndef s16
typedef short                       s16;
#endif

#ifndef u16
typedef unsigned short              u16;
#endif
#ifndef s32
typedef int                         s32;
#endif
#ifndef u32
typedef unsigned int                u32;
#endif
#ifndef u64
typedef unsigned long long          u64;
#endif

#endif //__S8_T_

#ifndef bool
#define bool                unsigned char
#endif

#ifndef __SIZE_T_
#define __SIZE_T_
typedef u32                size_t;
#endif

typedef volatile u32                SSV6XXX_REG;

#ifdef __linux__
typedef int (*TASK_FUNC)(void *data);
#else
typedef void (*TASK_FUNC)(void *);
#endif

#ifndef __SSS8_T_
#define __SSS8_T_
#define __le16 u16
#define __le32 u32
#define __le64 u64

#define __u8 u8
#define __be16 u16
#define __be32 u32
#define __be64 u64

#define le16 u16
#define le32 u32
#define le64 u64

#define be16 u16
#define be32 u32
#define be64 u64

typedef unsigned int __u32;

#endif


#if defined(WIN32) && !defined(__cplusplus)
#define inline __inline
#endif




#define	ETHER_ADDR_LEN	6
#define	LLC_HEADER_LEN	6
#define	ETHER_TYPE_LEN	2
#define	ETHER_HDR_LEN	14
#define	ETHER_MAC_LEN	12		//802.3 DA+SA


#define OS_APIs
#define H_APIs
#define LIB_APIs
#define DRV_APIs



#ifndef NULL
#define NULL                        (void *)0
#endif

#ifndef true
#define true                        1
#endif

#ifndef false
#define false                       0
#endif

#ifndef TRUE
#define TRUE                        1
#endif

#ifndef FALSE
#define FALSE                       0
#endif

#ifndef __FUNCTION__
#ifdef __linux__
#else
#define __FUNCTION__ __func__
#endif
#endif
/* TODO:aaron */    
#if 0
#define ASSERT(x) \
{ \
    extern void halt (void); \
    if (!(x)) \
    { \
        LOG_PRINTF("Assert!! file: %s, function: %s, line: %d\n\t" #x, __FILE__, \
        	__FUNCTION__, __LINE__); \
        halt(); \
    } \
}

#define ASSERT_RET(x, ret) \
{ \
    extern void halt (void); \
    if (!(x)) \
    { \
        LOG_PRINTF("Assert!! file: %s, function: %s, line: %d\n\t" #x, __FILE__, \
        	__FUNCTION__, __LINE__); \
        halt(); \
        return ret; \
    } \
}


#else
#define ASSERT(x) \
{ \
    extern void halt (void); \
    if (!(x)) \
    { \
        halt(); \
    } \
}

#define ASSERT_RET(x, ret) \
{ \
    extern void halt (void); \
    if (!(x)) \
    { \
        halt(); \
        return ret; \
    } \
}
#endif


#define EMPTY

#undef assert
#define assert(x)                       ASSERT(x)

#define _LE16(x)                        (u16)(x)
#define IS_EQUAL(a, b)                  ( (a) == (b) )

#ifdef SET_BIT
#undef  SET_BIT
#endif
#define SET_BIT(v, b)					( (v) |= (0x01<<b) )


#define CLEAR_BIT(v, b)			    	( (v) &= ~(0x01<<b) )
#define IS_BIT_SET(v, b)				( (v) & (0x01<<(b) ) )

#define ETH_ADDR_FORMAT                 "%02X:%02X:%02X:%02X:%02X:%02X"
#define ETH_ADDR(a)                     ((ETHER_ADDR *)(a))->addr[0], ((ETHER_ADDR *)(a))->addr[1], \
                                        ((ETHER_ADDR *)(a))->addr[2], ((ETHER_ADDR *)(a))->addr[3], \
                                        ((ETHER_ADDR *)(a))->addr[4], ((ETHER_ADDR *)(a))->addr[5]
#define KEY_32_FORMAT                   "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X"
#define KEY_16_FORMAT                   "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X"
#define KEY_8_FORMAT                    "%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X"
#define KEY_32_VAL(m)                   (m)[0],(m)[1],(m)[2],(m)[3],(m)[4],(m)[5],(m)[6],(m)[7],(m)[8],(m)[9],(m)[10],(m)[11],(m)[12],(m)[13],(m)[14],(m)[15],(m)[16],(m)[18],(m)[18],(m)[19],(m)[20],(m)[21],(m)[22],(m)[23],(m)[24],(m)[25],(m)[26],(m)[27],(m)[28],(m)[29],(m)[30],(m)[31]
#define KEY_16_VAL(m)                   (m)[0],(m)[1],(m)[2],(m)[3],(m)[4],(m)[5],(m)[6],(m)[7],(m)[8],(m)[9],(m)[10],(m)[11],(m)[12],(m)[13],(m)[14],(m)[15]
#define KEY_8_VAL(m)                    (m)[0],(m)[1],(m)[2],(m)[3],(m)[4],(m)[5],(m)[6],(m)[7]





#ifndef BIT
#define BIT(nr)			(1UL << (nr))
#endif

#ifndef SIZE_1KB
#define		SIZE_1KB	(1024)
#endif

#ifndef SIZE_1MB
#define		SIZE_1MB	(1024 * SIZE_1KB)
#endif



#ifndef OFFSETOF
#define OFFSETOF(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef ARRAY_SIZE
#ifdef __linux__
#else
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif
#endif
#ifndef ARRAY_ELEM_SIZE
#define ARRAY_ELEM_SIZE(TYPE)   ((size_t)(&((TYPE *)100)[1]) - 100U)
#endif

typedef struct __timer_64_S {
    u32     lt;
    u32     ut;
} Time64_S;

typedef union {
        Time64_S   ts;
        u64        t;
} Time_T;

#endif /* _TYPES_H_ */

