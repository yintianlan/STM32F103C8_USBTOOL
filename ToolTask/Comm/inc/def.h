#ifndef _DEF_H_
#define _DEF_H_

typedef enum {FALSE = 0, TRUE = !FALSE} Bool;
typedef Bool BOOL;

typedef unsigned char  BYTE;
typedef unsigned int   UINT;
typedef unsigned short WORD;
typedef unsigned short UINT16;
typedef unsigned int  UINT32;
typedef unsigned int  DWORD;
typedef unsigned int  ULONG;

typedef unsigned char  U8;
typedef unsigned char  u8;
typedef unsigned short U16;
typedef unsigned short u16;
typedef unsigned int  U32;
typedef unsigned int  u32;

typedef unsigned char  Uint8;
typedef unsigned short Uint16;
typedef unsigned int  Uint32;

typedef char			int8;
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;

typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;

extern void _DI(void);
extern void _EI(void);


#endif

