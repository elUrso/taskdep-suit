#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#if defined(SMP) && defined(MUTEX)
#include <pthread.h>
#endif

/* mob_pach */
#ifndef  __cplusplus
typedef enum { false=0, true=-1 } bool;
#endif

#define T41_INCLUDE
#define T33_INCLUDE
#define  NEW
#define  XX  127

#if !defined(SMP) && !defined(SUN)
#define  lock_t int
#endif

typedef unsigned int INDEX;
typedef unsigned int square;

/*
  Macro that fetches positions of pieces
*/

#define  C_PIECES  3  /* Maximum # of pieces of one color OTB */

#define  SqFindKing(psq)       (psq[C_PIECES*(x_pieceKing-1)])
#define  SqFindOne(psq, pi)    (psq[C_PIECES*(pi-1)])
#define  SqFindFirst(psq, pi)  (psq[C_PIECES*(pi-1)])
#define  SqFindSecond(psq, pi) (psq[C_PIECES*(pi-1)+1])
#define  SqFindThird(psq, pi)  (psq[C_PIECES*(pi-1)+2])

#include "lock.h"

/*
  All defined, now include probing code
*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if !defined (DEBUG) && !defined (EBUG)
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif
#include <assert.h>

// SMP stuff

#if defined (SMP)
static  lock_t  lockLRU;
#else
#define LockInit(x)
#define Lock(x)
#define UnLock(x)
#endif

// Declarations

typedef unsigned        char BYTE;
typedef unsigned        long ULONG;
typedef signed          char tb_t;

#if !defined (COLOR_DECLARED)
typedef int     color;
#define x_colorWhite    0
#define x_colorBlack    1
#define x_colorNeutral  2
#define COLOR_DECLARED
#endif

#if !defined (PIECES_DECLARED)
typedef int     piece;
#define x_pieceNone             0
#define x_piecePawn             1
#define x_pieceKnight           2
#define x_pieceBishop           3
#define x_pieceRook             4
#define x_pieceQueen            5
#define x_pieceKing             6
#define PIECES_DECLARED
#endif

#if !defined (SqFind2)
#define SqFind2(psq,pi1,sq1,pi2,sq2)    sq1=SqFindFirst(psq,pi1);sq2=SqFindFirst(psq,pi2);
#endif

// Machine and compiler-specific declarations

#if defined (_MSC_VER)

#undef  TB_CDECL
#define TB_CDECL        __cdecl
#define TB_FASTCALL     __fastcall
#if _MSC_VER >= 1200
#define INLINE          __forceinline
#endif

#else

#define TB_CDECL
#define TB_FASTCALL

#endif

#if !defined (INLINE)
#define INLINE  inline
#endif

// Some constants from SJE program

#define pageL 256

/* tablebase byte entry semispan length */

#define tbbe_ssL ((pageL - 4) / 2)

/* tablebase signed byte entry values */

#define bev_broken  (tbbe_ssL + 1)  /* illegal or busted */

#define bev_mi1     tbbe_ssL        /* mate in 1 move */
#define bev_mimin   1               /* mate in 126 moves */

#define bev_draw    0               /* draw */

#define bev_limax   (-1)            /* mated in 125 moves */
#define bev_li0     (-tbbe_ssL)     /* mated in 0 moves */

#define bev_limaxx      (-tbbe_ssL - 1) /* mated in 126 moves */
#define bev_miminx      (-tbbe_ssL - 2) /* mate in 127 moves */

// Some constants for 16-bit tables

#define L_pageL 65536

/* tablebase short entry semispan length */

#define L_tbbe_ssL ((L_pageL - 4) / 2)

/* tablebase signed short entry values */

#define L_bev_broken  (L_tbbe_ssL + 1)          /* illegal or busted */

#define L_bev_mi1     L_tbbe_ssL                        /* mate in 1 move */
#define L_bev_mimin   1                                         /* mate in 32766 moves */

#define L_bev_draw    0                                         /* draw */

#define L_bev_limax   (-1)                                      /* mated in 32765 moves */
#define L_bev_li0     (-L_tbbe_ssL)                     /* mated in 0 moves */

#define L_bev_limaxx    (-L_tbbe_ssL - 1)       /* mated in 32766 moves */
#define L_bev_miminx    (-L_tbbe_ssL - 2)       /* mate in 32767 moves */

// Convertion from 8-bit to 16-bit score
// UNDONE: Maybe implement via lookup table?

#define S_to_L(tbt) ((0 == tbt) ? 0: (tbt > 0) ? (bev_broken != tbt ? tbt + 32640 : L_bev_broken):(tbt >= bev_li0) ? tbt - 32640:(bev_limaxx == tbt) ? -32640: 32640 )

// Constants

#define i8      ((INDEX) 8)
#define i14     ((INDEX) 14)
#define i44     ((INDEX) 44)
#define i46     ((INDEX) 46)
#define i47     ((INDEX) 47)
#define i48     ((INDEX) 48)
#define i58     ((INDEX) 58)
#define i59     ((INDEX) 59)
#define i60     ((INDEX) 60)
#define i61     ((INDEX) 61)
#define i62     ((INDEX) 62)
#define i63     ((INDEX) 63)
#define i64     ((INDEX) 64)

#define x_row_1 0
#define x_row_2 1
#define x_row_3 2
#define x_row_4 3
#define x_row_5 4
#define x_row_6 5
#define x_row_7 6
#define x_row_8 7

#define x_column_a 0
#define x_column_b 1
#define x_column_c 2
#define x_column_d 3
#define x_column_e 4
#define x_column_f 5
#define x_column_g 6
#define x_column_h 7

/* reflection macros */

#define reflect_x(sq) ((sq) ^ 0x38)
#define reflect_y(sq) ((sq) ^ 0x07)
#define reflect_xy(sq) rgsqReflectXY[sq]

static const square rgsqReflectXY [] =
{
  0,  8, 16, 24, 32, 40, 48, 56,
  1,  9, 17, 25, 33, 41, 49, 57,
  2, 10, 18, 26, 34, 42, 50, 58,
  3, 11, 19, 27, 35, 43, 51, 59,
  4, 12, 20, 28, 36, 44, 52, 60,
  5, 13, 21, 29, 37, 45, 53, 61,
  6, 14, 22, 30, 38, 46, 54, 62,
  7, 15, 23, 31, 39, 47, 55, 63,
};

static const square rgsqReflectMaskY [] =
{
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
};

static const square rgsqReflectMaskYandX [] =
{
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0, 0, 0, 0, 7, 7, 7, 7,
 0x38, 0x38, 0x38, 0x38, 0x38+7, 0x38+7, 0x38+7, 0x38+7,
 0x38, 0x38, 0x38, 0x38, 0x38+7, 0x38+7, 0x38+7, 0x38+7,
 0x38, 0x38, 0x38, 0x38, 0x38+7, 0x38+7, 0x38+7, 0x38+7,
 0x38, 0x38, 0x38, 0x38, 0x38+7, 0x38+7, 0x38+7, 0x38+7,
};

static const square rgsqReflectInvertMask[] = { 0, 0x38 };

/* useful macros */

#define TbRow(sq)                               ((sq) >> 3)
#define TbColumn(sq)                    ((sq) & 7)

#if defined (NEW)
#define PchExt(side) ((x_colorWhite == side) ? ".nbw" : ".nbb")
#else
#define PchExt(side) ((x_colorWhite == side) ? ".tbw" : ".tbb")
#endif

// Verbose levels

static bool             fPrint = false;         // Print some technical statistics
static bool             fVerbose = false;       // Print additional information

// Malloc that checks for out-of-memory condition

static size_t   cbAllocated;

static void* PvMalloc
        (
        size_t cb
        )
        {
        void    *pv;

        pv = malloc (cb);
        if (NULL == pv)
                {
                printf ("*** Cannot allocate %d bytes of memory\n", cb);
                exit (1);
                }
        cbAllocated += cb;
        return pv;
        }

#if defined (NEW)       // New index schema ----------------------------------------

// 'Invalid' value have to be large, so index
// of invalid position will be very large, too.

#define INF     4000

// Enumeration: valid positions with 2 kings on board; white king restricted to
// a1-d1-d4 triangle; also, if it's at a1-d4 half-diagonal, then black king
// must be in a1-h1-h8 triangle

static const short rgsTriKings [64 * 64] =
{
 INF, INF,   0,   1,   2,   3,   4,   5, INF, INF,   6,   7,   8,   9,  10,  11,
 INF, INF,  12,  13,  14,  15,  16,  17, INF, INF, INF,  18,  19,  20,  21,  22,
 INF, INF, INF, INF,  23,  24,  25,  26, INF, INF, INF, INF, INF,  27,  28,  29,
 INF, INF, INF, INF, INF, INF,  30,  31, INF, INF, INF, INF, INF, INF, INF,  32,
 INF, INF, INF,  33,  34,  35,  36,  37, INF, INF, INF,  38,  39,  40,  41,  42,
  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,
  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,
  91, INF, INF, INF,  92,  93,  94,  95,  96, INF, INF, INF,  97,  98,  99, 100,
 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148,
 149, 150, INF, INF, INF, 151, 152, 153, 154, 155, INF, INF, INF, 156, 157, 158,
 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174,
 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190,
 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, 207, 208, 209, 210, 211, INF, INF, INF, 212, 213, 214, 215, 216,
 INF, INF, INF, 217, 218, 219, 220, 221, INF, INF, INF, 222, 223, 224, 225, 226,
 INF, INF, INF, INF, 227, 228, 229, 230, INF, INF, INF, INF, INF, 231, 232, 233,
 INF, INF, INF, INF, INF, INF, 234, 235, INF, INF, INF, INF, INF, INF, INF, 236,
 237, INF, INF, INF, 238, 239, 240, 241, 242, INF, INF, INF, 243, 244, 245, 246,
 247, INF, INF, INF, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259,
 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275,
 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291,
 292, 293, INF, INF, INF, 294, 295, 296, 297, 298, INF, INF, INF, 299, 300, 301,
 302, 303, INF, INF, INF, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314,
 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330,
 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 347, 348, 349, 350, 351, 352, 353, 354, INF, INF, INF, INF, 355, 356, 357, 358,
 INF, INF, INF, INF, 359, 360, 361, 362, INF, INF, INF, INF, 363, 364, 365, 366,
 INF, INF, INF, INF, 367, 368, 369, 370, INF, INF, INF, INF, INF, 371, 372, 373,
 INF, INF, INF, INF, INF, INF, 374, 375, INF, INF, INF, INF, INF, INF, INF, 376,
 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, INF, INF, INF, 387, 388, 389,
 390, 391, INF, INF, INF, 392, 393, 394, 395, 396, INF, INF, INF, 397, 398, 399,
 400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414, 415,
 416, 417, 418, 419, 420, 421, 422, 423, 424, 425, 426, 427, 428, 429, 430, 431,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 432, 433, 434, 435, 436, 437, 438, 439, INF, 440, 441, 442, 443, 444, 445, 446,
 INF, INF, INF, INF, INF, 447, 448, 449, INF, INF, INF, INF, INF, 450, 451, 452,
 INF, INF, INF, INF, INF, 453, 454, 455, INF, INF, INF, INF, INF, 456, 457, 458,
 INF, INF, INF, INF, INF, INF, 459, 460, INF, INF, INF, INF, INF, INF, INF, 461,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
};

// Enumeration: all valid positions with 2 kings on board when white king
// restricted to left half of the board

static const short rgsHalfKings [64 * 64] =
{
 INF, INF,   0,   1,   2,   3,   4,   5, INF, INF,   6,   7,   8,   9,  10,  11,
  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,
  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
 INF, INF, INF,  60,  61,  62,  63,  64, INF, INF, INF,  65,  66,  67,  68,  69,
  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,
  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101,
 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117,
 118, INF, INF, INF, 119, 120, 121, 122, 123, INF, INF, INF, 124, 125, 126, 127,
 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
 176, 177, INF, INF, INF, 178, 179, 180, 181, 182, INF, INF, INF, 183, 184, 185,
 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201,
 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, 234, 235, 236, 237, 238, 239, INF, INF, 240, 241, 242, 243, 244, 245,
 INF, INF, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259,
 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275,
 276, 277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291,
 INF, INF, INF, 292, 293, 294, 295, 296, INF, INF, INF, 297, 298, 299, 300, 301,
 INF, INF, INF, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314,
 315, 316, 317, 318, 319, 320, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330,
 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346,
 347, INF, INF, INF, 348, 349, 350, 351, 352, INF, INF, INF, 353, 354, 355, 356,
 357, INF, INF, INF, 358, 359, 360, 361, 362, 363, 364, 365, 366, 367, 368, 369,
 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385,
 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401,
 402, 403, INF, INF, INF, 404, 405, 406, 407, 408, INF, INF, INF, 409, 410, 411,
 412, 413, INF, INF, INF, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423, 424,
 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436, 437, 438, 439, 440,
 441, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 457, 458, 459, 460, 461, 462, 463, 464, INF, INF, 465, 466, 467, 468, 469, 470,
 INF, INF, 471, 472, 473, 474, 475, 476, INF, INF, 477, 478, 479, 480, 481, 482,
 483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 495, 496, 497, 498,
 499, 500, 501, 502, 503, 504, 505, 506, 507, 508, 509, 510, 511, 512, 513, 514,
 515, 516, 517, 518, 519, 520, 521, 522, INF, INF, INF, 523, 524, 525, 526, 527,
 INF, INF, INF, 528, 529, 530, 531, 532, INF, INF, INF, 533, 534, 535, 536, 537,
 538, 539, 540, 541, 542, 543, 544, 545, 546, 547, 548, 549, 550, 551, 552, 553,
 554, 555, 556, 557, 558, 559, 560, 561, 562, 563, 564, 565, 566, 567, 568, 569,
 570, 571, 572, 573, 574, 575, 576, 577, 578, INF, INF, INF, 579, 580, 581, 582,
 583, INF, INF, INF, 584, 585, 586, 587, 588, INF, INF, INF, 589, 590, 591, 592,
 593, 594, 595, 596, 597, 598, 599, 600, 601, 602, 603, 604, 605, 606, 607, 608,
 609, 610, 611, 612, 613, 614, 615, 616, 617, 618, 619, 620, 621, 622, 623, 624,
 625, 626, 627, 628, 629, 630, 631, 632, 633, 634, INF, INF, INF, 635, 636, 637,
 638, 639, INF, INF, INF, 640, 641, 642, 643, 644, INF, INF, INF, 645, 646, 647,
 648, 649, 650, 651, 652, 653, 654, 655, 656, 657, 658, 659, 660, 661, 662, 663,
 664, 665, 666, 667, 668, 669, 670, 671, 672, 673, 674, 675, 676, 677, 678, 679,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 680, 681, 682, 683, 684, 685, 686, 687, 688, 689, 690, 691, 692, 693, 694, 695,
 INF, INF, 696, 697, 698, 699, 700, 701, INF, INF, 702, 703, 704, 705, 706, 707,
 INF, INF, 708, 709, 710, 711, 712, 713, 714, 715, 716, 717, 718, 719, 720, 721,
 722, 723, 724, 725, 726, 727, 728, 729, 730, 731, 732, 733, 734, 735, 736, 737,
 738, 739, 740, 741, 742, 743, 744, 745, 746, 747, 748, 749, 750, 751, 752, 753,
 INF, INF, INF, 754, 755, 756, 757, 758, INF, INF, INF, 759, 760, 761, 762, 763,
 INF, INF, INF, 764, 765, 766, 767, 768, 769, 770, 771, 772, 773, 774, 775, 776,
 777, 778, 779, 780, 781, 782, 783, 784, 785, 786, 787, 788, 789, 790, 791, 792,
 793, 794, 795, 796, 797, 798, 799, 800, 801, 802, 803, 804, 805, 806, 807, 808,
 809, INF, INF, INF, 810, 811, 812, 813, 814, INF, INF, INF, 815, 816, 817, 818,
 819, INF, INF, INF, 820, 821, 822, 823, 824, 825, 826, 827, 828, 829, 830, 831,
 832, 833, 834, 835, 836, 837, 838, 839, 840, 841, 842, 843, 844, 845, 846, 847,
 848, 849, 850, 851, 852, 853, 854, 855, 856, 857, 858, 859, 860, 861, 862, 863,
 864, 865, INF, INF, INF, 866, 867, 868, 869, 870, INF, INF, INF, 871, 872, 873,
 874, 875, INF, INF, INF, 876, 877, 878, 879, 880, 881, 882, 883, 884, 885, 886,
 887, 888, 889, 890, 891, 892, 893, 894, 895, 896, 897, 898, 899, 900, 901, 902,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 903, 904, 905, 906, 907, 908, 909, 910, 911, 912, 913, 914, 915, 916, 917, 918,
 919, 920, 921, 922, 923, 924, 925, 926, INF, INF, 927, 928, 929, 930, 931, 932,
 INF, INF, 933, 934, 935, 936, 937, 938, INF, INF, 939, 940, 941, 942, 943, 944,
 945, 946, 947, 948, 949, 950, 951, 952, 953, 954, 955, 956, 957, 958, 959, 960,
 961, 962, 963, 964, 965, 966, 967, 968, 969, 970, 971, 972, 973, 974, 975, 976,
 977, 978, 979, 980, 981, 982, 983, 984, INF, INF, INF, 985, 986, 987, 988, 989,
 INF, INF, INF, 990, 991, 992, 993, 994, INF, INF, INF, 995, 996, 997, 998, 999,
1000,1001,1002,1003,1004,1005,1006,1007,1008,1009,1010,1011,1012,1013,1014,1015,
1016,1017,1018,1019,1020,1021,1022,1023,1024,1025,1026,1027,1028,1029,1030,1031,
1032,1033,1034,1035,1036,1037,1038,1039,1040, INF, INF, INF,1041,1042,1043,1044,
1045, INF, INF, INF,1046,1047,1048,1049,1050, INF, INF, INF,1051,1052,1053,1054,
1055,1056,1057,1058,1059,1060,1061,1062,1063,1064,1065,1066,1067,1068,1069,1070,
1071,1072,1073,1074,1075,1076,1077,1078,1079,1080,1081,1082,1083,1084,1085,1086,
1087,1088,1089,1090,1091,1092,1093,1094,1095,1096, INF, INF, INF,1097,1098,1099,
1100,1101, INF, INF, INF,1102,1103,1104,1105,1106, INF, INF, INF,1107,1108,1109,
1110,1111,1112,1113,1114,1115,1116,1117,1118,1119,1120,1121,1122,1123,1124,1125,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
1126,1127,1128,1129,1130,1131,1132,1133,1134,1135,1136,1137,1138,1139,1140,1141,
1142,1143,1144,1145,1146,1147,1148,1149,1150,1151,1152,1153,1154,1155,1156,1157,
 INF, INF,1158,1159,1160,1161,1162,1163, INF, INF,1164,1165,1166,1167,1168,1169,
 INF, INF,1170,1171,1172,1173,1174,1175,1176,1177,1178,1179,1180,1181,1182,1183,
1184,1185,1186,1187,1188,1189,1190,1191,1192,1193,1194,1195,1196,1197,1198,1199,
1200,1201,1202,1203,1204,1205,1206,1207,1208,1209,1210,1211,1212,1213,1214,1215,
 INF, INF, INF,1216,1217,1218,1219,1220, INF, INF, INF,1221,1222,1223,1224,1225,
 INF, INF, INF,1226,1227,1228,1229,1230,1231,1232,1233,1234,1235,1236,1237,1238,
1239,1240,1241,1242,1243,1244,1245,1246,1247,1248,1249,1250,1251,1252,1253,1254,
1255,1256,1257,1258,1259,1260,1261,1262,1263,1264,1265,1266,1267,1268,1269,1270,
1271, INF, INF, INF,1272,1273,1274,1275,1276, INF, INF, INF,1277,1278,1279,1280,
1281, INF, INF, INF,1282,1283,1284,1285,1286,1287,1288,1289,1290,1291,1292,1293,
1294,1295,1296,1297,1298,1299,1300,1301,1302,1303,1304,1305,1306,1307,1308,1309,
1310,1311,1312,1313,1314,1315,1316,1317,1318,1319,1320,1321,1322,1323,1324,1325,
1326,1327, INF, INF, INF,1328,1329,1330,1331,1332, INF, INF, INF,1333,1334,1335,
1336,1337, INF, INF, INF,1338,1339,1340,1341,1342,1343,1344,1345,1346,1347,1348,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
1349,1350,1351,1352,1353,1354,1355,1356,1357,1358,1359,1360,1361,1362,1363,1364,
1365,1366,1367,1368,1369,1370,1371,1372,1373,1374,1375,1376,1377,1378,1379,1380,
1381,1382,1383,1384,1385,1386,1387,1388, INF, INF,1389,1390,1391,1392,1393,1394,
 INF, INF,1395,1396,1397,1398,1399,1400, INF, INF,1401,1402,1403,1404,1405,1406,
1407,1408,1409,1410,1411,1412,1413,1414,1415,1416,1417,1418,1419,1420,1421,1422,
1423,1424,1425,1426,1427,1428,1429,1430,1431,1432,1433,1434,1435,1436,1437,1438,
1439,1440,1441,1442,1443,1444,1445,1446, INF, INF, INF,1447,1448,1449,1450,1451,
 INF, INF, INF,1452,1453,1454,1455,1456, INF, INF, INF,1457,1458,1459,1460,1461,
1462,1463,1464,1465,1466,1467,1468,1469,1470,1471,1472,1473,1474,1475,1476,1477,
1478,1479,1480,1481,1482,1483,1484,1485,1486,1487,1488,1489,1490,1491,1492,1493,
1494,1495,1496,1497,1498,1499,1500,1501,1502, INF, INF, INF,1503,1504,1505,1506,
1507, INF, INF, INF,1508,1509,1510,1511,1512, INF, INF, INF,1513,1514,1515,1516,
1517,1518,1519,1520,1521,1522,1523,1524,1525,1526,1527,1528,1529,1530,1531,1532,
1533,1534,1535,1536,1537,1538,1539,1540,1541,1542,1543,1544,1545,1546,1547,1548,
1549,1550,1551,1552,1553,1554,1555,1556,1557,1558, INF, INF, INF,1559,1560,1561,
1562,1563, INF, INF, INF,1564,1565,1566,1567,1568, INF, INF, INF,1569,1570,1571,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
1572,1573,1574,1575,1576,1577,1578,1579,1580,1581,1582,1583,1584,1585,1586,1587,
1588,1589,1590,1591,1592,1593,1594,1595,1596,1597,1598,1599,1600,1601,1602,1603,
1604,1605,1606,1607,1608,1609,1610,1611,1612,1613,1614,1615,1616,1617,1618,1619,
 INF, INF,1620,1621,1622,1623,1624,1625, INF, INF,1626,1627,1628,1629,1630,1631,
1632,1633,1634,1635,1636,1637,1638,1639,1640,1641,1642,1643,1644,1645,1646,1647,
1648,1649,1650,1651,1652,1653,1654,1655,1656,1657,1658,1659,1660,1661,1662,1663,
1664,1665,1666,1667,1668,1669,1670,1671,1672,1673,1674,1675,1676,1677,1678,1679,
 INF, INF, INF,1680,1681,1682,1683,1684, INF, INF, INF,1685,1686,1687,1688,1689,
1690,1691,1692,1693,1694,1695,1696,1697,1698,1699,1700,1701,1702,1703,1704,1705,
1706,1707,1708,1709,1710,1711,1712,1713,1714,1715,1716,1717,1718,1719,1720,1721,
1722,1723,1724,1725,1726,1727,1728,1729,1730,1731,1732,1733,1734,1735,1736,1737,
1738, INF, INF, INF,1739,1740,1741,1742,1743, INF, INF, INF,1744,1745,1746,1747,
1748,1749,1750,1751,1752,1753,1754,1755,1756,1757,1758,1759,1760,1761,1762,1763,
1764,1765,1766,1767,1768,1769,1770,1771,1772,1773,1774,1775,1776,1777,1778,1779,
1780,1781,1782,1783,1784,1785,1786,1787,1788,1789,1790,1791,1792,1793,1794,1795,
1796,1797, INF, INF, INF,1798,1799,1800,1801,1802, INF, INF, INF,1803,1804,1805,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
 INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF, INF,
};

// Useful macro and enumeration tables

#define IndTriKings(sqk1,sqk2)  ((INDEX) rgsTriKings[sqk1*64+sqk2])
#define IndHalfKings(sqk1,sqk2) ((INDEX) rgsHalfKings[sqk1*64+sqk2])

static const bool rgfTriangle[64] =
{
         true,  true,  true,  true,  false, false, false, false,
         false, true,  true,  true,  false, false, false, false,
         false, false, true,  true,  false, false, false, false,
         false, false, false, true,  false, false, false, false,
         false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false,
         false, false, false, false, false, false, false, false,
};

static const bool rgfNotDiagonal[64] =
{
         false, true,  true,  true,  true,  true,  true,  true,
         true,  false, true,  true,  true,  true,  true,  true,
         true,  true,  false, true,  true,  true,  true,  true,
         true,  true,  true,  false, true,  true,  true,  true,
         true,  true,  true,  true,  false, true,  true,  true,
         true,  true,  true,  true,  true,  false, true,  true,
         true,  true,  true,  true,  true,  true,  false, true,
         true,  true,  true,  true,  true,  true,  true,  false,
};

static const bool rgfInLargeTriangle[64] =
{
         true,  true,  true,  true,  true,  true,  true,  true,
         false, true,  true,  true,  true,  true,  true,  true,
         false, false, true,  true,  true,  true,  true,  true,
         false, false, false, true,  true,  true,  true,  true,
         false, false, false, false, true,  true,  true,  true,
         false, false, false, false, false, true,  true,  true,
         false, false, false, false, false, false, true,  true,
         false, false, false, false, false, false, false, true,
};

#define FInTriangle(sqwk,sqbk)  (rgfTriangle[sqwk] & (rgfNotDiagonal[sqwk]|rgfInLargeTriangle[sqbk]))

// Sort pieces

#define SORT(sq1,sq2)   if (sq1>sq2) { square sqTmp; sqTmp=sq1; sq1=sq2; sq2=sqTmp; }

// Exclude occupied squares

#define EXCLUDE1(sq,sq1)                                                (sq-(sq>sq1))
#define EXCLUDE2(sq,sq1,sq2)                                    (sq-((sq>sq1)+(sq>sq2)))
#define EXCLUDE3(sq,sq1,sq2,sq3)                                (sq-((sq>sq1)+(sq>sq2)+(sq>sq3)))
#define EXCLUDE4(sq,sq1,sq2,sq3,sq4)                    (sq-((sq>sq1)+(sq>sq2)+(sq>sq3)+(sq>sq4)))
#define EXCLUDE5(sq,sq1,sq2,sq3,sq4,sq5)                (sq-((sq>sq1)+(sq>sq2)+(sq>sq3)+(sq>sq4)+(sq>sq5)))
#define EXCLUDE6(sq,sq1,sq2,sq3,sq4,sq5,sq6)    (sq-((sq>sq1)+(sq>sq2)+(sq>sq3)+(sq>sq4)+(sq>sq5)+(sq>sq6)))

// Calculate index - a lot of functions...

// Enumeration tables

static BYTE             *rgprgsqPiece[6];       // Enumeration for each piece (0 - black pawn)
                                                                        // For each position of the King, all legal squares
                                                                        // of the opposite piece enumerated
static BYTE             rgcLegal[6][64];        // # of enumerated positions for each piece and each
                                                                        // location of enemy king

// Enumerations - indexed by [piece] and [kings enumeration].
// In each table for each [piece] and [king enumeration] we store # of preceeding positions.

static ULONG    *rgprgulSinglePawnless[6];
static ULONG    *rgprgulPairPawnless[6][6];
#if defined (T41_INCLUDE)
static ULONG    *rgprgulTriplePawnless[6][6][6];
#endif
static ULONG    *rgprgulSinglePawnPresent[6];
static ULONG    *rgprgulPairPawnPresent[6][6];
#if defined (T41_INCLUDE)
static ULONG    *rgprgulTriplePawnPresent[6][6][6];
#endif

// Total # of enumerated positions

static ULONG    rgcSinglePawnPresent[6];
static ULONG    rgcSinglePawnless[6];
static ULONG    rgcPairPawnPresent[6][6];
static ULONG    rgcPairPawnless[6][6];
#if defined (T41_INCLUDE)
static ULONG    rgcTriplePawnPresent[6][6][6];
static ULONG    rgcTriplePawnless[6][6][6];
#endif

// Infinities. Have to be larger than any legal enumeration yet small enough
// so there will be no overflow when combining them with remaining pieces.

#define INF_SINGLE      (110000)
#define INF_PAIR        (6500000)
#define INF_TRIPLE      (300000000)

// Initialize squares and counters table for one piece.
// Piece can be x_pieceNone - that means 'pawn of the wrong color', e.g. KPK BTM.

static void VInitSquaresTable
        (
        piece   pi,
        BYTE    *prgsqPiece,
        BYTE    *prgcLegal
        )
        {
        int     sqLo, sqHi;

        memset (prgsqPiece, -1, 64*64);
        sqLo = 0;
        sqHi = 64;
        if (pi <= x_piecePawn)
                {
                sqLo = 8;
                sqHi = 56;
                }
/* mob_pach */
#ifndef  __cplusplus
{
   int sqKing;
        for (sqKing = 0; sqKing < 64; sqKing ++)
#else
        for (int sqKing = 0; sqKing < 64; sqKing ++)
#endif
                {
                int     iPiece;

                iPiece = 0;
/* mob_pach */
#ifndef  __cplusplus
   {
      int sq;
        for (sq = sqLo; sq < sqHi; sq ++)
#else
                for (int sq = sqLo; sq < sqHi; sq ++)
#endif
                        {
                        if (sq == sqKing)
                                continue;
                        switch (pi)
                                {
                        case x_piecePawn:
                                if (
                                        (0 != TbColumn (sq) && sqKing == sq+7) ||
                                        (7 != TbColumn (sq) && sqKing == sq+9)
                                   )
                                   continue;
                                break;
                        case x_pieceKnight:
                                if (
                                        (TbRow (sq) >= 2 && TbColumn (sq) >= 1 && sqKing == sq-17) ||
                                        (TbRow (sq) >= 2 && TbColumn (sq) <= 6 && sqKing == sq-15) ||
                                        (TbRow (sq) >= 1 && TbColumn (sq) >= 2 && sqKing == sq-10) ||
                                        (TbRow (sq) >= 1 && TbColumn (sq) <= 5 && sqKing == sq-6) ||
                                        (TbRow (sq) <= 6 && TbColumn (sq) >= 2 && sqKing == sq+6) ||
                                        (TbRow (sq) <= 6 && TbColumn (sq) <= 5 && sqKing == sq+10) ||
                                        (TbRow (sq) <= 5 && TbColumn (sq) >= 1 && sqKing == sq+15) ||
                                        (TbRow (sq) <= 5 && TbColumn (sq) <= 6 && sqKing == sq+17)
                                   )
                                        continue;
                                break;
                        case x_pieceBishop:
                                if (
                                        (0 != TbRow (sq) && 0 != TbColumn (sq) && sqKing == sq-9) ||
                                        (0 != TbRow (sq) && 7 != TbColumn (sq) && sqKing == sq-7) ||
                                        (7 != TbRow (sq) && 0 != TbColumn (sq) && sqKing == sq+7) ||
                                        (7 != TbRow (sq) && 7 != TbColumn (sq) && sqKing == sq+9)
                                   )
                                        continue;
                                break;
                        case x_pieceRook:
                                if (
                                        (0 != TbColumn (sq) && sqKing == sq-1) ||
                                        (7 != TbColumn (sq) && sqKing == sq+1) ||
                                        (0 != TbRow (sq) && sqKing == sq-8) ||
                                        (7 != TbRow (sq) && sqKing == sq+8)
                                   )
                                        continue;
                                break;
                        case x_pieceQueen:
                                if (
                                        (0 != TbColumn (sq) && sqKing == sq-1) ||
                                        (7 != TbColumn (sq) && sqKing == sq+1) ||
                                        (0 != TbRow (sq) && sqKing == sq-8) ||
                                        (7 != TbRow (sq) && sqKing == sq+8) ||
                                        (0 != TbRow (sq) && 0 != TbColumn (sq) && sqKing == sq-9) ||
                                        (0 != TbRow (sq) && 7 != TbColumn (sq) && sqKing == sq-7) ||
                                        (7 != TbRow (sq) && 0 != TbColumn (sq) && sqKing == sq+7) ||
                                        (7 != TbRow (sq) && 7 != TbColumn (sq) && sqKing == sq+9)
                                   )
                                        continue;
                                break;
                                }
                        prgsqPiece[sqKing*64+sq] = (BYTE) iPiece;
                        iPiece ++;
                        }
/* mob_pach */
#ifndef  __cplusplus
   }
#endif
                prgcLegal[sqKing] = (BYTE) iPiece;
                }
/* mob_pach */
#ifndef  __cplusplus
}
#endif
        }

// Initialize enumeration table for single piece

static void VInitSingle
        (
        ULONG           *prgIndex,
        const short     *prgsKings,
        const BYTE      *prgcLegal,
        const BYTE      *prgsqPiece,
        ULONG           *pcEnumeration
        )
        {
        ULONG iIndex;

        iIndex = 0;
/* mob_pach */
#ifndef  __cplusplus
{
   int sqKing1;
   int sqKing2;
        for (sqKing1 = 0; sqKing1 < 64; sqKing1 ++)
                for (sqKing2 = 0; sqKing2 < 64; sqKing2 ++)
#else
        for (int sqKing1 = 0; sqKing1 < 64; sqKing1 ++)
                for (int sqKing2 = 0; sqKing2 < 64; sqKing2 ++)
#endif
                        {
                        if (INF != prgsKings[sqKing1*64+sqKing2])
                                {
                                prgIndex[prgsKings[sqKing1*64+sqKing2]] = iIndex;
                                iIndex += prgcLegal[sqKing2] - ((BYTE) -1 != prgsqPiece[sqKing2*64+sqKing1]);
                                }
                        }
/* mob_pach */
#ifndef  __cplusplus
}
#endif
        *pcEnumeration = iIndex;
        }

// Initialize enumeration table for pair of pieces

static void VInitPair
        (
        ULONG           *prgIndex,
        const short     *prgsKings,
        const BYTE      *prgcLegal1,
        const BYTE      *prgsqPiece1,
        const BYTE      *prgcLegal2,
        const BYTE      *prgsqPiece2,
        ULONG           *pcEnumeration
        )
        {
        ULONG iIndex;
        ULONG cPositions1, cPositions2;

        iIndex = 0;
/* mob_pach */
#ifndef  __cplusplus
{
   int sqKing1;
   int sqKing2;
        for (sqKing1 = 0; sqKing1 < 64; sqKing1 ++)
                for (sqKing2 = 0; sqKing2 < 64; sqKing2 ++)
#else
        for (int sqKing1 = 0; sqKing1 < 64; sqKing1 ++)
                for (int sqKing2 = 0; sqKing2 < 64; sqKing2 ++)
#endif
                        {
                        if (INF != prgsKings[sqKing1*64+sqKing2])
                                {
                                prgIndex[prgsKings[sqKing1*64+sqKing2]] = iIndex;
                                cPositions1 = prgcLegal1[sqKing2] - ((BYTE) -1 != prgsqPiece1[sqKing2*64+sqKing1]);
                                if (prgcLegal1 == prgcLegal2)
                                        iIndex += cPositions1*(cPositions1-1)/2;
                                else
                                        {
                                        cPositions2 = prgcLegal2[sqKing2] - ((BYTE) -1 != prgsqPiece2[sqKing2*64+sqKing1]);
                                        iIndex += cPositions1*cPositions2;
                                        }
                                }
                        }
/* mob_pach */
#ifndef  __cplusplus
}
#endif
        *pcEnumeration = iIndex;
        }

#if defined (T41_INCLUDE)

// Initialize enumeration table for triple piece

static void VInitTriple
        (
        ULONG           *prgIndex,
        const short     *prgsKings,
        const BYTE      *prgcLegal1,
        const BYTE      *prgsqPiece1,
        const BYTE      *prgcLegal2,
        const BYTE      *prgsqPiece2,
        const BYTE      *prgcLegal3,
        const BYTE      *prgsqPiece3,
        ULONG           *pcEnumeration
        )
        {
        ULONG iIndex;
        ULONG cPositions1, cPositions2, cPositions3;

        iIndex = 0;
/* mob_pach */
#ifndef  __cplusplus
{
   int sqKing1;
   int sqKing2;
        for (sqKing1 = 0; sqKing1 < 64; sqKing1 ++)
                for (sqKing2 = 0; sqKing2 < 64; sqKing2 ++)
#else
        for (int sqKing1 = 0; sqKing1 < 64; sqKing1 ++)
                for (int sqKing2 = 0; sqKing2 < 64; sqKing2 ++)
#endif
                        {
                        if (INF != prgsKings[sqKing1*64+sqKing2])
                                {
                                prgIndex[prgsKings[sqKing1*64+sqKing2]] = iIndex;
                                cPositions1 = prgcLegal1[sqKing2] - ((BYTE) -1 != prgsqPiece1[sqKing2*64+sqKing1]);
                                if (prgcLegal1 == prgcLegal2 && prgcLegal2 == prgcLegal3)
                                        iIndex += cPositions1*(cPositions1-1)*(cPositions1-2)/6;
                                else if (prgcLegal1 == prgcLegal2)
                                        {
                                        cPositions3 = prgcLegal3[sqKing2] - ((BYTE) -1 != prgsqPiece3[sqKing2*64+sqKing1]);
                                        iIndex += cPositions1*(cPositions1-1)/2*cPositions3;
                                        }
                                else if (prgcLegal2 == prgcLegal3)
                                        {
                                        cPositions2 = prgcLegal2[sqKing2] - ((BYTE) -1 != prgsqPiece2[sqKing2*64+sqKing1]);
                                        iIndex += cPositions1*cPositions2*(cPositions2-1)/2;
                                        }
                                else
                                        {
                                        cPositions2 = prgcLegal2[sqKing2] - ((BYTE) -1 != prgsqPiece2[sqKing2*64+sqKing1]);
                                        cPositions3 = prgcLegal3[sqKing2] - ((BYTE) -1 != prgsqPiece3[sqKing2*64+sqKing1]);
                                        iIndex += cPositions1*cPositions2*cPositions3;
                                        }
                                }
                        }
/* mob_pach */
#ifndef  __cplusplus
}
#endif
        *pcEnumeration = iIndex;
        }

#endif

// Initialize all Enumeration tables

static bool fEnumerationInitted = false;

static void VInitEnumerations (void)
        {
        piece   pi1;
        piece   pi2;
#if defined (T41_INCLUDE)
        piece   pi3;
#endif

        if (fEnumerationInitted)
                return;
        fEnumerationInitted = true;
        // Initialize square tables
        for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
                {
                rgprgsqPiece[pi1] = (BYTE *) PvMalloc (64*64);
                VInitSquaresTable (pi1, rgprgsqPiece[pi1], rgcLegal[pi1]);
                }

        for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
                {
                // Initialize enumeration tables for single piece
                rgprgulSinglePawnPresent[pi1] = (ULONG *) PvMalloc (1806*sizeof (ULONG));
                VInitSingle (rgprgulSinglePawnPresent[pi1], rgsHalfKings,
                                         rgcLegal[pi1], rgprgsqPiece[pi1], &rgcSinglePawnPresent[pi1]);
                if (pi1 > x_piecePawn)
                        {
                        rgprgulSinglePawnless[pi1] = (ULONG *) PvMalloc (462*sizeof (ULONG));
                        VInitSingle (rgprgulSinglePawnless[pi1], rgsTriKings,
                                                 rgcLegal[pi1], rgprgsqPiece[pi1], &rgcSinglePawnless[pi1]);
                        }
                // Initialize enumeration tables for pair of pieces
                for (pi2 = (x_pieceNone == pi1 ? x_pieceNone : x_piecePawn); pi2 <= pi1; pi2 = (piece) (pi2 + 1))
                        {
                        rgprgulPairPawnPresent[pi1][pi2] = (ULONG *) PvMalloc (1806*sizeof (ULONG));
                        VInitPair (rgprgulPairPawnPresent[pi1][pi2], rgsHalfKings,
                                           rgcLegal[pi1], rgprgsqPiece[pi1], rgcLegal[pi2], rgprgsqPiece[pi2],
                                           &rgcPairPawnPresent[pi1][pi2]);
                        if (pi1 > x_piecePawn && pi2 > x_piecePawn)
                                {
                                rgprgulPairPawnless[pi1][pi2] = (ULONG *) PvMalloc (462*sizeof (ULONG));
                                VInitPair (rgprgulPairPawnless[pi1][pi2], rgsTriKings,
                                                   rgcLegal[pi1], rgprgsqPiece[pi1], rgcLegal[pi2], rgprgsqPiece[pi2],
                                                   &rgcPairPawnless[pi1][pi2]);
                                }
#if defined (T41_INCLUDE)
                        // Initialize enumeration tables for three pieces
                        for (pi3 = (x_pieceNone == pi1 ? x_pieceNone : x_piecePawn); pi3 <= pi2; pi3 = (piece) (pi3 + 1))
                                {
                                if (pi1 <= x_piecePawn || pi2 <= x_piecePawn || pi3 <= x_piecePawn)
                                        {
                                        rgprgulTriplePawnPresent[pi1][pi2][pi3] = (ULONG *) PvMalloc (1806*sizeof (ULONG));
                                        VInitTriple (rgprgulTriplePawnPresent[pi1][pi2][pi3], rgsHalfKings,
                                                                 rgcLegal[pi1], rgprgsqPiece[pi1],
                                                                 rgcLegal[pi2], rgprgsqPiece[pi2],
                                                                 rgcLegal[pi3], rgprgsqPiece[pi3],
                                                                 &rgcTriplePawnPresent[pi1][pi2][pi3]);
                                        }
                                else
                                        {
                                        rgprgulTriplePawnless[pi1][pi2][pi3] = (ULONG *) PvMalloc (462*sizeof (ULONG));
                                        VInitTriple (rgprgulTriplePawnless[pi1][pi2][pi3], rgsTriKings,
                                                rgcLegal[pi1], rgprgsqPiece[pi1],
                                                rgcLegal[pi2], rgprgsqPiece[pi2],
                                                rgcLegal[pi3], rgprgsqPiece[pi3],
                                                &rgcTriplePawnless[pi1][pi2][pi3]);
                                        }
                                }
#endif
                        }
                }

        // All done!
        if (fPrint)
                {
                for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
                        printf ("%c - %ld enumerated positions\n", "pPNBRQ"[pi1], rgcSinglePawnPresent[pi1]);
                for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
                        {
                        if (0 != rgcSinglePawnless[pi1])
                                printf ("pawnless %c - %ld enumerated positions\n", "pPNBRQ"[pi1], rgcSinglePawnless[pi1]);
                        }
                for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
                        for (pi2 = x_pieceNone; pi2 <= pi1; pi2 = (piece) (pi2 + 1))
                                {
                                if (0 != rgcPairPawnPresent[pi1][pi2])
                                        printf ("%c%c - %ld enumerated positions\n", "pPNBRQ"[pi1], "pPNBRQ"[pi2],
                                                        rgcPairPawnPresent[pi1][pi2]);
                                }
                for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
                        for (pi2 = x_pieceNone; pi2 <= pi1; pi2 = (piece) (pi2 + 1))
                                {
                                if (0 != rgcPairPawnless[pi1][pi2])
                                        printf ("pawnless %c%c - %ld enumerated positions\n", "pPNBRQ"[pi1], "pPNBRQ"[pi2],
                                                        rgcPairPawnless[pi1][pi2]);
                                }
#if defined (T41_INCLUDE)
                for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
                        for (pi2 = x_pieceNone; pi2 <= pi1; pi2 = (piece) (pi2 + 1))
                                for (pi3 = x_pieceNone; pi3 <= pi2; pi3 = (piece) (pi3 + 1))
                                        {
                                        if (0 != rgcTriplePawnPresent[pi1][pi2][pi3])
                                                printf ("%c%c%c - %ld enumerated positions\n",
                                                                "pPNBRQ"[pi1], "pPNBRQ"[pi2], "pPNBRQ"[pi3],
                                                                rgcTriplePawnPresent[pi1][pi2][pi3]);
                                        }
                for (pi1 = x_pieceNone; pi1 <= x_pieceQueen; pi1 = (piece) (pi1 + 1))
                        for (pi2 = x_pieceNone; pi2 <= pi1; pi2 = (piece) (pi2 + 1))
                                for (pi3 = x_pieceNone; pi3 <= pi2; pi3 = (piece) (pi3 + 1))
                                        {
                                        if (0 != rgcTriplePawnless[pi1][pi2][pi3])
                                                printf ("pawnless %c%c%c - %ld enumerated positions\n",
                                                                "pPNBRQ"[pi1], "pPNBRQ"[pi2], "pPNBRQ"[pi3],
                                                                rgcTriplePawnless[pi1][pi2][pi3]);
                                        }
#endif
                printf ("\nAllocated %dk\n\n", (cbAllocated + 1023)/1024);
                }
        }

// Return enumeration of 2 kings and single piece

/* mob_pach */
#ifndef  __cplusplus
        static INLINE INDEX TB_FASTCALL TEnumerate1_Index
                (
      int piw1,
      bool fPawns,
      bool fInvert,
                square  sqwk,
                square  sqw1,
                square  sqbk
                )
#else
template <int piw1, bool fPawns, bool fInvert> class TEnumerate1
        {
public:
        static INLINE INDEX TB_FASTCALL Index
                (
                square  sqwk,
                square  sqw1,
                square  sqbk
                )
#endif
                {
                INDEX   ind;
                ULONG   ulKings;

                // For black pawn invert the board
                if (piw1 <= x_piecePawn && fInvert)
                        {
                        sqwk = reflect_x(sqwk);
                        sqw1 = reflect_x(sqw1);
                        sqbk = reflect_x(sqbk);
                        }

                // Get enumerated square
                ind = rgprgsqPiece[piw1][sqbk*64+sqw1];
#if defined (ILLEGAL_POSSIBLE)
                if ((BYTE) -1 == ind)
                        return INF_SINGLE;
#endif
                // Get enumerated position of both kings
                if (fPawns)
                        ulKings = rgsHalfKings[sqwk*64+sqbk];   // 0..1805
                else
                        ulKings = rgsTriKings[sqwk*64+sqbk];    // 0..461
#if defined (ILLEGAL_POSSIBLE)
                if (INF == ulKings)
                        return INF_SINGLE;
#endif
                // Can we remove one extra square?
                if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                        ind -= (sqw1 > sqwk);
                // Add enumerated square to the # of the preceeding positions
                return ind + (fPawns ? rgprgulSinglePawnPresent[piw1][ulKings] : rgprgulSinglePawnless[piw1][ulKings]);
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

// Return enumeration of 2 kings and 2 pieces

/* mob_pach */
#ifndef  __cplusplus
        static INLINE INDEX TB_FASTCALL TEnumerate2_Index
                (
                int piw1,
      int piw2,
      bool fPawns,
      bool fInvert,
      square    sqwk,
                square  sqw1,
                square  sqw2,
                square  sqbk
                )
#else
template <int piw1, int piw2, bool fPawns, bool fInvert> class TEnumerate2
        {
public:
        static INLINE INDEX TB_FASTCALL Index
                (
                square  sqwk,
                square  sqw1,
                square  sqw2,
                square  sqbk
                )
#endif
                {
                INDEX   ind1, ind2, cInd2;
                ULONG   ulKings;

                // For black pawn invert the board
                if (piw2 <= x_piecePawn && fInvert)
                        {
                        sqwk = reflect_x(sqwk);
                        sqw1 = reflect_x(sqw1);
                        sqw2 = reflect_x(sqw2);
                        sqbk = reflect_x(sqbk);
                        }

                // Get enumerated squares for both pieces
                if (piw1 == piw2)
                        SORT (sqw1, sqw2);
                ind1 = rgprgsqPiece[piw1][sqbk*64+sqw1];
                ind2 = rgprgsqPiece[piw2][sqbk*64+sqw2];
#if defined (ILLEGAL_POSSIBLE)
                if ((BYTE) -1 == ind1 || (BYTE) -1 == ind2)
                        return INF_PAIR;
#endif
                // Get enumerated position of both kings
                if (fPawns)
                        ulKings = rgsHalfKings[sqwk*64+sqbk];   // 0..1805
                else
                        ulKings = rgsTriKings[sqwk*64+sqbk];    // 0..461
#if defined (ILLEGAL_POSSIBLE)
                if (INF == ulKings)
                        return INF_PAIR;
#endif
                if (piw1 == piw2)
                        {
                        // Can we remove one extra square?
                        if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                                {
                                ind1 -= (sqw1 > sqwk);
                                ind2 -= (sqw2 > sqwk);
                                }
                        // Add enumerated squares to the # of the preceeding positions
                        return  ind2*(ind2-1)/2 + ind1 +
                                        (fPawns ? rgprgulPairPawnPresent[piw1][piw2][ulKings] : rgprgulPairPawnless[piw1][piw2][ulKings]);
                        }
                else
                        {
                        // Can we remove WK square from 1st piece Enumeration?
                        if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                                ind1 -= (sqw1 > sqwk);
                        // Get # of enumerated positions of 2nd piece
                        cInd2 = rgcLegal[piw2][sqbk];
                        // Can we remove WK square from 2nd piece Enumeration?
                        if ((piw2>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw2][sqbk*64+sqwk]))
                                {
                                cInd2 --;
                                ind2 -= (sqw2 > sqwk);
                                }
                        // Add enumerated square to the # of the preceeding positions
                        return cInd2*ind1 + ind2 + (fPawns ? rgprgulPairPawnPresent[piw1][piw2][ulKings] : rgprgulPairPawnless[piw1][piw2][ulKings]);
                        }
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

#if defined (T41_INCLUDE)

// Return enumeration of 2 kings and 3 pieces

/* mob_pach */
#ifndef  __cplusplus
        static INLINE INDEX TB_FASTCALL TEnumerate3_Index
                (
                int piw1,
      int piw2,
      int piw3,
      bool fPawns,
      bool fInvert,
      square    sqwk,
                square  sqw1,
                square  sqw2,
                square  sqw3,
                square  sqbk
                )
#else
template <int piw1, int piw2, int piw3, bool fPawns, bool fInvert> class TEnumerate3
        {
public:
        static INLINE INDEX TB_FASTCALL Index
                (
                square  sqwk,
                square  sqw1,
                square  sqw2,
                square  sqw3,
                square  sqbk
                )
#endif
                {
                INDEX   ind1, ind2, ind3, cInd1, cInd2, cInd3;
                ULONG   ulKings;

                // For black pawn invert the board
                if (piw3 <= x_piecePawn && fInvert)
                        {
                        sqwk = reflect_x(sqwk);
                        sqw1 = reflect_x(sqw1);
                        sqw2 = reflect_x(sqw2);
                        sqw3 = reflect_x(sqw3);
                        sqbk = reflect_x(sqbk);
                        }

                // Get enumerated squares for all pieces
                if (piw1 == piw2 && piw1 == piw3)
                        {
                        SORT (sqw1, sqw2);
                        SORT (sqw2, sqw3);
                        SORT (sqw1, sqw2);
                        }
                else if (piw1 == piw2)
                        {
                        SORT (sqw1, sqw2);
                        }
                else if (piw2 == piw3)
                        {
                        SORT (sqw2, sqw3);
                        }
                ind1 = rgprgsqPiece[piw1][sqbk*64+sqw1];
                ind2 = rgprgsqPiece[piw2][sqbk*64+sqw2];
                ind3 = rgprgsqPiece[piw3][sqbk*64+sqw3];
#if defined (ILLEGAL_POSSIBLE)
                if ((BYTE) -1 == ind1 || (BYTE) -1 == ind2 || (BYTE) -1 == ind3)
                        return INF_TRIPLE;
#endif
                // Get enumerated position of both kings
                if (fPawns)
                        ulKings = rgsHalfKings[sqwk*64+sqbk];   // 0..1805
                else
                        ulKings = rgsTriKings[sqwk*64+sqbk];    // 0..461
#if defined (ILLEGAL_POSSIBLE)
                if (INF == ulKings)
                        return INF_TRIPLE;
#endif
                if (piw1 == piw2 && piw2 == piw3)
                        {
                        // Can we remove one extra square?
                        if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                                {
                                ind1 -= (sqw1 > sqwk);
                                ind2 -= (sqw2 > sqwk);
                                ind3 -= (sqw3 > sqwk);
                                }
                        // Add enumerated squares to the # of the preceeding positions
                        return  ind3*(ind3-1)*(ind3-2)/6 + ind2*(ind2-1)/2 + ind1 +
                                        (fPawns ? rgprgulTriplePawnPresent[piw1][piw2][piw3][ulKings] :
                                                          rgprgulTriplePawnless[piw1][piw2][piw3][ulKings]);
                        }
                else if (piw1 == piw2)
                        {
                        // Can we remove one extra square?
                        if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                                {
                                ind1 -= (sqw1 > sqwk);
                                ind2 -= (sqw2 > sqwk);
                                }
                        // Get # of enumerated positions of 3rd piece
                        cInd3 = rgcLegal[piw3][sqbk];
                        // Can we remove WK square from 3rd piece Enumeration?
                        if ((piw3>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw3][sqbk*64+sqwk]))
                                {
                                cInd3 --;
                                ind3 -= (sqw3 > sqwk);
                                }
                        // Add enumerated squares to the # of the preceeding positions
                        return  (ind2*(ind2-1)/2 + ind1)*cInd3 + ind3 +
                                        (fPawns ? rgprgulTriplePawnPresent[piw1][piw2][piw3][ulKings] :
                                                          rgprgulTriplePawnless[piw1][piw2][piw3][ulKings]);
                        }
                else if (piw2 == piw3)
                        {
                        // Can we remove one extra square?
                        if ((piw2>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw2][sqbk*64+sqwk]))
                                {
                                ind2 -= (sqw2 > sqwk);
                                ind3 -= (sqw3 > sqwk);
                                }
                        // Get # of enumerated positions of 1st piece
                        cInd1 = rgcLegal[piw1][sqbk];
                        // Can we remove WK square from 3rd piece Enumeration?
                        if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                                {
                                cInd1 --;
                                ind1 -= (sqw1 > sqwk);
                                }
                        // Add enumerated squares to the # of the preceeding positions
                        return  (ind3*(ind3-1)/2 + ind2)*cInd1 + ind1 +
                                        (fPawns ? rgprgulTriplePawnPresent[piw1][piw2][piw3][ulKings] :
                                                          rgprgulTriplePawnless[piw1][piw2][piw3][ulKings]);
                        }
                else
                        {
                        // Can we remove WK square from 1st piece Enumeration?
                        if ((piw1>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw1][sqbk*64+sqwk]))
                                ind1 -= (sqw1 > sqwk);
                        // Get # of enumerated positions of 2nd piece
                        cInd2 = rgcLegal[piw2][sqbk];
                        // Can we remove WK square from 2nd piece Enumeration?
                        if ((piw2>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw2][sqbk*64+sqwk]))
                                {
                                cInd2 --;
                                ind2 -= (sqw2 > sqwk);
                                }
                        // Get # of enumerated positions of 3rd piece
                        cInd3 = rgcLegal[piw3][sqbk];
                        // Can we remove WK square from 3rd piece Enumeration?
                        if ((piw3>x_pieceKnight) || ((BYTE)-1 != rgprgsqPiece[piw3][sqbk*64+sqwk]))
                                {
                                cInd3 --;
                                ind3 -= (sqw3 > sqwk);
                                }
                        // Add enumerated square to the # of the preceeding positions
                        return  cInd3*(cInd2*ind1 + ind2) + ind3 +
                                        (fPawns ? rgprgulTriplePawnPresent[piw1][piw2][piw3][ulKings] :
                                                          rgprgulTriplePawnless[piw1][piw2][piw3][ulKings]);
                        }
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

#endif

// Enumerate en passant captures

static INLINE INDEX TB_FASTCALL IndEnPassant11W
        (
        square  sqw,
        square  sqb,
        square  sqEnP
        )
        {
        assert (sqb+8 == sqEnP);
        if (sqw+7 == sqEnP)
                // Capture to the left
                return  (sqw&7)-1;
        else
                {
                // Capture to the right
                assert (sqw+9 == sqEnP);
                return  (sqw&7)+7;
                }
        }

static INLINE INDEX TB_FASTCALL IndEnPassant11B
        (
        square  sqw,
        square  sqb,
        square  sqEnP
        )
        {
        assert (sqw-8 == sqEnP);
        if (sqb-9 == sqEnP)
                // Capture to the left
                return (sqb&7)-1;
        else
                {
                // Capture to the right
                assert (sqb-7 == sqEnP);
                return (sqb&7)+7;
                }
        }

static INLINE INDEX TB_FASTCALL IndEnPassant21W
        (
        square  sqw1,
        square  sqw2,
        square  sqb,
        square  sqEnP
        )
        {
        assert (sqb+8 == sqEnP);
        SORT (sqw1, sqw2);
        if (sqw1+7 == sqEnP && 0 != TbColumn(sqw1))
                // Capture to the left
                return (sqw1&7)-1+(EXCLUDE3(sqw2,sqb,sqEnP,sqEnP+8)-i8-1)*i14;
        else if (sqw1+9 == sqEnP && 7 != TbColumn(sqw1))
                // Capture to the right
                return (sqw1&7)+7+(EXCLUDE3(sqw2,sqb,sqEnP,sqEnP+8)-i8-1)*i14;
        else if (sqw2+7 == sqEnP && 0 != TbColumn(sqw2))
                // Capture to the left
                return (sqw2&7)-1+(EXCLUDE3(sqw1,sqb,sqEnP,sqEnP+8)-i8)*i14;
        else
                {
                // Capture to the right
                assert (sqw2+9 == sqEnP && 7 != TbColumn(sqw2));
                return (sqw2&7)+7+(EXCLUDE3(sqw1,sqb,sqEnP,sqEnP+8)-i8)*i14;
                }
        }

static INLINE INDEX TB_FASTCALL IndEnPassant21B
        (
        square  sqw1,
        square  sqw2,
        square  sqb,
        square  sqEnP
        )
        {
        assert (sqw1 < sqw2);   // Must be already sorted
        if (sqb-9 == sqEnP && 0 != TbColumn(sqb))
                // Capture to the left
                if (sqw1-8 == sqEnP)
                        return (sqb&7)-1+(EXCLUDE3(sqw2,sqb,sqEnP,sqEnP-8)-i8-1)*i14;
                else
                        {
                        assert (sqw2-8 == sqEnP);
                        return (sqb&7)-1+(EXCLUDE3(sqw1,sqb,sqEnP,sqEnP-8)-i8)*i14;
                        }
        else
                {
                // Capture to the right
                assert (sqb-7 == sqEnP && 7 != TbColumn(sqb));
                if (sqw1-8 == sqEnP)
                        return (sqb&7)+7+(EXCLUDE3(sqw2,sqb,sqEnP,sqEnP-8)-i8-1)*i14;
                else
                        {
                        assert (sqw2-8 == sqEnP);
                        return (sqb&7)+7+(EXCLUDE3(sqw1,sqb,sqEnP,sqEnP-8)-i8)*i14;
                        }
                }
        }

// Index calculation functions for different endgame classes

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T21_IndCalcW
                (
                int piw1,
      square    *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
template <int piw1> class T21
        {
public:
        static INDEX TB_FASTCALL IndCalcW
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sqwk, sqw1, sqbk, sqMask;

                sqwk = SqFindKing (psqW);
                sqw1 = SqFindOne  (psqW, piw1);
                sqbk = SqFindKing (psqB);

                if (x_piecePawn == piw1)
                        sqMask = rgsqReflectMaskY [sqwk] ^ rgsqReflectInvertMask [fInvert];
                else
                        sqMask = rgsqReflectMaskYandX [sqwk];
                sqwk ^= sqMask;
                sqbk ^= sqMask;
                sqw1 ^= sqMask;

                if (x_piecePawn != piw1)
                        {
                        // No pawn
                        if (! FInTriangle (sqwk, sqbk))
                                {
                                sqwk = reflect_xy(sqwk);
                                sqbk = reflect_xy(sqbk);
                                sqw1 = reflect_xy(sqw1);
                                };
                        }
/* mob_pach */
#ifndef  __cplusplus
                return TEnumerate1_Index(piw1,x_piecePawn==piw1 ? true : false,false,sqwk,sqw1,sqbk);
#else
                return TEnumerate1<piw1,x_piecePawn==piw1 ? true : false,false>::Index(sqwk,sqw1,sqbk);
#endif
                }

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T21_IndCalcB
                (
                int piw1,
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
        static INDEX TB_FASTCALL IndCalcB
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sqwk, sqw1, sqbk, sqMask;

                sqwk = SqFindKing (psqW);
                sqw1 = SqFindOne  (psqW, piw1);
                sqbk = SqFindKing (psqB);

                if (x_piecePawn == piw1)
                        sqMask = rgsqReflectMaskY [sqbk] ^ rgsqReflectInvertMask [fInvert];
                else
                        sqMask = rgsqReflectMaskYandX [sqbk];
                sqwk ^= sqMask;
                sqbk ^= sqMask;
                sqw1 ^= sqMask;

                if (x_piecePawn == piw1)
/* mob_pach */
#ifndef  __cplusplus
                        return TEnumerate1_Index(x_pieceNone,true,true,sqbk,sqw1,sqwk);
#else
                        return TEnumerate1<x_pieceNone,true,true>::Index(sqbk,sqw1,sqwk);
#endif
                else
                        {
                        // No pawn
                        if (! FInTriangle (sqbk, sqwk))
                                {
                                sqwk = reflect_xy(sqwk);
                                sqbk = reflect_xy(sqbk);
                                sqw1 = reflect_xy(sqw1);
                                };
                        return IndTriKings(sqbk,sqwk)*i62 + EXCLUDE2(sqw1,sqwk,sqbk);
                        }
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T22_IndCalcW
                (
                int piw1,
      int pib1,
      square    *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
template <int piw1, int pib1> class T22
        {
public:
        static INDEX TB_FASTCALL IndCalcW
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sqwk, sqw1, sqbk, sqb1, sqMask;

                sqwk = SqFindKing (psqW);
                sqw1 = SqFindOne  (psqW, piw1);
                sqbk = SqFindKing (psqB);
                sqb1 = SqFindOne  (psqB, pib1);

                if (x_piecePawn == pib1)
                        sqMask = rgsqReflectMaskY [sqwk] ^ rgsqReflectInvertMask [fInvert];
                else
                        sqMask = rgsqReflectMaskYandX [sqwk];
                sqwk ^= sqMask;
                sqbk ^= sqMask;
                sqw1 ^= sqMask;
                sqb1 ^= sqMask;

                if (x_piecePawn == pib1)
                        {
                        // There are pawns on the board
                        if (x_piecePawn == piw1)
                                {
                                // One white and one black pawn
                                if (XX == sqEnP)
/* mob_pach */
#ifndef  __cplusplus
                                        return  TEnumerate1_Index(x_piecePawn,true,false,sqwk,sqw1,sqbk)*i47 +
                                                        EXCLUDE1(sqb1,sqw1)-i8; // 47
#else
                                        return  TEnumerate1<x_piecePawn,true,false>::Index(sqwk,sqw1,sqbk)*i47 +
                                                        EXCLUDE1(sqb1,sqw1)-i8; // 47
#endif
                                else
                                        return  rgcSinglePawnPresent[x_piecePawn]*i47 +
                                                        IndHalfKings(sqwk,sqbk)*i14 +
                                                        IndEnPassant11W (sqw1, sqb1, sqEnP ^ sqMask);
                                }
                        else
                                // Only black pawn
/* mob_pach */
#ifndef  __cplusplus
                                return  TEnumerate1_Index(piw1,true,false,sqwk,sqw1,sqbk)*i48 + sqb1-i8;
#else
                                return  TEnumerate1<piw1,true,false>::Index(sqwk,sqw1,sqbk)*i48 + sqb1-i8;
#endif
                        }
                else
                        {
                        // No pawns at all
                        if (!FInTriangle (sqwk, sqbk))
                                {
                                sqwk = reflect_xy(sqwk);
                                sqbk = reflect_xy(sqbk);
                                sqw1 = reflect_xy(sqw1);
                                sqb1 = reflect_xy(sqb1);
                                };
/* mob_pach */
#ifndef  __cplusplus
                        return  TEnumerate1_Index(piw1,false,false,sqwk,sqw1,sqbk)*i61 +
                                        EXCLUDE3(sqb1,sqwk,sqbk,sqw1);  // 61
#else
                        return  TEnumerate1<piw1,false,false>::Index(sqwk,sqw1,sqbk)*i61 +
                                        EXCLUDE3(sqb1,sqwk,sqbk,sqw1);  // 61
#endif
                        }
                }

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T22_IndCalcB
                (
                int piw1,
      int pib1,
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
        static INDEX TB_FASTCALL IndCalcB
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sqwk, sqw1, sqbk, sqb1, sqMask;

                sqwk = SqFindKing (psqW);
                sqw1 = SqFindOne  (psqW, piw1);
                sqbk = SqFindKing (psqB);
                sqb1 = SqFindOne  (psqB, pib1);

                if (x_piecePawn == pib1)
                        sqMask = rgsqReflectMaskY [sqbk] ^ rgsqReflectInvertMask [fInvert];
                else
                        sqMask = rgsqReflectMaskYandX [sqbk];
                sqwk ^= sqMask;
                sqbk ^= sqMask;
                sqw1 ^= sqMask;
                sqb1 ^= sqMask;

                if (x_piecePawn == pib1)
                        {
                        // There are pawns on the board
                        if (x_piecePawn == piw1)
                                {
                                // One white and one black pawn
                                if (XX == sqEnP)
/* mob_pach */
#ifndef  __cplusplus
                                        return  TEnumerate1_Index(x_piecePawn,true,true,sqbk,sqb1,sqwk)*i47 +
                                                        EXCLUDE1(sqw1,sqb1)-i8; // 47
#else
                                        return  TEnumerate1<x_piecePawn,true,true>::Index(sqbk,sqb1,sqwk)*i47 +
                                                        EXCLUDE1(sqw1,sqb1)-i8; // 47
#endif
                                else
                                        return  rgcSinglePawnPresent[x_piecePawn]*i47 +
                                                        IndHalfKings(sqbk,sqwk)*i14 +
                                                        IndEnPassant11B (sqw1, sqb1, sqEnP ^ sqMask);
                                }
                        }
                else
                        {
                        // No pawns at all
                        if (!FInTriangle (sqbk, sqwk))
                                {
                                sqwk = reflect_xy(sqwk);
                                sqbk = reflect_xy(sqbk);
                                sqw1 = reflect_xy(sqw1);
                                sqb1 = reflect_xy(sqb1);
                                };
                        }
/* mob_pach */
#ifndef  __cplusplus
                return  (x_piecePawn == pib1 ?  TEnumerate1_Index(pib1,true,true,sqbk,sqb1,sqwk) :
                                                                                TEnumerate1_Index(pib1,false,false,sqbk,sqb1,sqwk))*i61 +
                                EXCLUDE3(sqw1,sqwk,sqbk,sqb1);  // 61
#else
                return  (x_piecePawn == pib1 ?  TEnumerate1<pib1,true,true>::Index(sqbk,sqb1,sqwk) :
                                                                                TEnumerate1<pib1,false,false>::Index(sqbk,sqb1,sqwk))*i61 +
                                EXCLUDE3(sqw1,sqwk,sqbk,sqb1);  // 61
#endif
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T31_IndCalcW
                (
                int piw1,
      int piw2,
      square    *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
template <int piw1, int piw2> class T31
        {
public:
        static INDEX TB_FASTCALL IndCalcW
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sqwk, sqw1, sqw2, sqbk, sqMask;

                sqwk = SqFindKing (psqW);
                if (piw1 == piw2)
                        {
                        sqw1 = SqFindFirst  (psqW, piw1);
                        sqw2 = SqFindSecond (psqW, piw2);
                        }
                else
                        {
                        SqFind2 (psqW, piw1, sqw1, piw2, sqw2);
                        }
                sqbk = SqFindKing (psqB);

                if (x_piecePawn == piw2)
                        sqMask = rgsqReflectMaskY [sqwk] ^ rgsqReflectInvertMask [fInvert];
                else
                        sqMask = rgsqReflectMaskYandX [sqwk];
                sqwk ^= sqMask;
                sqbk ^= sqMask;
                sqw1 ^= sqMask;
                sqw2 ^= sqMask;

                if (x_piecePawn != piw2)
                        {
                        // There are no pawns on the board
                        if (!FInTriangle (sqwk, sqbk))
                                {
                                sqwk = reflect_xy(sqwk);
                                sqw1 = reflect_xy(sqw1);
                                sqw2 = reflect_xy(sqw2);
                                sqbk = reflect_xy(sqbk);
                                };
                        }
/* mob_pach */
#ifndef  __cplusplus
                return TEnumerate2_Index(piw1, piw2, x_piecePawn==piw2 ? true : false, false,sqwk, sqw1, sqw2, sqbk);
#else
                return TEnumerate2<piw1, piw2, x_piecePawn==piw2 ? true : false, false>::Index(sqwk, sqw1, sqw2, sqbk);
#endif
                }

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T31_IndCalcB
                (
                int piw1,
      int piw2,
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
        static INDEX TB_FASTCALL IndCalcB
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sqwk, sqw1, sqw2, sqbk, sqMask;

                sqwk = SqFindKing (psqW);
                if (piw1 == piw2)
                        {
                        sqw1 = SqFindFirst (psqW, piw1);
                        sqw2 = SqFindSecond (psqW, piw2);
                        }
                else
                        {
                        SqFind2 (psqW, piw1, sqw1, piw2, sqw2);
                        }
                sqbk = SqFindKing (psqB);

                if (x_piecePawn == piw2)
                        sqMask = rgsqReflectMaskY [sqbk] ^ rgsqReflectInvertMask [fInvert];
                else
                        sqMask = rgsqReflectMaskYandX [sqbk];
                sqwk ^= sqMask;
                sqbk ^= sqMask;
                sqw1 ^= sqMask;
                sqw2 ^= sqMask;

                if (x_piecePawn == piw2)
                        {
                        // There are pawns on the board
                        if (x_piecePawn == piw1)
                                // Two white pawns
/* mob_pach */
#ifndef  __cplusplus
                                return  TEnumerate2_Index(x_pieceNone,x_pieceNone,true,true,sqbk,sqw1,sqw2,sqwk);
#else
                                return  TEnumerate2<x_pieceNone,x_pieceNone,true,true>::Index(sqbk,sqw1,sqw2,sqwk);
#endif
                        else
                                // Only one white pawn
/* mob_pach */
#ifndef  __cplusplus
                                return  TEnumerate1_Index(x_pieceNone,true,true,sqbk,sqw2,sqwk)*i61 +
                                                EXCLUDE3(sqw1,sqwk,sqbk,sqw2);  // 61
#else
                                return  TEnumerate1<x_pieceNone,true,true>::Index(sqbk,sqw2,sqwk)*i61 +
                                                EXCLUDE3(sqw1,sqwk,sqbk,sqw2);  // 61
#endif
                        }
                else
                        {
                        // No pawns
                        if (!FInTriangle (sqbk, sqwk))
                                {
                                sqwk = reflect_xy(sqwk);
                                sqw1 = reflect_xy(sqw1);
                                sqw2 = reflect_xy(sqw2);
                                sqbk = reflect_xy(sqbk);
                                };
                        if (piw1 == piw2)
                                {
                                SORT (sqw1, sqw2);
                                sqw2 = EXCLUDE2(sqw2,sqwk,sqbk);                                        // 62
                                return  IndTriKings(sqbk,sqwk)*(i62*i61/2) +
                                                sqw2*(sqw2-1)/2+EXCLUDE2(sqw1,sqwk,sqbk);       // 62*61/2
                                }
                        else
                                return  IndTriKings(sqbk,sqwk)*(i62*i61) +
                                                EXCLUDE2(sqw1,sqwk,sqbk)*i61 +  // 62
                                                EXCLUDE3(sqw2,sqwk,sqbk,sqw1);  // 61
                        }
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T32_IndCalcW
                (
                int piw1,
      int piw2,
      int pib1,
      square    *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
template <int piw1, int piw2, int pib1> class T32
        {
public:
        static INDEX TB_FASTCALL IndCalcW
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sqwk, sqw1, sqw2, sqbk, sqb1, sqMask;

                sqwk = SqFindKing (psqW);
                if (piw1 == piw2)
                        {
                        sqw1 = SqFindFirst  (psqW, piw1);
                        sqw2 = SqFindSecond (psqW, piw2);
                        }
                else
                        {
                        SqFind2 (psqW, piw1, sqw1, piw2, sqw2);
                        }
                sqbk = SqFindKing (psqB);
                sqb1 = SqFindOne  (psqB, pib1);

                if (x_piecePawn == piw2 || x_piecePawn == pib1)
                        sqMask = rgsqReflectMaskY [sqwk] ^ rgsqReflectInvertMask [fInvert];
                else
                        sqMask = rgsqReflectMaskYandX [sqwk];
                sqwk ^= sqMask;
                sqbk ^= sqMask;
                sqw1 ^= sqMask;
                sqw2 ^= sqMask;
                sqb1 ^= sqMask;

                if (x_piecePawn == piw2 || x_piecePawn == pib1)
                        {
                        // There are pawns on the board
                        if (x_piecePawn == pib1)
                                {
                                // Black pawn
                                if (x_piecePawn == piw1 && x_piecePawn == piw2)
                                        {
                                        // All 3 pieces are pawns
                                        if (XX == sqEnP)
/* mob_pach */
#ifndef  __cplusplus
                                                return  TEnumerate2_Index(x_piecePawn,x_piecePawn,true,false,sqwk,sqw1,sqw2,sqbk)*i46+
                                                                EXCLUDE2(sqb1,sqw1,sqw2)-i8;    // 46
#else
                                                return  TEnumerate2<x_piecePawn,x_piecePawn,true,false>::
                                                                        Index(sqwk,sqw1,sqw2,sqbk)*i46+
                                                                EXCLUDE2(sqb1,sqw1,sqw2)-i8;    // 46
#endif
                                        else
                                                // En passant capture
                                                return  rgcPairPawnPresent[x_piecePawn][x_piecePawn]*i46 +
                                                                IndHalfKings(sqwk,sqbk)*(i14*i44) +
                                                                IndEnPassant21W (sqw1, sqw2, sqb1, sqEnP ^ sqMask);
                                        }
                                else if (x_piecePawn == piw2)
                                        {
                                        // One white pawn, one black pawn
                                        if (XX == sqEnP)
/* mob_pach */
#ifndef  __cplusplus
                                                return  TEnumerate2_Index(piw1,x_piecePawn,true,false,sqwk,sqw1,sqw2,sqbk)*i47 +
                                                                EXCLUDE1(sqb1,sqw2)-i8; // 47
#else
                                                return  TEnumerate2<piw1,x_piecePawn,true,false>::
                                                                        Index(sqwk,sqw1,sqw2,sqbk)*i47 +
                                                                EXCLUDE1(sqb1,sqw2)-i8; // 47
#endif
                                        else
                                                // En passant capture
                                                return  rgcPairPawnPresent[piw1][x_piecePawn]*i47 +
/* mob_pach */
#ifndef  __cplusplus
                                                                TEnumerate1_Index(piw1,true,false,sqwk,sqw1,sqbk)*i14 +
                                                                IndEnPassant11W (sqw2, sqb1, sqEnP ^ sqMask);
#else
                                                                TEnumerate1<piw1,true,false>::Index(sqwk,sqw1,sqbk)*i14 +
                                                                IndEnPassant11W (sqw2, sqb1, sqEnP ^ sqMask);
#endif
                                        }
                                else
                                        // Only black pawn
/* mob_pach */
#ifndef  __cplusplus
                                        return  TEnumerate2_Index(piw1,piw2,true,false,sqwk,sqw1,sqw2,sqbk)*i48 +
                                                        sqb1-i8;        // 48
#else
                                        return  TEnumerate2<piw1,piw2,true,false>::
                                                                Index(sqwk,sqw1,sqw2,sqbk)*i48 +
                                                        sqb1-i8;        // 48
#endif
                                }
                        }
                else
                        {
                        // No pawns
                        if (!FInTriangle (sqwk, sqbk))
                                {
                                sqwk = reflect_xy(sqwk);
                                sqw1 = reflect_xy(sqw1);
                                sqw2 = reflect_xy(sqw2);
                                sqbk = reflect_xy(sqbk);
                                sqb1 = reflect_xy(sqb1);
                                };
                        }
/* mob_pach */
#ifndef  __cplusplus
                return  TEnumerate2_Index(piw1,piw2,(x_piecePawn==piw2||x_piecePawn==pib1) ? true : false,false,sqwk,sqw1,sqw2,sqbk)*i60 +
                                EXCLUDE4(sqb1,sqwk,sqbk,sqw1,sqw2);     // 60
#else
                return  TEnumerate2<piw1,piw2,(x_piecePawn==piw2||x_piecePawn==pib1) ? true : false,false>::
                                        Index(sqwk,sqw1,sqw2,sqbk)*i60 +
                                EXCLUDE4(sqb1,sqwk,sqbk,sqw1,sqw2);     // 60
#endif
                }

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T32_IndCalcB
                (
                int piw1,
      int piw2,
      int pib1,
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
        static INDEX TB_FASTCALL IndCalcB
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sqwk, sqw1, sqw2, sqbk, sqb1, sqMask;

                sqwk = SqFindKing (psqW);
                if (piw1 == piw2)
                        {
                        sqw1 = SqFindFirst  (psqW, piw1);
                        sqw2 = SqFindSecond (psqW, piw2);
                        }
                else
                        {
                        SqFind2 (psqW, piw1, sqw1, piw2, sqw2);
                        }
                sqbk = SqFindKing (psqB);
                sqb1 = SqFindOne  (psqB, pib1);

                if (x_piecePawn == piw2 || x_piecePawn == pib1)
                        sqMask = rgsqReflectMaskY [sqbk] ^ rgsqReflectInvertMask [fInvert];
                else
                        sqMask = rgsqReflectMaskYandX [sqbk];
                sqwk ^= sqMask;
                sqbk ^= sqMask;
                sqw1 ^= sqMask;
                sqw2 ^= sqMask;
                sqb1 ^= sqMask;

                if (x_piecePawn == piw2 || x_piecePawn == pib1)
                        {
                        // There are pawns on the board
                        if (x_piecePawn == pib1)
                                {
                                // Black pawn
                                if (x_piecePawn == piw1 && x_piecePawn == piw2)
                                        {
                                        // All 3 pieces are pawns
                                        SORT (sqw1, sqw2);
                                        if (XX == sqEnP)
                                                {
                                                sqw2 = EXCLUDE1(sqw2,sqb1)-i8;                                  // 47
/* mob_pach */
#ifndef  __cplusplus
                                                return  TEnumerate1_Index(x_piecePawn,true,true,sqbk,sqb1,sqwk)*(i47*i46/2) +
                                                                sqw2*(sqw2-1)/2+EXCLUDE1(sqw1,sqb1)-i8; // 47*46/2
#else
                                                return  TEnumerate1<x_piecePawn,true,true>::
                                                                        Index(sqbk,sqb1,sqwk)*(i47*i46/2) +
                                                                sqw2*(sqw2-1)/2+EXCLUDE1(sqw1,sqb1)-i8; // 47*46/2
#endif
                                                }
                                        else
                                                // En passant capture
                                                return  rgcSinglePawnPresent[x_piecePawn]*(i47*i46/2) +
                                                                IndHalfKings(sqbk,sqwk)*(i44*i14) +
                                                                IndEnPassant21B (sqw1, sqw2, sqb1, sqEnP ^ sqMask);
                                        }
                                else if (x_piecePawn == piw2)
                                        {
                                        // One white pawn, one black pawn
                                        if (XX == sqEnP)
/* mob_pach */
#ifndef  __cplusplus
                                                return  TEnumerate1_Index(x_piecePawn,true,true,sqbk,sqb1,sqwk)*(i60*i47) +
                                                                EXCLUDE4(sqw1,sqwk,sqbk,sqw2,sqb1)*i47 +        // 60
                                                                EXCLUDE1(sqw2,sqb1)-i8;                                         // 47
#else
                                                return  TEnumerate1<x_piecePawn,true,true>::
                                                                        Index(sqbk,sqb1,sqwk)*(i60*i47) +
                                                                EXCLUDE4(sqw1,sqwk,sqbk,sqw2,sqb1)*i47 +        // 60
                                                                EXCLUDE1(sqw2,sqb1)-i8;                                         // 47
#endif
                                        else
                                                {
                                                // En passant capture
                                                sqEnP ^= sqMask;
                                                return  rgcSinglePawnPresent[x_piecePawn]*(i60*i47) +
                                                                IndHalfKings(sqbk,sqwk)*(i58*i14) +
                                                                EXCLUDE6(sqw1,sqwk,sqbk,sqw2,sqb1,sqEnP,sqEnP-8)*i14 +  // 58
                                                                IndEnPassant11B (sqw2, sqb1, sqEnP);
                                                }
                                        }
                                else
                                        {
                                        // Only black pawn
                                        if (piw1 == piw2)
                                                {
                                                // 2 identical white pieces
                                                SORT (sqw1, sqw2);
                                                sqw2 = EXCLUDE3(sqw2,sqwk,sqbk,sqb1);                                           // 61
/* mob_pach */
#ifndef  __cplusplus
                                                return  TEnumerate1_Index(x_piecePawn,true,true,sqbk,sqb1,sqwk)*(i61*i60/2) +
                                                                sqw2*(sqw2-1)/2 + EXCLUDE3(sqw1,sqwk,sqbk,sqb1);        // 61*60/2
#else
                                                return  TEnumerate1<x_piecePawn,true,true>::
                                                                        Index(sqbk,sqb1,sqwk)*(i61*i60/2) +
                                                                sqw2*(sqw2-1)/2 + EXCLUDE3(sqw1,sqwk,sqbk,sqb1);        // 61*60/2
#endif
                                                }
/* mob_pach */
#ifndef  __cplusplus
                                        return  TEnumerate1_Index(x_piecePawn,true,true,sqbk,sqb1,sqwk)*(i61*i60) +
                                                        EXCLUDE3(sqw1,sqwk,sqbk,sqb1)*i60 +     // 61
                                                        EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqb1);     // 60
#else
                                        return  TEnumerate1<x_piecePawn,true,true>::
                                                                Index(sqbk,sqb1,sqwk)*(i61*i60) +
                                                        EXCLUDE3(sqw1,sqwk,sqbk,sqb1)*i60 +     // 61
                                                        EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqb1);     // 60
#endif
                                        }
                                }
                        else
                                {
                                // No black pawn
                                if (x_piecePawn == piw1)
                                        {
                                        // Only 2 white pawns
                                        SORT (sqw1, sqw2);
                                        sqw2 -= i8;
/* mob_pach */
#ifndef  __cplusplus
                                        return  TEnumerate1_Index(pib1,true,true,sqbk,sqb1,sqwk)*(i48*47/2) +
                                                        sqw2*(sqw2-1)/2+sqw1-i8;                        // 48*47/2
#else
                                        return  TEnumerate1<pib1,true,true>::
                                                                Index(sqbk,sqb1,sqwk)*(i48*47/2) +
                                                        sqw2*(sqw2-1)/2+sqw1-i8;                        // 48*47/2
#endif
                                        }
                                else
                                        // Only one white pawn
/* mob_pach */
#ifndef  __cplusplus
                                        return  TEnumerate1_Index(pib1,true,true,sqbk,sqb1,sqwk)*(i60*i48) +
                                                        EXCLUDE4(sqw1,sqwk,sqbk,sqw2,sqb1)*i48 +        // 60
                                                        sqw2-i8;                                                                        // 48
#else
                                        return  TEnumerate1<pib1,true,true>::
                                                                Index(sqbk,sqb1,sqwk)*(i60*i48) +
                                                        EXCLUDE4(sqw1,sqwk,sqbk,sqw2,sqb1)*i48 +        // 60
                                                        sqw2-i8;                                                                        // 48
#endif
                                }
                        }
                else
                        {
                        // No pawns
                        if (!FInTriangle (sqbk, sqwk))
                                {
                                sqwk = reflect_xy(sqwk);
                                sqw1 = reflect_xy(sqw1);
                                sqw2 = reflect_xy(sqw2);
                                sqbk = reflect_xy(sqbk);
                                sqb1 = reflect_xy(sqb1);
                                };
                        if (piw1 == piw2)
                                {
                                // 2 identical white pieces
                                SORT (sqw1, sqw2);
                                sqw2 = EXCLUDE3(sqw2,sqwk,sqbk,sqb1);                                   // 61
/* mob_pach */
#ifndef  __cplusplus
                                return  TEnumerate1_Index(pib1,false,false,sqbk,sqb1,sqwk)*(i61*i60/2) +
                                                sqw2*(sqw2-1)/2+EXCLUDE3(sqw1,sqwk,sqbk,sqb1);  // 61*60/2
#else
                                return  TEnumerate1<pib1,false,false>::
                                                        Index(sqbk,sqb1,sqwk)*(i61*i60/2) +
                                                sqw2*(sqw2-1)/2+EXCLUDE3(sqw1,sqwk,sqbk,sqb1);  // 61*60/2
#endif
                                }
                        else
/* mob_pach */
#ifndef  __cplusplus
                                return  TEnumerate1_Index(pib1,false,false,sqbk,sqb1,sqwk)*(i61*i60) +
                                                EXCLUDE3(sqw1,sqwk,sqbk,sqb1)*i60 +     // 61
                                                EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqb1);     // 60
#else
                                return  TEnumerate1<pib1,false,false>::
                                                        Index(sqbk,sqb1,sqwk)*(i61*i60) +
                                                EXCLUDE3(sqw1,sqwk,sqbk,sqb1)*i60 +     // 61
                                                EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqb1);     // 60
#endif
                        }
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

#if defined (T41_INCLUDE)

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T41_IndCalcW
                (
                int piw1,
      int piw2,
      int piw3,
      square    *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
template <int piw1, int piw2, int piw3> class T41
        {
public:
        static INDEX TB_FASTCALL IndCalcW
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sqwk, sqw1, sqw2, sqw3, sqbk, sqMask;

                sqwk = SqFindKing (psqW);
                sqw1 = SqFindFirst (psqW, piw1);
                if (piw1 == piw2 && piw2 == piw3)
                        {
                        sqw2 = SqFindSecond (psqW, piw2);
                        sqw3 = SqFindThird (psqW, piw3);
                        }
                else if (piw1 == piw2)
                        {
                        sqw2 = SqFindSecond (psqW, piw2);
                        sqw3 = SqFindFirst (psqW, piw3);
                        }
                else if (piw2 == piw3)
                        {
                        sqw2 = SqFindFirst (psqW, piw2);
                        sqw3 = SqFindSecond (psqW, piw3);
                        }
                else
                        {
                        sqw2 = SqFindFirst (psqW, piw2);
                        sqw3 = SqFindFirst (psqW, piw3);
                        }
                sqbk = SqFindKing (psqB);

                if (x_piecePawn == piw3)
                        sqMask = rgsqReflectMaskY [sqwk] ^ rgsqReflectInvertMask [fInvert];
                else
                        sqMask = rgsqReflectMaskYandX [sqwk];
                sqwk ^= sqMask;
                sqbk ^= sqMask;
                sqw1 ^= sqMask;
                sqw2 ^= sqMask;
                sqw3 ^= sqMask;

                if (x_piecePawn != piw3)
                        {
                        // No pawns
                        if (!FInTriangle (sqwk, sqbk))
                                {
                                sqwk = reflect_xy(sqwk);
                                sqw1 = reflect_xy(sqw1);
                                sqw2 = reflect_xy(sqw2);
                                sqw3 = reflect_xy(sqw3);
                                sqbk = reflect_xy(sqbk);
                                };
                        }
/* mob_pach */
#ifndef  __cplusplus
                return  TEnumerate3_Index(piw1,piw2,piw3,x_piecePawn == piw3,false,sqwk,sqw1,sqw2,sqw3,sqbk);
#else
                return  TEnumerate3<piw1,piw2,piw3,x_piecePawn == piw3,false>::Index(sqwk,sqw1,sqw2,sqw3,sqbk);
#endif
                }

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T41_IndCalcB
                (
                int piw1,
      int piw2,
      int piw3,
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
        static INDEX TB_FASTCALL IndCalcB
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sqwk, sqw1, sqw2, sqw3, sqbk, sqMask;

                sqwk = SqFindKing (psqW);
                sqw1 = SqFindFirst (psqW, piw1);
                if (piw1 == piw2 && piw2 == piw3)
                        {
                        sqw2 = SqFindSecond (psqW, piw2);
                        sqw3 = SqFindThird (psqW, piw3);
                        }
                else if (piw1 == piw2)
                        {
                        sqw2 = SqFindSecond (psqW, piw2);
                        sqw3 = SqFindFirst (psqW, piw3);
                        }
                else if (piw2 == piw3)
                        {
                        sqw2 = SqFindFirst (psqW, piw2);
                        sqw3 = SqFindSecond (psqW, piw3);
                        }
                else
                        {
                        sqw2 = SqFindFirst (psqW, piw2);
                        sqw3 = SqFindFirst (psqW, piw3);
                        }
                sqbk = SqFindKing (psqB);

                if (x_piecePawn == piw3)
                        sqMask = rgsqReflectMaskY [sqbk] ^ rgsqReflectInvertMask [fInvert];
                else
                        sqMask = rgsqReflectMaskYandX [sqbk];
                sqwk ^= sqMask;
                sqbk ^= sqMask;
                sqw1 ^= sqMask;
                sqw2 ^= sqMask;
                sqw3 ^= sqMask;

                if (x_piecePawn == piw3)
                        {
                        // There are pawns on the board
                        if (x_piecePawn == piw1)
                                // 3 white pawns
/* mob_pach */
#ifndef  __cplusplus
                                return  TEnumerate3_Index(x_pieceNone,x_pieceNone,x_pieceNone,true,true,sqbk,sqw1,sqw2,sqw3,sqwk);
#else
                                return  TEnumerate3<x_pieceNone,x_pieceNone,x_pieceNone,true,true>::
                                                        Index(sqbk,sqw1,sqw2,sqw3,sqwk);
#endif
                        else if (x_piecePawn == piw2)
                                // 2 white pawns
/* mob_pach */
#ifndef  __cplusplus
                                return  TEnumerate2_Index(x_pieceNone,x_pieceNone,true,true,sqbk,sqw2,sqw3,sqwk)*i60 +
                                                EXCLUDE4(sqw1,sqwk,sqbk,sqw2,sqw3);     // 60
#else
                                return  TEnumerate2<x_pieceNone,x_pieceNone,true,true>::
                                                        Index(sqbk,sqw2,sqw3,sqwk)*i60 +
                                                EXCLUDE4(sqw1,sqwk,sqbk,sqw2,sqw3);     // 60
#endif
                        else if (piw1 == piw2)
                                {
                                // 1 pawn, 2 pieces equal
                                SORT (sqw1, sqw2);
                                sqw2 = EXCLUDE3(sqw2,sqwk,sqbk,sqw3);                                   // 61
/* mob_pach */
#ifndef  __cplusplus
                                return  TEnumerate1_Index(x_pieceNone,true,true,sqbk,sqw3,sqwk)*(i61*i60/2) +
                                                sqw2*(sqw2-1)/2+EXCLUDE3(sqw1,sqwk,sqbk,sqw3);  // 61*60/2
#else
                                return  TEnumerate1<x_pieceNone,true,true>::
                                                        Index(sqbk,sqw3,sqwk)*(i61*i60/2) +
                                                sqw2*(sqw2-1)/2+EXCLUDE3(sqw1,sqwk,sqbk,sqw3);  // 61*60/2
#endif
                                }
                        else
                                // Only one white pawn
/* mob_pach */
#ifndef  __cplusplus
                                return  TEnumerate1_Index(x_pieceNone,true,true,sqbk,sqw3,sqwk)*i61*i60 +
                                                EXCLUDE3(sqw1,sqwk,sqbk,sqw3)*i60 +     // 61
                                                EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqw3);     // 60
#else
                                return  TEnumerate1<x_pieceNone,true,true>::Index(sqbk,sqw3,sqwk)*i61*i60 +
                                                EXCLUDE3(sqw1,sqwk,sqbk,sqw3)*i60 +     // 61
                                                EXCLUDE4(sqw2,sqwk,sqbk,sqw1,sqw3);     // 60
#endif
                        }
                else
                        {
                        // No pawns
                        if (!FInTriangle (sqbk, sqwk))
                                {
                                sqwk = reflect_xy(sqwk);
                                sqw1 = reflect_xy(sqw1);
                                sqw2 = reflect_xy(sqw2);
                                sqw3 = reflect_xy(sqw3);
                                sqbk = reflect_xy(sqbk);
                                };
                        if (piw1 == piw2 && piw2 == piw3)
                                {
                                // All 3 pieces equal
                                SORT (sqw1, sqw2);
                                SORT (sqw2, sqw3);
                                SORT (sqw1, sqw2);
                                sqw3 = EXCLUDE2(sqw3,sqwk,sqbk);        // 62
                                sqw2 = EXCLUDE2(sqw2,sqwk,sqbk);
                                return  IndTriKings(sqbk,sqwk)*(i62*i61*i60/6) +
                                                sqw3*(sqw3-1)*(sqw3-2)/6+
                                                sqw2*(sqw2-1)/2+
                                                EXCLUDE2(sqw1,sqwk,sqbk);       // 62*61*60/6
                                }
                        else if (piw1 == piw2)
                                {
                                // 2 major pieces equal
                                SORT (sqw1, sqw2);
                                sqw2 = EXCLUDE3(sqw2,sqwk,sqbk,sqw3);                                                   // 61
                                return  IndTriKings(sqbk,sqwk)*(i61*i60/2*i62) +
                                                (sqw2*(sqw2-1)/2+EXCLUDE3(sqw1,sqwk,sqbk,sqw3))*i62 +   // 61*60/2
                                                EXCLUDE2(sqw3,sqwk,sqbk);                                                               // 62
                                }
                        else if (piw2 == piw3)
                                {
                                // 2 minor pieces equal
                                SORT (sqw2, sqw3);
                                sqw3 = EXCLUDE3(sqw3,sqwk,sqbk,sqw1);                                   // 61
                                return  IndTriKings(sqbk,sqwk)*(i62*i61*i60/2) +
                                                EXCLUDE2(sqw1,sqwk,sqbk)*(i61*i60/2) +                  // 62
                                                sqw3*(sqw3-1)/2+EXCLUDE3(sqw2,sqwk,sqbk,sqw1);  // 61*60/2
                                }
                        else
                                return  IndTriKings(sqbk,sqwk)*(i62*i61*i60) +
                                                EXCLUDE2(sqw1,sqwk,sqbk)*(i61*i60) +    // 62
                                                EXCLUDE3(sqw2,sqwk,sqbk,sqw1)*i60 +             // 61
                                                EXCLUDE4(sqw3,sqwk,sqbk,sqw1,sqw2);             // 60
                        }
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

#endif  // T41

#if defined (T33_INCLUDE)

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T33_IndCalcW
                (
                int piw1,
      int piw2,
      int pib1,
      int pib2,
      square    *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
template <int piw1, int piw2, int pib1, int pib2> class T33
        {
public:
        static INDEX TB_FASTCALL IndCalcW
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sqwk, sqw1, sqw2, sqbk, sqb1, sqb2, sqMask;
                INDEX ind;

                sqwk = SqFindKing (psqW);
                if (piw1 == piw2)
                        {
                        sqw1 = SqFindFirst  (psqW, piw1);
                        sqw2 = SqFindSecond (psqW, piw2);
                        }
                else
                        {
                        SqFind2 (psqW, piw1, sqw1, piw2, sqw2);
                        }
                sqbk = SqFindKing (psqB);
                if (pib1 == pib2)
                        {
                        sqb1 = SqFindFirst  (psqB, pib1);
                        sqb2 = SqFindSecond (psqB, pib2);
                        }
                else
                        {
                        SqFind2 (psqB, pib1, sqb1, pib2, sqb2);
                        }

                assert (x_piecePawn != piw2 && x_piecePawn != pib2);
                sqMask = rgsqReflectMaskYandX [sqwk];
                sqwk ^= sqMask;
                sqbk ^= sqMask;
                sqw1 ^= sqMask;
                sqw2 ^= sqMask;
                sqb1 ^= sqMask;
                sqb2 ^= sqMask;

                if (!FInTriangle (sqwk, sqbk))
                        {
                        sqwk = reflect_xy(sqwk);
                        sqw1 = reflect_xy(sqw1);
                        sqw2 = reflect_xy(sqw2);
                        sqbk = reflect_xy(sqbk);
                        sqb1 = reflect_xy(sqb1);
                        sqb2 = reflect_xy(sqb2);
                        }
/* mob_pach */
#ifndef  __cplusplus
                ind = TEnumerate2_Index(piw1,piw2,false,false,sqwk,sqw1,sqw2,sqbk);
#else
                ind = TEnumerate2<piw1,piw2,false,false>::Index(sqwk,sqw1,sqw2,sqbk);
#endif
#if defined (ILLEGAL_POSSIBLE)
                if (INF_PAIR == ind)
                        return (INDEX) -1;
#endif
                if (pib1 == pib2)
                        {
                        SORT (sqb1, sqb2);
                        sqb2 = EXCLUDE4(sqb2,sqwk,sqbk,sqw1,sqw2);      // 60
                        return  ind*(i60*i59/2) +
                                        sqb2*(sqb2-1)/2+
                                        EXCLUDE4(sqb1,sqwk,sqbk,sqw1,sqw2);     // 60*59/2
                        }
                else
                        return  ind*(i60*i59) +
                                        EXCLUDE4(sqb1,sqwk,sqbk,sqw1,sqw2)*i59 +        // 60
                                        EXCLUDE5(sqb2,sqwk,sqbk,sqw1,sqw2,sqb1);        // 59
                }

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T33_IndCalcB
                (
                int piw1,
      int piw2,
      int pib1,
      int pib2,
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
        static INDEX TB_FASTCALL IndCalcB
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sqwk, sqw1, sqw2, sqbk, sqb1, sqb2, sqMask;
                INDEX ind;

                sqwk = SqFindKing (psqW);
                if (piw1 == piw2)
                        {
                        sqw1 = SqFindFirst  (psqW, piw1);
                        sqw2 = SqFindSecond (psqW, piw2);
                        }
                else
                        {
                        SqFind2 (psqW, piw1, sqw1, piw2, sqw2);
                        }
                sqbk = SqFindKing (psqB);
                if (pib1 == pib2)
                        {
                        sqb1 = SqFindFirst  (psqB, pib1);
                        sqb2 = SqFindSecond (psqB, pib2);
                        }
                else
                        {
                        SqFind2 (psqB, pib1, sqb1, pib2, sqb2);
                        }

                assert (x_piecePawn != piw2 && x_piecePawn != pib2);
                sqMask = rgsqReflectMaskYandX [sqbk];
                sqwk ^= sqMask;
                sqbk ^= sqMask;
                sqw1 ^= sqMask;
                sqw2 ^= sqMask;
                sqb1 ^= sqMask;
                sqb2 ^= sqMask;

                if (!FInTriangle (sqbk, sqwk))
                        {
                        sqwk = reflect_xy(sqwk);
                        sqw1 = reflect_xy(sqw1);
                        sqw2 = reflect_xy(sqw2);
                        sqbk = reflect_xy(sqbk);
                        sqb1 = reflect_xy(sqb1);
                        sqb2 = reflect_xy(sqb2);
                        }
/* mob_pach */
#ifndef  __cplusplus
                ind = TEnumerate2_Index(pib1,pib2,false,false,sqbk,sqb1,sqb2,sqwk);
#else
                ind = TEnumerate2<pib1,pib2,false,false>::Index(sqbk,sqb1,sqb2,sqwk);
#endif
#if defined (ILLEGAL_POSSIBLE)
                if (INF_PAIR == ind)
                        return (INDEX) -1;
#endif
                if (piw1 == piw2)
                        {
                        SORT (sqw1, sqw2);
                        sqw2 = EXCLUDE4(sqw2,sqbk,sqwk,sqb1,sqb2);      // 60
                        return  ind*(i60*i59/2) +
                                        sqw2*(sqw2-1)/2+
                                        EXCLUDE4(sqw1,sqbk,sqwk,sqb1,sqb2);     // 60*59/2
                        }
                else
                        return  ind*(i60*i59) +
                                        EXCLUDE4(sqw1,sqbk,sqwk,sqb1,sqb2)*i59 +        // 60
                                        EXCLUDE5(sqw2,sqbk,sqwk,sqb1,sqb2,sqw1);        // 59
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

#endif  // T33

#else   // Old SJE schema ------------------------------------------------------

/* scanning pattern: triangle encoding */

static const INDEX sptriv[] =
        {
         0,  1,  2,  3, -1, -1, -1, -1,
        -1,  4,  5,  6, -1, -1, -1, -1,
        -1, -1,  7,  8, -1, -1, -1, -1,
        -1, -1, -1,  9, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1,
        };

/* scanning pattern: queenside flank encoding */

static const INDEX spqsfv[] =
        {
         0,  1,  2,  3, -1, -1, -1, -1,
         4,  5,  6,  7, -1, -1, -1, -1,
         8,  9, 10, 11, -1, -1, -1, -1,
        12, 13, 14, 15, -1, -1, -1, -1,
        16, 17, 18, 19, -1, -1, -1, -1,
        20, 21, 22, 23, -1, -1, -1, -1,
        24, 25, 26, 27, -1, -1, -1, -1,
        28, 29, 30, 31, -1, -1, -1, -1,
        };

/*--> CalcIndex3A: calculate index, mode 3A */
INLINE INDEX CalcIndex3A
        (
        square sq0,
        square sq1,
        square sq2
        )
        {
        INDEX index;

        if (TbRow(sq2) > x_row_4)
                {
                sq0 = reflect_x(sq0);
                sq1 = reflect_x(sq1);
                sq2 = reflect_x(sq2);
                };

        if (TbColumn(sq2) > x_column_d)
                {
                sq0 = reflect_y(sq0);
                sq1 = reflect_y(sq1);
                sq2 = reflect_y(sq2);
                };

        if (TbRow(sq2) > TbColumn(sq2))
                {
                sq0 = reflect_xy(sq0);
                sq1 = reflect_xy(sq1);
                sq2 = reflect_xy(sq2);
                };

        index =
                sq0 +
                sq1 * i64 +
                sptriv [sq2] * i64 * i64;

        return (index);
        }

/*--> CalcIndex3B: calculate index, mode 3B */
INLINE INDEX CalcIndex3B
        (
        square sq0,
        square sq1,
        square sq2
        )
        {
        INDEX index;

        if (TbColumn(sq1) > x_column_d)
                {
                sq0 = reflect_y(sq0);
                sq1 = reflect_y(sq1);
                sq2 = reflect_y(sq2);
                };

        index =
                sq0 +
                spqsfv [sq1] * i64 +
                sq2 * (i64 / 2) * i64;

        return (index);
        }

/*--> CalcIndex4A: calculate index, mode 4A */
INLINE INDEX CalcIndex4A
        (
        square sq0,
        square sq1,
        square sq2,
        square sq3
        )
        {
        INDEX index;

        if (TbRow(sq3) > x_row_4)
                {
                sq0 = reflect_x(sq0);
                sq1 = reflect_x(sq1);
                sq2 = reflect_x(sq2);
                sq3 = reflect_x(sq3);
                };

        if (TbColumn(sq3) > x_column_d)
                {
                sq0 = reflect_y(sq0);
                sq1 = reflect_y(sq1);
                sq2 = reflect_y(sq2);
                sq3 = reflect_y(sq3);
                };

        if (TbRow(sq3) > TbColumn(sq3))
                {
                sq0 = reflect_xy(sq0);
                sq1 = reflect_xy(sq1);
                sq2 = reflect_xy(sq2);
                sq3 = reflect_xy(sq3);
                };

        index =
                sq0 +
                sq1 * i64 +
                sq2 * i64 * i64 +
                sptriv [sq3] * i64 * i64 * i64;

        return (index);
        }

/*--> CalcIndex4B: calculate index, mode 4B */
INLINE INDEX CalcIndex4B
        (
        square sq0,
        square sq1,
        square sq2,
        square sq3
        )
        {
        INDEX index;

        if (TbColumn(sq3) > x_column_d)
                {
                sq0 = reflect_y(sq0);
                sq1 = reflect_y(sq1);
                sq2 = reflect_y(sq2);
                sq3 = reflect_y(sq3);
                };

        index =
                sq0 +
                sq1 * i64 +
                sq2 * i64 * i64 +
                spqsfv [sq3] * i64 * i64 * i64;

        return (index);
        }

/*--> CalcIndex4C: calculate index, mode 4C */
INLINE INDEX CalcIndex4C
        (
        square sq0,
        square sq1,
        square sq2,
        square sq3
        )
        {
        INDEX index;

        if (TbColumn(sq2) > x_column_d)
                {
                sq0 = reflect_y(sq0);
                sq1 = reflect_y(sq1);
                sq2 = reflect_y(sq2);
                sq3 = reflect_y(sq3);
                };

        index =
                sq0 +
                sq1 * i64 +
                spqsfv [sq2] * i64 * i64 +
                sq3 * (i64 / 2) * i64 * i64;

        return (index);
        }

/*--> CalcIndex5A: calculate index, mode 5A */
INLINE INDEX CalcIndex5A
        (
        square sq0,
        square sq1,
        square sq2,
        square sq3,
        square sq4
        )
        {
        INDEX index;

        if (TbRow(sq4) > x_row_4)
                {
                sq0 = reflect_x(sq0);
                sq1 = reflect_x(sq1);
                sq2 = reflect_x(sq2);
                sq3 = reflect_x(sq3);
                sq4 = reflect_x(sq4);
                };

        if (TbColumn(sq4) > x_column_d)
                {
                sq0 = reflect_y(sq0);
                sq1 = reflect_y(sq1);
                sq2 = reflect_y(sq2);
                sq3 = reflect_y(sq3);
                sq4 = reflect_y(sq4);
                };

        if (TbRow(sq4) > TbColumn(sq4))
                {
                sq0 = reflect_xy(sq0);
                sq1 = reflect_xy(sq1);
                sq2 = reflect_xy(sq2);
                sq3 = reflect_xy(sq3);
                sq4 = reflect_xy(sq4);
                };

        index =
                sq0 +
                sq1 * i64 +
                sq2 * i64 * i64 +
                sq3 * i64 * i64 * i64 +
                sptriv [sq4] * i64 * i64 * i64 * i64;

        return (index);
        }

/*--> CalcIndex5B: calculate index, mode 5B */
INLINE INDEX CalcIndex5B
        (
        square sq0,
        square sq1,
        square sq2,
        square sq3,
        square sq4
        )
        {
        INDEX index;

        if (TbColumn(sq4) > x_column_d)
                {
                sq0 = reflect_y(sq0);
                sq1 = reflect_y(sq1);
                sq2 = reflect_y(sq2);
                sq3 = reflect_y(sq3);
                sq4 = reflect_y(sq4);
                };

        index =
                sq0 +
                sq1 * i64 +
                sq2 * i64 * i64 +
                sq3 * i64 * i64 * i64 +
                spqsfv [sq4] * i64 * i64 * i64 * i64;

        return (index);
        }

/*--> CalcIndex5C: calculate index, mode 5C */
INLINE INDEX CalcIndex5C
        (
        square sq0,
        square sq1,
        square sq2,
        square sq3,
        square sq4
        )
        {
        INDEX index;

        if (TbColumn(sq3) > x_column_d)
                {
                sq0 = reflect_y(sq0);
                sq1 = reflect_y(sq1);
                sq2 = reflect_y(sq2);
                sq3 = reflect_y(sq3);
                sq4 = reflect_y(sq4);
                };

        index =
                sq0 +
                sq1 * i64 +
                sq2 * i64 * i64 +
                spqsfv [sq3] * i64 * i64 * i64 +
                sq4 * (i64 / 2) * i64 * i64 * i64;

        return (index);
        }

/*--> CalcIndex5D: calculate index, mode 5D */
INLINE INDEX CalcIndex5D
        (
        square sq0,
        square sq1,
        square sq2,
        square sq3,
        square sq4
        )
        {
        INDEX index;

        if (TbColumn(sq2) > x_column_d)
                {
                sq0 = reflect_y(sq0);
                sq1 = reflect_y(sq1);
                sq2 = reflect_y(sq2);
                sq3 = reflect_y(sq3);
                sq4 = reflect_y(sq4);
                };

        index =
                sq0 +
                sq1 * i64 +
                spqsfv [sq2] * i64 * i64 +
                sq3 * (i64 / 2) * i64 * i64 +
                sq4 * (i64 / 2) * i64 * i64 * i64;

        return (index);
        }

// Calculate index - a lot of functions...

#define IndCalcW        IndCalc
#define IndCalcB        IndCalc

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T21_IndCalc
                (
                int pi,
      square    *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
template <int pi> class T21
        {
public:
        static INDEX TB_FASTCALL IndCalc
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sq0, sq1, sq2;

                sq0 = SqFindKing (psqW);
                sq1 = SqFindOne  (psqW, pi);
                sq2 = SqFindKing (psqB);

                if (x_piecePawn == pi)
                        {
                        if (fInvert)
                                {
                                sq0 = reflect_x (sq0);
                                sq1 = reflect_x (sq1);
                                sq2 = reflect_x (sq2);
                                }
                        return CalcIndex3B (sq0, sq1, sq2);
                        }
                else
                        return CalcIndex3A (sq0, sq1, sq2);
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T22_IndCalc
                (
                int pi1,
      int pi2,
      square    *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
template <int pi1, int pi2> class T22
        {
public:
        static INDEX TB_FASTCALL IndCalc
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sq0, sq1, sq2, sq3;

                sq0 = SqFindKing (psqW);
                sq1 = SqFindOne  (psqW, pi1);
                sq2 = SqFindKing (psqB);
                sq3 = SqFindOne  (psqB, pi2);

                if (x_piecePawn == pi1 || x_piecePawn == pi2)
                        {
                        if (fInvert)
                                {
                                sq0 = reflect_x (sq0);
                                sq1 = reflect_x (sq1);
                                sq2 = reflect_x (sq2);
                                sq3 = reflect_x (sq3);
                                }
                        return CalcIndex4B (sq0, sq1, sq2, sq3);
                        }
                else
                        return CalcIndex4A (sq0, sq1, sq2, sq3);
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T31_IndCalc
                (
                int pi1,
      int pi2,
      square    *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
template <int pi1, int pi2> class T31
        {
public:
        static INDEX TB_FASTCALL IndCalc
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sq0, sq1, sq2, sq3;

                sq0 = SqFindKing (psqW);
                sq1 = SqFindFirst (psqW, pi1);
                if (pi1 == pi2)
                        sq2 = SqFindSecond (psqW, pi2);
                else
                        sq2 = SqFindFirst (psqW, pi2);
                sq3 = SqFindKing (psqB);

                if (x_piecePawn == pi1 || x_piecePawn == pi2)
                        {
                        if (fInvert)
                                {
                                sq0 = reflect_x (sq0);
                                sq1 = reflect_x (sq1);
                                sq2 = reflect_x (sq2);
                                sq3 = reflect_x (sq3);
                                }
                        return CalcIndex4C (sq0, sq1, sq2, sq3);
                        }
                else
                        return CalcIndex4A (sq0, sq1, sq2, sq3);
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T32_IndCalc
                (
                int pi1,
      int pi2,
      int pi3,
      square    *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
template <int pi1, int pi2, int pi3> class T32
        {
public:
        static INDEX TB_FASTCALL IndCalc
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sq0, sq1, sq2, sq3, sq4;

                sq0 = SqFindKing (psqW);
                sq1 = SqFindFirst (psqW, pi1);
                if (pi1 == pi2)
                        sq2 = SqFindSecond (psqW, pi2);
                else
                        sq2 = SqFindFirst (psqW, pi2);
                sq3 = SqFindKing (psqB);
                sq4 = SqFindOne  (psqB, pi3);

                if (x_piecePawn == pi1 || x_piecePawn == pi2 || x_piecePawn == pi3)
                        {
                        if (fInvert)
                                {
                                sq0 = reflect_x (sq0);
                                sq1 = reflect_x (sq1);
                                sq2 = reflect_x (sq2);
                                sq3 = reflect_x (sq3);
                                sq4 = reflect_x (sq4);
                                }
                        if (x_piecePawn == pi3)
                                return CalcIndex5B (sq0, sq1, sq2, sq3, sq4);
                        else
                                return CalcIndex5D (sq0, sq1, sq2, sq3, sq4);
                        }
                else
                        return CalcIndex5A (sq0, sq1, sq2, sq3, sq4);
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

#if defined (T41_INCLUDE)

/* mob_pach */
#ifndef  __cplusplus
        static INDEX TB_FASTCALL T41_IndCalc
                (
                int pi1,
      int pi2,
      int pi3,
      square    *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#else
template <int pi1, int pi2, int pi3> class T41
        {
public:
        static INDEX TB_FASTCALL IndCalc
                (
                square  *psqW,
                square  *psqB,
                square  sqEnP,
                int             fInvert
                )
#endif
                {
                square sq0, sq1, sq2, sq3, sq4;

                sq0 = SqFindKing (psqW);
                sq1 = SqFindFirst (psqW, pi1);
                sq2 = SqFindFirst (psqW, pi2);
                sq3 = SqFindFirst (psqW, pi3);
                sq4 = SqFindKing (psqB);

                if (x_piecePawn == pi1 || x_piecePawn == pi2 || x_piecePawn == pi3)
                        {
                        // There are pawns on the board
                        if (fInvert)
                                {
                                sq0 = reflect_x (sq0);
                                sq1 = reflect_x (sq1);
                                sq2 = reflect_x (sq2);
                                sq3 = reflect_x (sq3);
                                sq4 = reflect_x (sq4);
                                }
                        return CalcIndex5C (sq0, sq1, sq2, sq3, sq4);
                        }
                else    // No pawns
                        return CalcIndex5A (sq0, sq1, sq2, sq3, sq4);
                }
/* mob_pach */
#ifdef  __cplusplus
        };
#endif

#endif

#endif  //----------------------------------------------------------------------

// All tablebases enumerated

#define tbid_kk      0
#define tbid_kpk     1
#define tbid_knk     2
#define tbid_kbk     3
#define tbid_krk     4
#define tbid_kqk     5
#define tbid_kpkp    6
#define tbid_knkp    7
#define tbid_knkn    8
#define tbid_kbkp    9
#define tbid_kbkn   10
#define tbid_kbkb   11
#define tbid_krkp   12
#define tbid_krkn   13
#define tbid_krkb   14
#define tbid_krkr   15
#define tbid_kqkp   16
#define tbid_kqkn   17
#define tbid_kqkb   18
#define tbid_kqkr   19
#define tbid_kqkq   20
#define tbid_kppk   21
#define tbid_knpk   22
#define tbid_knnk   23
#define tbid_kbpk   24
#define tbid_kbnk   25
#define tbid_kbbk   26
#define tbid_krpk   27
#define tbid_krnk   28
#define tbid_krbk   29
#define tbid_krrk   30
#define tbid_kqpk   31
#define tbid_kqnk   32
#define tbid_kqbk   33
#define tbid_kqrk   34
#define tbid_kqqk   35
#define tbid_kppkp  36
#define tbid_kppkn  37
#define tbid_kppkb  38
#define tbid_kppkr  39
#define tbid_kppkq  40
#define tbid_knpkp  41
#define tbid_knpkn  42
#define tbid_knpkb  43
#define tbid_knpkr  44
#define tbid_knpkq  45
#define tbid_knnkp  46
#define tbid_knnkn  47
#define tbid_knnkb  48
#define tbid_knnkr  49
#define tbid_knnkq  50
#define tbid_kbpkp  51
#define tbid_kbpkn  52
#define tbid_kbpkb  53
#define tbid_kbpkr  54
#define tbid_kbpkq  55
#define tbid_kbnkp  56
#define tbid_kbnkn  57
#define tbid_kbnkb  58
#define tbid_kbnkr  59
#define tbid_kbnkq  60
#define tbid_kbbkp  61
#define tbid_kbbkn  62
#define tbid_kbbkb  63
#define tbid_kbbkr  64
#define tbid_kbbkq  65
#define tbid_krpkp  66
#define tbid_krpkn  67
#define tbid_krpkb  68
#define tbid_krpkr  69
#define tbid_krpkq  70
#define tbid_krnkp  71
#define tbid_krnkn  72
#define tbid_krnkb  73
#define tbid_krnkr  74
#define tbid_krnkq  75
#define tbid_krbkp  76
#define tbid_krbkn  77
#define tbid_krbkb  78
#define tbid_krbkr  79
#define tbid_krbkq  80
#define tbid_krrkp  81
#define tbid_krrkn  82
#define tbid_krrkb  83
#define tbid_krrkr  84
#define tbid_krrkq  85
#define tbid_kqpkp  86
#define tbid_kqpkn  87
#define tbid_kqpkb  88
#define tbid_kqpkr  89
#define tbid_kqpkq  90
#define tbid_kqnkp  91
#define tbid_kqnkn  92
#define tbid_kqnkb  93
#define tbid_kqnkr  94
#define tbid_kqnkq  95
#define tbid_kqbkp  96
#define tbid_kqbkn  97
#define tbid_kqbkb  98
#define tbid_kqbkr  99
#define tbid_kqbkq 100
#define tbid_kqrkp 101
#define tbid_kqrkn 102
#define tbid_kqrkb 103
#define tbid_kqrkr 104
#define tbid_kqrkq 105
#define tbid_kqqkp 106
#define tbid_kqqkn 107
#define tbid_kqqkb 108
#define tbid_kqqkr 109
#define tbid_kqqkq 110

#if defined (T41_INCLUDE)
#define tbid_kpppk 111
#define tbid_knppk 112
#define tbid_knnpk 113
#define tbid_knnnk 114
#define tbid_kbppk 115
#define tbid_kbnpk 116
#define tbid_kbnnk 117
#define tbid_kbbpk 118
#define tbid_kbbnk 119
#define tbid_kbbbk 120
#define tbid_krppk 121
#define tbid_krnpk 122
#define tbid_krnnk 123
#define tbid_krbpk 124
#define tbid_krbnk 125
#define tbid_krbbk 126
#define tbid_krrpk 127
#define tbid_krrnk 128
#define tbid_krrbk 129
#define tbid_krrrk 130
#define tbid_kqppk 131
#define tbid_kqnpk 132
#define tbid_kqnnk 133
#define tbid_kqbpk 134
#define tbid_kqbnk 135
#define tbid_kqbbk 136
#define tbid_kqrpk 137
#define tbid_kqrnk 138
#define tbid_kqrbk 139
#define tbid_kqrrk 140
#define tbid_kqqpk 141
#define tbid_kqqnk 142
#define tbid_kqqbk 143
#define tbid_kqqrk 144
#define tbid_kqqqk 145
#endif

#if defined (T33_INCLUDE)
#if defined (T41_INCLUDE)
#define BASE_33 145
#else
#define BASE_33 110
#endif
#define tbid_knnknn (BASE_33 + 1)
#define tbid_kbnknn (BASE_33 + 2)
#define tbid_kbbknn (BASE_33 + 3)
#define tbid_kbbkbn (BASE_33 + 4)
#define tbid_kbbkbb (BASE_33 + 5)
#define tbid_krnknn (BASE_33 + 6)
#define tbid_krnkbb (BASE_33 + 7)
#define tbid_krbknn (BASE_33 + 8)
#define tbid_krbkbb (BASE_33 + 9)
#define tbid_krrknn (BASE_33 + 10)
#define tbid_krrkbn (BASE_33 + 11)
#define tbid_krrkbb (BASE_33 + 12)
#define tbid_krrkrn (BASE_33 + 13)
#define tbid_krrkrr (BASE_33 + 14)
#define tbid_kqnknn (BASE_33 + 15)
#define tbid_kqnkbb (BASE_33 + 16)
#define tbid_kqnkrr (BASE_33 + 17)
#define tbid_kqbknn (BASE_33 + 18)
#define tbid_kqbkbb (BASE_33 + 19)
#define tbid_kqbkrr (BASE_33 + 20)
#define tbid_kqrknn (BASE_33 + 21)
#define tbid_kqrkbb (BASE_33 + 22)
#define tbid_kqrkrr (BASE_33 + 23)
#define tbid_kqqknn (BASE_33 + 24)
#define tbid_kqqkbn (BASE_33 + 25)
#define tbid_kqqkbb (BASE_33 + 26)
#define tbid_kqqkrn (BASE_33 + 27)
#define tbid_kqqkrb (BASE_33 + 28)
#define tbid_kqqkrr (BASE_33 + 29)
#define tbid_kqqkqn (BASE_33 + 30)
#define tbid_kqqkqb (BASE_33 + 31)
#define tbid_kqqkqr (BASE_33 + 32)
#define tbid_kqqkqq (BASE_33 + 33)
#define C33     (tbid_kqqkqq - BASE_33)
#else
#define C33     0
#endif

#if defined (T41_INCLUDE)
#define cTb     (146 + C33)
#else
#define cTb     (111 + C33)
#endif

// Compression

#include "tbdecode.h"

#if !defined (CPUS)
#define CPUS    1
#endif

#if defined (SMP)
static  lock_t  lockDecode;
#endif
#define TB_CRC_CHECK 0
static int cCompressed = 0;
static decode_block *rgpdbDecodeBlocks[CPUS];

// Information about tablebases

#if defined (T33_INCLUDE)
#define MAX_TOTAL_PIECES                6       /* Maximum # of pieces on the board */
#else
#define MAX_TOTAL_PIECES                5       /* Maximum # of pieces on the board */
#endif
#define MAX_NON_KINGS                   (MAX_TOTAL_PIECES - 2)

#if !defined (TB_DIRECTORY_SIZE)
#define TB_DIRECTORY_SIZE               32      /* # of cache buckets */
#endif

#if !defined (PFNCALCINDEX_DECLARED)
typedef INDEX (TB_FASTCALL * PfnCalcIndex) (square *psqW, square *psqB,
                                                                                 square sqEnP, int fInverse);
#define PFNCALCINDEX_DECLARED
#endif

struct CTbCache;

typedef struct          // Hungarian: tbcb
        {
#if defined (SMP)
        lock_t                          m_lock;                 // Lock on this cache bucket list
#endif
/* mob_pach */
#ifndef  __cplusplus
        volatile struct CTbCache  *m_ptbcFirst; // Cached file chunks in LRU order
#else
        volatile CTbCache  *m_ptbcFirst;        // Cached file chunks in LRU order
#endif
        }
        CTbCacheBucket;

typedef struct          // Hungarian: tbd
        {
        int                             m_iTbId;
        int                             m_fSymmetric:1;
        int                             m_f16bit:1;
        PfnCalcIndex    m_rgpfnCalcIndex[2];
        char                    m_rgchName[MAX_TOTAL_PIECES+1];
        INDEX                   m_rgcbLength[2];
        char                    *m_rgpchFileName[2];
#if defined (SMP)
        lock_t                  m_rglockFiles[2];
#endif
        FILE                    *m_rgfpFiles[2];
        decode_info             *m_rgpdiDecodeInfo[2];
        CTbCacheBucket  *m_prgtbcbBuckets[2];   // Cached file chunks in LRU order
        BYTE                    *m_rgpbRead[2];
        }
        CTbDesc;

#define TB(name, fSym, f16bit, funW, funB, cbW, cbB)    { tbid_##name, fSym, f16bit, { funW, funB }, #name, { cbW, cbB } },

#define P       x_piecePawn
#define N       x_pieceKnight
#define B       x_pieceBishop
#define R       x_pieceRook
#define Q       x_pieceQueen

/* mob_pach */
#ifndef  __cplusplus
static INDEX TB_FASTCALL T21_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T21_IndCalcW(P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T21_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T21_IndCalcB(P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T21_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T21_IndCalcW(N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T21_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T21_IndCalcB(N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T21_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T21_IndCalcW(B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T21_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T21_IndCalcB(B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T21_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T21_IndCalcW(R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T21_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T21_IndCalcB(R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T21_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T21_IndCalcW(Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T21_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T21_IndCalcB(Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_P_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_P_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_N_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_N_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_B_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_B_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_B_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_B_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_R_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(R, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_R_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(R, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_R_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_R_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_R_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_R_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_R_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_R_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_Q_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(Q, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_Q_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(Q, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_Q_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(Q, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_Q_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(Q, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_Q_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(Q, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_Q_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(Q, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_Q_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(Q, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_Q_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(Q, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_Q_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcW(Q, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T22_Q_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T22_IndCalcB(Q, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_P_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_P_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_N_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_N_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_B_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_B_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_B_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_B_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_R_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(R, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_R_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(R, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_R_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_R_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_R_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_R_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_R_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_R_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_Q_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(Q, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_Q_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(Q, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_Q_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(Q, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_Q_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(Q, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_Q_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(Q, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_Q_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(Q, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_Q_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(Q, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_Q_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(Q, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_Q_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcW(Q, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T31_Q_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T31_IndCalcB(Q, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_P_P_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(P, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_P_P_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(P, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_P_P_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(P, P, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_P_P_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(P, P, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_P_P_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(P, P, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_P_P_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(P, P, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_P_P_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(P, P, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_P_P_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(P, P, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_P_P_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(P, P, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_P_P_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(P, P, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_P_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(N, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_P_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(N, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_P_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(N, P, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_P_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(N, P, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_P_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(N, P, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_P_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(N, P, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_P_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(N, P, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_P_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(N, P, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_P_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(N, P, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_P_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(N, P, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_N_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(N, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_N_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(N, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(N, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(N, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_N_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(N, N, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_N_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(N, N, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_N_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(N, N, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_N_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(N, N, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_N_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(N, N, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_N_N_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(N, N, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_P_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_P_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_P_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, P, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_P_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, P, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_P_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, P, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_P_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, P, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_P_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, P, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_P_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, P, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_P_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, P, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_P_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, P, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_N_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_N_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_N_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, N, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_N_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, N, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_N_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, N, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_N_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, N, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_N_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, N, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_N_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, N, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_B_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_B_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_B_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_B_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_B_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, B, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_B_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, B, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_B_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(B, B, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_B_B_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(B, B, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_P_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_P_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_P_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, P, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_P_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, P, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_P_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, P, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_P_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, P, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_P_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, P, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_P_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, P, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_P_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, P, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_P_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, P, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_N_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_N_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_N_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, N, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_N_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, N, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_N_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, N, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_N_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, N, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_N_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, N, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_N_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, N, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_B_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_B_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_B_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_B_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_B_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, B, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_B_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, B, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_B_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, B, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_B_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, B, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_R_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, R, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_R_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, R, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_R_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_R_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_R_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_R_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_R_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_R_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_R_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(R, R, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_R_R_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(R, R, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_P_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_P_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_P_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, P, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_P_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, P, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_P_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, P, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_P_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, P, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_P_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, P, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_P_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, P, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_P_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, P, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_P_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, P, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_N_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_N_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_N_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, N, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_N_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, N, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_N_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, N, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_N_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, N, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_N_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, N, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_N_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, N, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_B_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_B_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_B_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_B_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_B_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, B, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_B_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, B, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_B_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, B, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_B_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, B, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_R_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, R, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_R_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, R, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_R_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_R_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_R_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_R_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_R_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_R_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_R_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, R, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_R_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, R, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_Q_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, Q, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_Q_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, Q, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_Q_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, Q, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_Q_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, Q, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_Q_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, Q, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_Q_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, Q, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_Q_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, Q, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_Q_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, Q, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_Q_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcW(Q, Q, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T32_Q_Q_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T32_IndCalcB(Q, Q, Q, psqW, psqB, sqEnP, fInvert);
   }

#if defined (T41_INCLUDE)

static INDEX TB_FASTCALL T41_P_P_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(P, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_P_P_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(P, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_N_P_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(N, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_N_P_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(N, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_N_N_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(N, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_N_N_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(N, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_N_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(N, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_N_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(N, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_B_P_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(B, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_B_P_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(B, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_B_N_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(B, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_B_N_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(B, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_B_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(B, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_B_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(B, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_B_B_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(B, B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_B_B_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(B, B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_B_B_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(B, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_B_B_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(B, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_B_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(B, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_B_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(B, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_P_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(R, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_P_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(R, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_N_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(R, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_N_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(R, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(R, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(R, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_B_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(R, B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_B_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(R, B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_B_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(R, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_B_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(R, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(R, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(R, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_R_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(R, R, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_R_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(R, R, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_R_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(R, R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_R_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(R, R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_R_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(R, R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_R_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(R, R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_R_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(R, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_R_R_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(R, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_P_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_P_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, P, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_N_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_N_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, N, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_B_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_B_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, B, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_B_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_B_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_R_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, R, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_R_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, R, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_R_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_R_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_R_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_R_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_R_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_R_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_Q_P_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, Q, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_Q_P_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, Q, P, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_Q_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, Q, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_Q_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, Q, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_Q_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, Q, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_Q_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, Q, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_Q_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, Q, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_Q_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, Q, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_Q_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcW(Q, Q, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T41_Q_Q_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T41_IndCalcB(Q, Q, Q, psqW, psqB, sqEnP, fInvert);
   }

#endif

#if defined (T33_INCLUDE)

static INDEX TB_FASTCALL T33_N_N_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(N, N, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_N_N_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(N, N, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_B_N_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(B, N, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_B_N_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(B, N, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_B_B_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(B, B, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_B_B_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(B, B, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_B_B_B_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(B, B, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_B_B_B_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(B, B, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_B_B_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(B, B, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_B_B_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(B, B, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_N_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(R, N, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_N_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(R, N, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_N_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(R, N, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_N_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(R, N, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_B_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(R, B, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_B_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(R, B, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_B_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(R, B, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_B_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(R, B, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_R_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(R, R, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_R_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(R, R, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_R_B_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(R, R, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_R_B_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(R, R, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_R_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(R, R, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_R_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(R, R, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_R_R_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(R, R, R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_R_R_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(R, R, R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_R_R_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(R, R, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_R_R_R_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(R, R, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_N_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, N, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_N_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, N, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_N_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, N, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_N_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, N, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_N_R_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, N, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_N_R_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, N, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_B_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, B, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_B_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, B, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_B_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, B, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_B_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, B, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_B_R_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, B, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_B_R_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, B, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_R_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, R, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_R_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, R, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_R_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, R, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_R_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, R, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_R_R_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, R, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_R_R_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, R, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_N_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, Q, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_N_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, Q, N, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_B_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, Q, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_B_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, Q, B, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_B_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, Q, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_B_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, Q, B, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_R_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, Q, R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_R_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, Q, R, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_R_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, Q, R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_R_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, Q, R, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_R_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, Q, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_R_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, Q, R, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_Q_N_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, Q, Q, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_Q_N_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, Q, Q, N, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_Q_B_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, Q, Q, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_Q_B_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, Q, Q, B, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_Q_R_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, Q, Q, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_Q_R_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, Q, Q, R, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_Q_Q_IndCalcW
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcW(Q, Q, Q, Q, psqW, psqB, sqEnP, fInvert);
   }

static INDEX TB_FASTCALL T33_Q_Q_Q_Q_IndCalcB
   (
   square       *psqW,
   square       *psqB,
   square       sqEnP,
   int          fInvert
   )
   {
      return T33_IndCalcB(Q, Q, Q, Q, psqW, psqB, sqEnP, fInvert);
   }

#endif

CTbDesc rgtbdDesc[cTb] =
        {
        TB (kk, true, false, NULL, NULL, 0, 0)

        TB (kpk, false, false, (T21_P_IndCalcW), (T21_P_IndCalcB), 81664, 84012)
        TB (knk, false, false, (T21_N_IndCalcW), (T21_N_IndCalcB), 26282, 28644)
        TB (kbk, false, false, (T21_B_IndCalcW), (T21_B_IndCalcB), 27243, 28644)
        TB (krk, false, false, (T21_R_IndCalcW), (T21_R_IndCalcB), 27030, 28644)
        TB (kqk, false, false, (T21_Q_IndCalcW), (T21_Q_IndCalcB), 25629, 28644)

        TB (kpkp, false, false, (T22_P_P_IndCalcW), (T22_P_P_IndCalcB), 3863492, 3863492)
        TB (knkp, false, false, (T22_N_P_IndCalcW), (T22_N_P_IndCalcB), 4931904, 4981504)
        TB (knkn, true,  false, (T22_N_N_IndCalcW), (T22_N_N_IndCalcB), 1603202, 1603202)
        TB (kbkp, false, false, (T22_B_P_IndCalcW), (T22_B_P_IndCalcB), 5112000, 4981504)
        TB (kbkn, false, false, (T22_B_N_IndCalcW), (T22_B_N_IndCalcB), 1661823, 1603202)
        TB (kbkb, true,  false, (T22_B_B_IndCalcW), (T22_B_B_IndCalcB), 1661823, 1661823)
        TB (krkp, false, false, (T22_R_P_IndCalcW), (T22_R_P_IndCalcB), 5072736, 4981504)
        TB (krkn, false, false, (T22_R_N_IndCalcW), (T22_R_N_IndCalcB), 1649196, 1603202)
        TB (krkb, false, false, (T22_R_B_IndCalcW), (T22_R_B_IndCalcB), 1649196, 1661823)
        TB (krkr, true,  false, (T22_R_R_IndCalcW), (T22_R_R_IndCalcB), 1649196, 1649196)
        TB (kqkp, false, false, (T22_Q_P_IndCalcW), (T22_Q_P_IndCalcB), 4810080, 4981504)
        TB (kqkn, false, false, (T22_Q_N_IndCalcW), (T22_Q_N_IndCalcB), 1563735, 1603202)
        TB (kqkb, false, false, (T22_Q_B_IndCalcW), (T22_Q_B_IndCalcB), 1563735, 1661823)
        TB (kqkr, false, false, (T22_Q_R_IndCalcW), (T22_Q_R_IndCalcB), 1563735, 1649196)
        TB (kqkq, true,  false, (T22_Q_Q_IndCalcW), (T22_Q_Q_IndCalcB), 1563735, 1563735)

        TB (kppk, false, false, (T31_P_P_IndCalcW), (T31_P_P_IndCalcB), 1806671, 1912372)
        TB (knpk, false, false, (T31_N_P_IndCalcW), (T31_N_P_IndCalcB), 4648581, 5124732)
        TB (knnk, false, false, (T31_N_N_IndCalcW), (T31_N_N_IndCalcB),  735304,  873642)
        TB (kbpk, false, false, (T31_B_P_IndCalcW), (T31_B_P_IndCalcB), 4817128, 5124732)
        TB (kbnk, false, false, (T31_B_N_IndCalcW), (T31_B_N_IndCalcB), 1550620, 1747284)
        TB (kbbk, false, false, (T31_B_B_IndCalcW), (T31_B_B_IndCalcB),  789885,  873642)
        TB (krpk, false, false, (T31_R_P_IndCalcW), (T31_R_P_IndCalcB), 4779530, 5124732)
        TB (krnk, false, false, (T31_R_N_IndCalcW), (T31_R_N_IndCalcB), 1538479, 1747284)
        TB (krbk, false, false, (T31_R_B_IndCalcW), (T31_R_B_IndCalcB), 1594560, 1747284)
        TB (krrk, false, false, (T31_R_R_IndCalcW), (T31_R_R_IndCalcB),  777300,  873642)
        TB (kqpk, false, false, (T31_Q_P_IndCalcW), (T31_Q_P_IndCalcB), 4533490, 5124732)
        TB (kqnk, false, false, (T31_Q_N_IndCalcW), (T31_Q_N_IndCalcB), 1459616, 1747284)
        TB (kqbk, false, false, (T31_Q_B_IndCalcW), (T31_Q_B_IndCalcB), 1512507, 1747284)
        TB (kqrk, false, false, (T31_Q_R_IndCalcW), (T31_Q_R_IndCalcB), 1500276, 1747284)
        TB (kqqk, false, false, (T31_Q_Q_IndCalcW), (T31_Q_Q_IndCalcB),  698739,  873642)

#if !defined (KPPKP_16BIT)
        TB (kppkp, false, false, (T32_P_P_P_IndCalcW), (T32_P_P_P_IndCalcB),  84219361,  89391280)
#else
        TB (kppkp, false, true,  (T32_P_P_P_IndCalcW), (T32_P_P_P_IndCalcB),  84219361,  89391280)
#endif
        TB (kppkn, false, false, (T32_P_P_N_IndCalcW), (T32_P_P_N_IndCalcB), 108400260, 115899744)
        TB (kppkb, false, false, (T32_P_P_B_IndCalcW), (T32_P_P_B_IndCalcB), 108400260, 120132000)
        TB (kppkr, false, false, (T32_P_P_R_IndCalcW), (T32_P_P_R_IndCalcB), 108400260, 119209296)
        TB (kppkq, false, false, (T32_P_P_Q_IndCalcW), (T32_P_P_Q_IndCalcB), 108400260, 113036880)
        TB (knpkp, false, false, (T32_N_P_P_IndCalcW), (T32_N_P_P_IndCalcB), 219921779, 231758952)
        TB (knpkn, false, false, (T32_N_P_N_IndCalcW), (T32_N_P_N_IndCalcB), 278914860, 295914240)
        TB (knpkb, false, false, (T32_N_P_B_IndCalcW), (T32_N_P_B_IndCalcB), 278914860, 306720000)
        TB (knpkr, false, false, (T32_N_P_R_IndCalcW), (T32_N_P_R_IndCalcB), 278914860, 304369920)
        TB (knpkq, false, false, (T32_N_P_Q_IndCalcW), (T32_N_P_Q_IndCalcB), 278914860, 288610560)
        TB (knnkp, false, false, (T32_N_N_P_IndCalcW), (T32_N_N_P_IndCalcB), 137991648, 149445120)
        TB (knnkn, false, false, (T32_N_N_N_IndCalcW), (T32_N_N_N_IndCalcB),  44118240,  48096060)
        TB (knnkb, false, false, (T32_N_N_B_IndCalcW), (T32_N_N_B_IndCalcB),  44118240,  49854690)
        TB (knnkr, false, false, (T32_N_N_R_IndCalcW), (T32_N_N_R_IndCalcB),  44118240,  49475880)
        TB (knnkq, false, false, (T32_N_N_Q_IndCalcW), (T32_N_N_Q_IndCalcB),  44118240,  46912050)
        TB (kbpkp, false, false, (T32_B_P_P_IndCalcW), (T32_B_P_P_IndCalcB), 227896016, 231758952)
        TB (kbpkn, false, false, (T32_B_P_N_IndCalcW), (T32_B_P_N_IndCalcB), 289027680, 295914240)
        TB (kbpkb, false, false, (T32_B_P_B_IndCalcW), (T32_B_P_B_IndCalcB), 289027680, 306720000)
        TB (kbpkr, false, false, (T32_B_P_R_IndCalcW), (T32_B_P_R_IndCalcB), 289027680, 304369920)
        TB (kbpkq, false, false, (T32_B_P_Q_IndCalcW), (T32_B_P_Q_IndCalcB), 289027680, 288610560)
        TB (kbnkp, false, false, (T32_B_N_P_IndCalcW), (T32_B_N_P_IndCalcB), 290989584, 298890240)
        TB (kbnkn, false, false, (T32_B_N_N_IndCalcW), (T32_B_N_N_IndCalcB),  93037200,  96192120)
        TB (kbnkb, false, false, (T32_B_N_B_IndCalcW), (T32_B_N_B_IndCalcB),  93037200,  99709380)
        TB (kbnkr, false, false, (T32_B_N_R_IndCalcW), (T32_B_N_R_IndCalcB),  93037200,  98951760)
        TB (kbnkq, false, false, (T32_B_N_Q_IndCalcW), (T32_B_N_Q_IndCalcB),  93037200,  93824100)
        TB (kbbkp, false, false, (T32_B_B_P_IndCalcW), (T32_B_B_P_IndCalcB), 148223520, 149445120)
        TB (kbbkn, false, false, (T32_B_B_N_IndCalcW), (T32_B_B_N_IndCalcB),  47393100,  48096060)
        TB (kbbkb, false, false, (T32_B_B_B_IndCalcW), (T32_B_B_B_IndCalcB),  47393100,  49854690)
        TB (kbbkr, false, false, (T32_B_B_R_IndCalcW), (T32_B_B_R_IndCalcB),  47393100,  49475880)
        TB (kbbkq, false, false, (T32_B_B_Q_IndCalcW), (T32_B_B_Q_IndCalcB),  47393100,  46912050)
        TB (krpkp, false, false, (T32_R_P_P_IndCalcW), (T32_R_P_P_IndCalcB), 226121876, 231758952)
        TB (krpkn, false, false, (T32_R_P_N_IndCalcW), (T32_R_P_N_IndCalcB), 286777440, 295914240)
        TB (krpkb, false, false, (T32_R_P_B_IndCalcW), (T32_R_P_B_IndCalcB), 286777440, 306720000)
        TB (krpkr, false, false, (T32_R_P_R_IndCalcW), (T32_R_P_R_IndCalcB), 286777440, 304369920)
        TB (krpkq, false, false, (T32_R_P_Q_IndCalcW), (T32_R_P_Q_IndCalcB), 286777440, 288610560)
        TB (krnkp, false, false, (T32_R_N_P_IndCalcW), (T32_R_N_P_IndCalcB), 288692928, 298890240)
        TB (krnkn, false, false, (T32_R_N_N_IndCalcW), (T32_R_N_N_IndCalcB),  92308740,  96192120)
        TB (krnkb, false, false, (T32_R_N_B_IndCalcW), (T32_R_N_B_IndCalcB),  92308740,  99709380)
        TB (krnkr, false, false, (T32_R_N_R_IndCalcW), (T32_R_N_R_IndCalcB),  92308740,  98951760)
        TB (krnkq, false, false, (T32_R_N_Q_IndCalcW), (T32_R_N_Q_IndCalcB),  92308740,  93824100)
        TB (krbkp, false, false, (T32_R_B_P_IndCalcW), (T32_R_B_P_IndCalcB), 299203200, 298890240)
        TB (krbkn, false, false, (T32_R_B_N_IndCalcW), (T32_R_B_N_IndCalcB),  95673600,  96192120)
        TB (krbkb, false, false, (T32_R_B_B_IndCalcW), (T32_R_B_B_IndCalcB),  95673600,  99709380)
        TB (krbkr, false, false, (T32_R_B_R_IndCalcW), (T32_R_B_R_IndCalcB),  95673600,  98951760)
        TB (krbkq, false, false, (T32_R_B_Q_IndCalcW), (T32_R_B_Q_IndCalcB),  95673600,  93824100)
        TB (krrkp, false, false, (T32_R_R_P_IndCalcW), (T32_R_R_P_IndCalcB), 145901232, 149445120)
        TB (krrkn, false, false, (T32_R_R_N_IndCalcW), (T32_R_R_N_IndCalcB),  46658340,  48096060)
        TB (krrkb, false, false, (T32_R_R_B_IndCalcW), (T32_R_R_B_IndCalcB),  46658340,  49854690)
        TB (krrkr, false, false, (T32_R_R_R_IndCalcW), (T32_R_R_R_IndCalcB),  46658340,  49475880)
        TB (krrkq, false, false, (T32_R_R_Q_IndCalcW), (T32_R_R_Q_IndCalcB),  46658340,  46912050)
        TB (kqpkp, false, false, (T32_Q_P_P_IndCalcW), (T32_Q_P_P_IndCalcB), 214481388, 231758952)
        TB (kqpkn, false, false, (T32_Q_P_N_IndCalcW), (T32_Q_P_N_IndCalcB), 272015040, 295914240)
        TB (kqpkb, false, false, (T32_Q_P_B_IndCalcW), (T32_Q_P_B_IndCalcB), 272015040, 306720000)
        TB (kqpkr, false, false, (T32_Q_P_R_IndCalcW), (T32_Q_P_R_IndCalcB), 272015040, 304369920)
        TB (kqpkq, false, false, (T32_Q_P_Q_IndCalcW), (T32_Q_P_Q_IndCalcB), 272015040, 288610560)
        TB (kqnkp, false, false, (T32_Q_N_P_IndCalcW), (T32_Q_N_P_IndCalcB), 273904512, 298890240)
        TB (kqnkn, false, false, (T32_Q_N_N_IndCalcW), (T32_Q_N_N_IndCalcB),  87576960,  96192120)
        TB (kqnkb, false, false, (T32_Q_N_B_IndCalcW), (T32_Q_N_B_IndCalcB),  87576960,  99709380)
        TB (kqnkr, false, false, (T32_Q_N_R_IndCalcW), (T32_Q_N_R_IndCalcB),  87576960,  98951760)
        TB (kqnkq, false, false, (T32_Q_N_Q_IndCalcW), (T32_Q_N_Q_IndCalcB),  87576960,  93824100)
        TB (kqbkp, false, false, (T32_Q_B_P_IndCalcW), (T32_Q_B_P_IndCalcB), 283818240, 298890240)
        TB (kqbkn, false, false, (T32_Q_B_N_IndCalcW), (T32_Q_B_N_IndCalcB),  90750420,  96192120)
        TB (kqbkb, false, false, (T32_Q_B_B_IndCalcW), (T32_Q_B_B_IndCalcB),  90750420,  99709380)
        TB (kqbkr, false, false, (T32_Q_B_R_IndCalcW), (T32_Q_B_R_IndCalcB),  90750420,  98951760)
        TB (kqbkq, false, false, (T32_Q_B_Q_IndCalcW), (T32_Q_B_Q_IndCalcB),  90750420,  93824100)
        TB (kqrkp, false, false, (T32_Q_R_P_IndCalcW), (T32_Q_R_P_IndCalcB), 281568240, 298890240)
        TB (kqrkn, false, false, (T32_Q_R_N_IndCalcW), (T32_Q_R_N_IndCalcB),  90038460,  96192120)
        TB (kqrkb, false, false, (T32_Q_R_B_IndCalcW), (T32_Q_R_B_IndCalcB),  90038460,  99709380)
        TB (kqrkr, false, false, (T32_Q_R_R_IndCalcW), (T32_Q_R_R_IndCalcB),  90038460,  98951760)
        TB (kqrkq, false, false, (T32_Q_R_Q_IndCalcW), (T32_Q_R_Q_IndCalcB),  90038460,  93824100)
        TB (kqqkp, false, false, (T32_Q_Q_P_IndCalcW), (T32_Q_Q_P_IndCalcB), 131170128, 149445120)
        TB (kqqkn, false, false, (T32_Q_Q_N_IndCalcW), (T32_Q_Q_N_IndCalcB),  41944320,  48096060)
        TB (kqqkb, false, false, (T32_Q_Q_B_IndCalcW), (T32_Q_Q_B_IndCalcB),  41944320,  49854690)
        TB (kqqkr, false, false, (T32_Q_Q_R_IndCalcW), (T32_Q_Q_R_IndCalcB),  41944320,  49475880)
        TB (kqqkq, false, false, (T32_Q_Q_Q_IndCalcW), (T32_Q_Q_Q_IndCalcB),  41944320,  46912050)

#if defined (T41_INCLUDE)
        TB (kpppk, false, false, (T41_P_P_P_IndCalcW), (T41_P_P_P_IndCalcB),  26061704,  28388716)
        TB (knppk, false, false, (T41_N_P_P_IndCalcW), (T41_N_P_P_IndCalcB), 102898651, 114742320)
        TB (knnpk, false, false, (T41_N_N_P_IndCalcW), (T41_N_N_P_IndCalcB), 130135501, 153741960)
        TB (knnnk, false, false, (T41_N_N_N_IndCalcW), (T41_N_N_N_IndCalcB),  13486227,  17472840)
        TB (kbppk, false, false, (T41_B_P_P_IndCalcW), (T41_B_P_P_IndCalcB), 106602156, 114742320)
        TB (kbnpk, false, false, (T41_B_N_P_IndCalcW), (T41_B_N_P_IndCalcB), 274352939, 307483920)
        TB (kbnnk, false, false, (T41_B_N_N_IndCalcW), (T41_B_N_N_IndCalcB),  43406294, 52418520)
        TB (kbbpk, false, false, (T41_B_B_P_IndCalcW), (T41_B_B_P_IndCalcB), 139715040, 153741960)
        TB (kbbnk, false, false, (T41_B_B_N_IndCalcW), (T41_B_B_N_IndCalcB),  44983618,  52418520)
        TB (kbbbk, false, false, (T41_B_B_B_IndCalcW), (T41_B_B_B_IndCalcB),  15010230,  17472840)
        TB (krppk, false, false, (T41_R_P_P_IndCalcW), (T41_R_P_P_IndCalcB), 105758666, 114742320)
        TB (krnpk, false, false, (T41_R_N_P_IndCalcW), (T41_R_N_P_IndCalcB), 272153675, 307483920)
        TB (krnnk, false, false, (T41_R_N_N_IndCalcW), (T41_R_N_N_IndCalcB),  43056198,  52418520)
        TB (krbpk, false, false, (T41_R_B_P_IndCalcW), (T41_R_B_P_IndCalcB), 281991360, 307483920)
        TB (krbnk, false, false, (T41_R_B_N_IndCalcW), (T41_R_B_N_IndCalcB),  90787358, 104837040)
        TB (krbbk, false, false, (T41_R_B_B_IndCalcW), (T41_R_B_B_IndCalcB),  46242089,  52418520)
        TB (krrpk, false, false, (T41_R_R_P_IndCalcW), (T41_R_R_P_IndCalcB), 137491197, 153741960)
        TB (krrnk, false, false, (T41_R_R_N_IndCalcW), (T41_R_R_N_IndCalcB),  44265261,  52418520)
        TB (krrbk, false, false, (T41_R_R_B_IndCalcW), (T41_R_R_B_IndCalcB),  45873720,  52418520)
        TB (krrrk, false, false, (T41_R_R_R_IndCalcW), (T41_R_R_R_IndCalcB),  14644690,  17472840)
        TB (kqppk, false, false, (T41_Q_P_P_IndCalcW), (T41_Q_P_P_IndCalcB), 100347220, 114742320)
        TB (kqnpk, false, false, (T41_Q_N_P_IndCalcW), (T41_Q_N_P_IndCalcB), 258294639, 307483920)
        TB (kqnnk, false, false, (T41_Q_N_N_IndCalcW), (T41_Q_N_N_IndCalcB),  40873646,  52418520)
        TB (kqbpk, false, false, (T41_Q_B_P_IndCalcW), (T41_Q_B_P_IndCalcB), 267576632, 307483920)
        TB (kqbnk, false, false, (T41_Q_B_N_IndCalcW), (T41_Q_B_N_IndCalcB),  86166717, 104837040)
        TB (kqbbk, false, false, (T41_Q_B_B_IndCalcW), (T41_Q_B_B_IndCalcB),  43879679,  52418520)
        TB (kqrpk, false, false, (T41_Q_R_P_IndCalcW), (T41_Q_R_P_IndCalcB), 265421907, 307483920)
        TB (kqrnk, false, false, (T41_Q_R_N_IndCalcW), (T41_Q_R_N_IndCalcB),  85470603, 104837040)
        TB (kqrbk, false, false, (T41_Q_R_B_IndCalcW), (T41_Q_R_B_IndCalcB),  88557959, 104837040)
        TB (kqrrk, false, false, (T41_Q_R_R_IndCalcW), (T41_Q_R_R_IndCalcB),  43157690,  52418520)
        TB (kqqpk, false, false, (T41_Q_Q_P_IndCalcW), (T41_Q_Q_P_IndCalcB), 123688859, 153741960)
        TB (kqqnk, false, false, (T41_Q_Q_N_IndCalcW), (T41_Q_Q_N_IndCalcB),  39840787,  52418520)
        TB (kqqbk, false, false, (T41_Q_Q_B_IndCalcW), (T41_Q_Q_B_IndCalcB),  41270973,  52418520)
        TB (kqqrk, false, false, (T41_Q_Q_R_IndCalcW), (T41_Q_Q_R_IndCalcB),  40916820,  52418520)
        TB (kqqqk, false, false, (T41_Q_Q_Q_IndCalcW), (T41_Q_Q_Q_IndCalcB),  12479974,  17472840)
#endif

#if defined (T33_INCLUDE)
        TB (knnknn, true,  false, (T33_N_N_N_N_IndCalcW), (T33_N_N_N_N_IndCalcB),  1301488080,  1301488080)
        TB (kbnknn, false, false, (T33_B_N_N_N_IndCalcW), (T33_B_N_N_N_IndCalcB),  2744597400,  2602976160)
        TB (kbbknn, false, false, (T33_B_B_N_N_IndCalcW), (T33_B_B_N_N_IndCalcB),  1398096450,  1301488080)
        TB (kbbkbn, false, false, (T33_B_B_B_N_IndCalcW), (T33_B_B_B_N_IndCalcB),  2796192900,  2744597400)
        TB (kbbkbb, true,  false, (T33_B_B_B_B_IndCalcW), (T33_B_B_B_B_IndCalcB),  1398096450,  1398096450)
        TB (krnknn, false, true,  (T33_R_N_N_N_IndCalcW), (T33_R_N_N_N_IndCalcB),  2723107830,  2602976160)
        TB (krnkbb, false, true,  (T33_R_N_B_B_IndCalcW), (T33_R_N_B_B_IndCalcB),  2723107830,  2796192900)
        TB (krbknn, false, true,  (T33_R_B_N_N_IndCalcW), (T33_R_B_N_N_IndCalcB),  2822371200,  2602976160)
        TB (krbkbb, false, false, (T33_R_B_B_B_IndCalcW), (T33_R_B_B_B_IndCalcB),  2822371200,  2796192900)
        TB (krrknn, false, false, (T33_R_R_N_N_IndCalcW), (T33_R_R_N_N_IndCalcB),  1376421030,  1301488080)
        TB (krrkbn, false, false, (T33_R_R_B_N_IndCalcW), (T33_R_R_B_N_IndCalcB),  2752842060,  2744597400)
        TB (krrkbb, false, false, (T33_R_R_B_B_IndCalcW), (T33_R_R_B_B_IndCalcB),  1376421030,  1398096450)
        TB (krrkrn, false, false,  (T33_R_R_R_N_IndCalcW), (T33_R_R_R_N_IndCalcB),  2752842060,  2723107830)
        TB (krrkrr, true,  false, (T33_R_R_R_R_IndCalcW), (T33_R_R_R_R_IndCalcB),  1376421030,  1376421030)
        TB (kqnknn, false, false, (T33_Q_N_N_N_IndCalcW), (T33_Q_N_N_N_IndCalcB),  2583520320,  2602976160)
        TB (kqnkbb, false, false, (T33_Q_N_B_B_IndCalcW), (T33_Q_N_B_B_IndCalcB),  2583520320,  2796192900)
        TB (kqnkrr, false, true,  (T33_Q_N_R_R_IndCalcW), (T33_Q_N_R_R_IndCalcB),  2583520320,  2752842060)
        TB (kqbknn, false, false, (T33_Q_B_N_N_IndCalcW), (T33_Q_B_N_N_IndCalcB),  2677137390,  2602976160)
        TB (kqbkbb, false, false, (T33_Q_B_B_B_IndCalcW), (T33_Q_B_B_B_IndCalcB),  2677137390,  2796192900)
        TB (kqbkrr, false, false, (T33_Q_B_R_R_IndCalcW), (T33_Q_B_R_R_IndCalcB),  2677137390,  2752842060)
        TB (kqrknn, false, false, (T33_Q_R_N_N_IndCalcW), (T33_Q_R_N_N_IndCalcB),  2656134570,  2602976160)
        TB (kqrkbb, false, false, (T33_Q_R_B_B_IndCalcW), (T33_Q_R_B_B_IndCalcB),  2656134570,  2796192900)
        TB (kqrkrr, false, false, (T33_Q_R_R_R_IndCalcW), (T33_Q_R_R_R_IndCalcB),  2656134570,  2752842060)
        TB (kqqknn, false, false, (T33_Q_Q_N_N_IndCalcW), (T33_Q_Q_N_N_IndCalcB),  1237357440,  1301488080)
        TB (kqqkbn, false, false, (T33_Q_Q_B_N_IndCalcW), (T33_Q_Q_B_N_IndCalcB),  2474714880,  2744597400)
        TB (kqqkbb, false, false, (T33_Q_Q_B_B_IndCalcW), (T33_Q_Q_B_B_IndCalcB),  1237357440,  1398096450)
        TB (kqqkrn, false, false, (T33_Q_Q_R_N_IndCalcW), (T33_Q_Q_R_N_IndCalcB),  2474714880,  2723107830)
        TB (kqqkrb, false, false, (T33_Q_Q_R_B_IndCalcW), (T33_Q_Q_R_B_IndCalcB),  2474714880,  2822371200)
        TB (kqqkrr, false, false, (T33_Q_Q_R_R_IndCalcW), (T33_Q_Q_R_R_IndCalcB),  1237357440,  1376421030)
        TB (kqqkqn, false, false, (T33_Q_Q_Q_N_IndCalcW), (T33_Q_Q_Q_N_IndCalcB),  2474714880,  2583520320)
        TB (kqqkqb, false, false, (T33_Q_Q_Q_B_IndCalcW), (T33_Q_Q_Q_B_IndCalcB),  2474714880,  2677137390)
        TB (kqqkqr, false, false, (T33_Q_Q_Q_R_IndCalcW), (T33_Q_Q_Q_R_IndCalcB),  2474714880,  2656134570)
        TB (kqqkqq, true,  false, (T33_Q_Q_Q_Q_IndCalcW), (T33_Q_Q_Q_Q_IndCalcB),  1237357440,  1237357440)
#endif
        };

#else

CTbDesc rgtbdDesc[cTb] =
        {
        TB (kk, true, false, NULL, NULL, 0, 0)

        TB (kpk, false, false, (T21<P>::IndCalcW), (T21<P>::IndCalcB), 81664, 84012)
        TB (knk, false, false, (T21<N>::IndCalcW), (T21<N>::IndCalcB), 26282, 28644)
        TB (kbk, false, false, (T21<B>::IndCalcW), (T21<B>::IndCalcB), 27243, 28644)
        TB (krk, false, false, (T21<R>::IndCalcW), (T21<R>::IndCalcB), 27030, 28644)
        TB (kqk, false, false, (T21<Q>::IndCalcW), (T21<Q>::IndCalcB), 25629, 28644)

        TB (kpkp, false, false, (T22<P, P>::IndCalcW), (T22<P, P>::IndCalcB), 3863492, 3863492)
        TB (knkp, false, false, (T22<N, P>::IndCalcW), (T22<N, P>::IndCalcB), 4931904, 4981504)
        TB (knkn, true,  false, (T22<N, N>::IndCalcW), (T22<N, N>::IndCalcB), 1603202, 1603202)
        TB (kbkp, false, false, (T22<B, P>::IndCalcW), (T22<B, P>::IndCalcB), 5112000, 4981504)
        TB (kbkn, false, false, (T22<B, N>::IndCalcW), (T22<B, N>::IndCalcB), 1661823, 1603202)
        TB (kbkb, true,  false, (T22<B, B>::IndCalcW), (T22<B, B>::IndCalcB), 1661823, 1661823)
        TB (krkp, false, false, (T22<R, P>::IndCalcW), (T22<R, P>::IndCalcB), 5072736, 4981504)
        TB (krkn, false, false, (T22<R, N>::IndCalcW), (T22<R, N>::IndCalcB), 1649196, 1603202)
        TB (krkb, false, false, (T22<R, B>::IndCalcW), (T22<R, B>::IndCalcB), 1649196, 1661823)
        TB (krkr, true,  false, (T22<R, R>::IndCalcW), (T22<R, R>::IndCalcB), 1649196, 1649196)
        TB (kqkp, false, false, (T22<Q, P>::IndCalcW), (T22<Q, P>::IndCalcB), 4810080, 4981504)
        TB (kqkn, false, false, (T22<Q, N>::IndCalcW), (T22<Q, N>::IndCalcB), 1563735, 1603202)
        TB (kqkb, false, false, (T22<Q, B>::IndCalcW), (T22<Q, B>::IndCalcB), 1563735, 1661823)
        TB (kqkr, false, false, (T22<Q, R>::IndCalcW), (T22<Q, R>::IndCalcB), 1563735, 1649196)
        TB (kqkq, true,  false, (T22<Q, Q>::IndCalcW), (T22<Q, Q>::IndCalcB), 1563735, 1563735)
        TB (kppk, false, false, (T31<P, P>::IndCalcW), (T31<P, P>::IndCalcB), 1806671, 1912372)
        TB (knpk, false, false, (T31<N, P>::IndCalcW), (T31<N, P>::IndCalcB), 4648581, 5124732)
        TB (knnk, false, false, (T31<N, N>::IndCalcW), (T31<N, N>::IndCalcB),  735304,  873642)
        TB (kbpk, false, false, (T31<B, P>::IndCalcW), (T31<B, P>::IndCalcB), 4817128, 5124732)
        TB (kbnk, false, false, (T31<B, N>::IndCalcW), (T31<B, N>::IndCalcB), 1550620, 1747284)
        TB (kbbk, false, false, (T31<B, B>::IndCalcW), (T31<B, B>::IndCalcB),  789885,  873642)
        TB (krpk, false, false, (T31<R, P>::IndCalcW), (T31<R, P>::IndCalcB), 4779530, 5124732)
        TB (krnk, false, false, (T31<R, N>::IndCalcW), (T31<R, N>::IndCalcB), 1538479, 1747284)
        TB (krbk, false, false, (T31<R, B>::IndCalcW), (T31<R, B>::IndCalcB), 1594560, 1747284)
        TB (krrk, false, false, (T31<R, R>::IndCalcW), (T31<R, R>::IndCalcB),  777300,  873642)
        TB (kqpk, false, false, (T31<Q, P>::IndCalcW), (T31<Q, P>::IndCalcB), 4533490, 5124732)
        TB (kqnk, false, false, (T31<Q, N>::IndCalcW), (T31<Q, N>::IndCalcB), 1459616, 1747284)
        TB (kqbk, false, false, (T31<Q, B>::IndCalcW), (T31<Q, B>::IndCalcB), 1512507, 1747284)
        TB (kqrk, false, false, (T31<Q, R>::IndCalcW), (T31<Q, R>::IndCalcB), 1500276, 1747284)
        TB (kqqk, false, false, (T31<Q, Q>::IndCalcW), (T31<Q, Q>::IndCalcB),  698739,  873642)

#if !defined (KPPKP_16BIT)
        TB (kppkp, false, false, (T32<P, P, P>::IndCalcW), (T32<P, P, P>::IndCalcB),  84219361,  89391280)
#else
        TB (kppkp, false, true,  (T32<P, P, P>::IndCalcW), (T32<P, P, P>::IndCalcB),  84219361,  89391280)
#endif
        TB (kppkn, false, false, (T32<P, P, N>::IndCalcW), (T32<P, P, N>::IndCalcB), 108400260, 115899744)
        TB (kppkb, false, false, (T32<P, P, B>::IndCalcW), (T32<P, P, B>::IndCalcB), 108400260, 120132000)
        TB (kppkr, false, false, (T32<P, P, R>::IndCalcW), (T32<P, P, R>::IndCalcB), 108400260, 119209296)
        TB (kppkq, false, false, (T32<P, P, Q>::IndCalcW), (T32<P, P, Q>::IndCalcB), 108400260, 113036880)
        TB (knpkp, false, false, (T32<N, P, P>::IndCalcW), (T32<N, P, P>::IndCalcB), 219921779, 231758952)
        TB (knpkn, false, false, (T32<N, P, N>::IndCalcW), (T32<N, P, N>::IndCalcB), 278914860, 295914240)
        TB (knpkb, false, false, (T32<N, P, B>::IndCalcW), (T32<N, P, B>::IndCalcB), 278914860, 306720000)
        TB (knpkr, false, false, (T32<N, P, R>::IndCalcW), (T32<N, P, R>::IndCalcB), 278914860, 304369920)
        TB (knpkq, false, false, (T32<N, P, Q>::IndCalcW), (T32<N, P, Q>::IndCalcB), 278914860, 288610560)
        TB (knnkp, false, false, (T32<N, N, P>::IndCalcW), (T32<N, N, P>::IndCalcB), 137991648, 149445120)
        TB (knnkn, false, false, (T32<N, N, N>::IndCalcW), (T32<N, N, N>::IndCalcB),  44118240,  48096060)
        TB (knnkb, false, false, (T32<N, N, B>::IndCalcW), (T32<N, N, B>::IndCalcB),  44118240,  49854690)
        TB (knnkr, false, false, (T32<N, N, R>::IndCalcW), (T32<N, N, R>::IndCalcB),  44118240,  49475880)
        TB (knnkq, false, false, (T32<N, N, Q>::IndCalcW), (T32<N, N, Q>::IndCalcB),  44118240,  46912050)
        TB (kbpkp, false, false, (T32<B, P, P>::IndCalcW), (T32<B, P, P>::IndCalcB), 227896016, 231758952)
        TB (kbpkn, false, false, (T32<B, P, N>::IndCalcW), (T32<B, P, N>::IndCalcB), 289027680, 295914240)
        TB (kbpkb, false, false, (T32<B, P, B>::IndCalcW), (T32<B, P, B>::IndCalcB), 289027680, 306720000)
        TB (kbpkr, false, false, (T32<B, P, R>::IndCalcW), (T32<B, P, R>::IndCalcB), 289027680, 304369920)
        TB (kbpkq, false, false, (T32<B, P, Q>::IndCalcW), (T32<B, P, Q>::IndCalcB), 289027680, 288610560)
        TB (kbnkp, false, false, (T32<B, N, P>::IndCalcW), (T32<B, N, P>::IndCalcB), 290989584, 298890240)
        TB (kbnkn, false, false, (T32<B, N, N>::IndCalcW), (T32<B, N, N>::IndCalcB),  93037200,  96192120)
        TB (kbnkb, false, false, (T32<B, N, B>::IndCalcW), (T32<B, N, B>::IndCalcB),  93037200,  99709380)
        TB (kbnkr, false, false, (T32<B, N, R>::IndCalcW), (T32<B, N, R>::IndCalcB),  93037200,  98951760)
        TB (kbnkq, false, false, (T32<B, N, Q>::IndCalcW), (T32<B, N, Q>::IndCalcB),  93037200,  93824100)
        TB (kbbkp, false, false, (T32<B, B, P>::IndCalcW), (T32<B, B, P>::IndCalcB), 148223520, 149445120)
        TB (kbbkn, false, false, (T32<B, B, N>::IndCalcW), (T32<B, B, N>::IndCalcB),  47393100,  48096060)
        TB (kbbkb, false, false, (T32<B, B, B>::IndCalcW), (T32<B, B, B>::IndCalcB),  47393100,  49854690)
        TB (kbbkr, false, false, (T32<B, B, R>::IndCalcW), (T32<B, B, R>::IndCalcB),  47393100,  49475880)
        TB (kbbkq, false, false, (T32<B, B, Q>::IndCalcW), (T32<B, B, Q>::IndCalcB),  47393100,  46912050)
        TB (krpkp, false, false, (T32<R, P, P>::IndCalcW), (T32<R, P, P>::IndCalcB), 226121876, 231758952)
        TB (krpkn, false, false, (T32<R, P, N>::IndCalcW), (T32<R, P, N>::IndCalcB), 286777440, 295914240)
        TB (krpkb, false, false, (T32<R, P, B>::IndCalcW), (T32<R, P, B>::IndCalcB), 286777440, 306720000)
        TB (krpkr, false, false, (T32<R, P, R>::IndCalcW), (T32<R, P, R>::IndCalcB), 286777440, 304369920)
        TB (krpkq, false, false, (T32<R, P, Q>::IndCalcW), (T32<R, P, Q>::IndCalcB), 286777440, 288610560)
        TB (krnkp, false, false, (T32<R, N, P>::IndCalcW), (T32<R, N, P>::IndCalcB), 288692928, 298890240)
        TB (krnkn, false, false, (T32<R, N, N>::IndCalcW), (T32<R, N, N>::IndCalcB),  92308740,  96192120)
        TB (krnkb, false, false, (T32<R, N, B>::IndCalcW), (T32<R, N, B>::IndCalcB),  92308740,  99709380)
        TB (krnkr, false, false, (T32<R, N, R>::IndCalcW), (T32<R, N, R>::IndCalcB),  92308740,  98951760)
        TB (krnkq, false, false, (T32<R, N, Q>::IndCalcW), (T32<R, N, Q>::IndCalcB),  92308740,  93824100)
        TB (krbkp, false, false, (T32<R, B, P>::IndCalcW), (T32<R, B, P>::IndCalcB), 299203200, 298890240)
        TB (krbkn, false, false, (T32<R, B, N>::IndCalcW), (T32<R, B, N>::IndCalcB),  95673600,  96192120)
        TB (krbkb, false, false, (T32<R, B, B>::IndCalcW), (T32<R, B, B>::IndCalcB),  95673600,  99709380)
        TB (krbkr, false, false, (T32<R, B, R>::IndCalcW), (T32<R, B, R>::IndCalcB),  95673600,  98951760)
        TB (krbkq, false, false, (T32<R, B, Q>::IndCalcW), (T32<R, B, Q>::IndCalcB),  95673600,  93824100)
        TB (krrkp, false, false, (T32<R, R, P>::IndCalcW), (T32<R, R, P>::IndCalcB), 145901232, 149445120)
        TB (krrkn, false, false, (T32<R, R, N>::IndCalcW), (T32<R, R, N>::IndCalcB),  46658340,  48096060)
        TB (krrkb, false, false, (T32<R, R, B>::IndCalcW), (T32<R, R, B>::IndCalcB),  46658340,  49854690)
        TB (krrkr, false, false, (T32<R, R, R>::IndCalcW), (T32<R, R, R>::IndCalcB),  46658340,  49475880)
        TB (krrkq, false, false, (T32<R, R, Q>::IndCalcW), (T32<R, R, Q>::IndCalcB),  46658340,  46912050)
        TB (kqpkp, false, false, (T32<Q, P, P>::IndCalcW), (T32<Q, P, P>::IndCalcB), 214481388, 231758952)
        TB (kqpkn, false, false, (T32<Q, P, N>::IndCalcW), (T32<Q, P, N>::IndCalcB), 272015040, 295914240)
        TB (kqpkb, false, false, (T32<Q, P, B>::IndCalcW), (T32<Q, P, B>::IndCalcB), 272015040, 306720000)
        TB (kqpkr, false, false, (T32<Q, P, R>::IndCalcW), (T32<Q, P, R>::IndCalcB), 272015040, 304369920)
        TB (kqpkq, false, false, (T32<Q, P, Q>::IndCalcW), (T32<Q, P, Q>::IndCalcB), 272015040, 288610560)
        TB (kqnkp, false, false, (T32<Q, N, P>::IndCalcW), (T32<Q, N, P>::IndCalcB), 273904512, 298890240)
        TB (kqnkn, false, false, (T32<Q, N, N>::IndCalcW), (T32<Q, N, N>::IndCalcB),  87576960,  96192120)
        TB (kqnkb, false, false, (T32<Q, N, B>::IndCalcW), (T32<Q, N, B>::IndCalcB),  87576960,  99709380)
        TB (kqnkr, false, false, (T32<Q, N, R>::IndCalcW), (T32<Q, N, R>::IndCalcB),  87576960,  98951760)
        TB (kqnkq, false, false, (T32<Q, N, Q>::IndCalcW), (T32<Q, N, Q>::IndCalcB),  87576960,  93824100)
        TB (kqbkp, false, false, (T32<Q, B, P>::IndCalcW), (T32<Q, B, P>::IndCalcB), 283818240, 298890240)
        TB (kqbkn, false, false, (T32<Q, B, N>::IndCalcW), (T32<Q, B, N>::IndCalcB),  90750420,  96192120)
        TB (kqbkb, false, false, (T32<Q, B, B>::IndCalcW), (T32<Q, B, B>::IndCalcB),  90750420,  99709380)
        TB (kqbkr, false, false, (T32<Q, B, R>::IndCalcW), (T32<Q, B, R>::IndCalcB),  90750420,  98951760)
        TB (kqbkq, false, false, (T32<Q, B, Q>::IndCalcW), (T32<Q, B, Q>::IndCalcB),  90750420,  93824100)
        TB (kqrkp, false, false, (T32<Q, R, P>::IndCalcW), (T32<Q, R, P>::IndCalcB), 281568240, 298890240)
        TB (kqrkn, false, false, (T32<Q, R, N>::IndCalcW), (T32<Q, R, N>::IndCalcB),  90038460,  96192120)
        TB (kqrkb, false, false, (T32<Q, R, B>::IndCalcW), (T32<Q, R, B>::IndCalcB),  90038460,  99709380)
        TB (kqrkr, false, false, (T32<Q, R, R>::IndCalcW), (T32<Q, R, R>::IndCalcB),  90038460,  98951760)
        TB (kqrkq, false, false, (T32<Q, R, Q>::IndCalcW), (T32<Q, R, Q>::IndCalcB),  90038460,  93824100)
        TB (kqqkp, false, false, (T32<Q, Q, P>::IndCalcW), (T32<Q, Q, P>::IndCalcB), 131170128, 149445120)
        TB (kqqkn, false, false, (T32<Q, Q, N>::IndCalcW), (T32<Q, Q, N>::IndCalcB),  41944320,  48096060)
        TB (kqqkb, false, false, (T32<Q, Q, B>::IndCalcW), (T32<Q, Q, B>::IndCalcB),  41944320,  49854690)
        TB (kqqkr, false, false, (T32<Q, Q, R>::IndCalcW), (T32<Q, Q, R>::IndCalcB),  41944320,  49475880)
        TB (kqqkq, false, false, (T32<Q, Q, Q>::IndCalcW), (T32<Q, Q, Q>::IndCalcB),  41944320,  46912050)

#if defined (T41_INCLUDE)
        TB (kpppk, false, false, (T41<P, P, P>::IndCalcW), (T41<P, P, P>::IndCalcB),  26061704,  28388716)
        TB (knppk, false, false, (T41<N, P, P>::IndCalcW), (T41<N, P, P>::IndCalcB), 102898651, 114742320)
        TB (knnpk, false, false, (T41<N, N, P>::IndCalcW), (T41<N, N, P>::IndCalcB), 130135501, 153741960)
        TB (knnnk, false, false, (T41<N, N, N>::IndCalcW), (T41<N, N, N>::IndCalcB),  13486227,  17472840)
        TB (kbppk, false, false, (T41<B, P, P>::IndCalcW), (T41<B, P, P>::IndCalcB), 106602156, 114742320)
        TB (kbnpk, false, false, (T41<B, N, P>::IndCalcW), (T41<B, N, P>::IndCalcB), 274352939, 307483920)
        TB (kbnnk, false, false, (T41<B, N, N>::IndCalcW), (T41<B, N, N>::IndCalcB),  43406294, 52418520)
        TB (kbbpk, false, false, (T41<B, B, P>::IndCalcW), (T41<B, B, P>::IndCalcB), 139715040, 153741960)
        TB (kbbnk, false, false, (T41<B, B, N>::IndCalcW), (T41<B, B, N>::IndCalcB),  44983618,  52418520)
        TB (kbbbk, false, false, (T41<B, B, B>::IndCalcW), (T41<B, B, B>::IndCalcB),  15010230,  17472840)
        TB (krppk, false, false, (T41<R, P, P>::IndCalcW), (T41<R, P, P>::IndCalcB), 105758666, 114742320)
        TB (krnpk, false, false, (T41<R, N, P>::IndCalcW), (T41<R, N, P>::IndCalcB), 272153675, 307483920)
        TB (krnnk, false, false, (T41<R, N, N>::IndCalcW), (T41<R, N, N>::IndCalcB),  43056198,  52418520)
        TB (krbpk, false, false, (T41<R, B, P>::IndCalcW), (T41<R, B, P>::IndCalcB), 281991360, 307483920)
        TB (krbnk, false, false, (T41<R, B, N>::IndCalcW), (T41<R, B, N>::IndCalcB),  90787358, 104837040)
        TB (krbbk, false, false, (T41<R, B, B>::IndCalcW), (T41<R, B, B>::IndCalcB),  46242089,  52418520)
        TB (krrpk, false, false, (T41<R, R, P>::IndCalcW), (T41<R, R, P>::IndCalcB), 137491197, 153741960)
        TB (krrnk, false, false, (T41<R, R, N>::IndCalcW), (T41<R, R, N>::IndCalcB),  44265261,  52418520)
        TB (krrbk, false, false, (T41<R, R, B>::IndCalcW), (T41<R, R, B>::IndCalcB),  45873720,  52418520)
        TB (krrrk, false, false, (T41<R, R, R>::IndCalcW), (T41<R, R, R>::IndCalcB),  14644690,  17472840)
        TB (kqppk, false, false, (T41<Q, P, P>::IndCalcW), (T41<Q, P, P>::IndCalcB), 100347220, 114742320)
        TB (kqnpk, false, false, (T41<Q, N, P>::IndCalcW), (T41<Q, N, P>::IndCalcB), 258294639, 307483920)
        TB (kqnnk, false, false, (T41<Q, N, N>::IndCalcW), (T41<Q, N, N>::IndCalcB),  40873646,  52418520)
        TB (kqbpk, false, false, (T41<Q, B, P>::IndCalcW), (T41<Q, B, P>::IndCalcB), 267576632, 307483920)
        TB (kqbnk, false, false, (T41<Q, B, N>::IndCalcW), (T41<Q, B, N>::IndCalcB),  86166717, 104837040)
        TB (kqbbk, false, false, (T41<Q, B, B>::IndCalcW), (T41<Q, B, B>::IndCalcB),  43879679,  52418520)
        TB (kqrpk, false, false, (T41<Q, R, P>::IndCalcW), (T41<Q, R, P>::IndCalcB), 265421907, 307483920)
        TB (kqrnk, false, false, (T41<Q, R, N>::IndCalcW), (T41<Q, R, N>::IndCalcB),  85470603, 104837040)
        TB (kqrbk, false, false, (T41<Q, R, B>::IndCalcW), (T41<Q, R, B>::IndCalcB),  88557959, 104837040)
        TB (kqrrk, false, false, (T41<Q, R, R>::IndCalcW), (T41<Q, R, R>::IndCalcB),  43157690,  52418520)
        TB (kqqpk, false, false, (T41<Q, Q, P>::IndCalcW), (T41<Q, Q, P>::IndCalcB), 123688859, 153741960)
        TB (kqqnk, false, false, (T41<Q, Q, N>::IndCalcW), (T41<Q, Q, N>::IndCalcB),  39840787,  52418520)
        TB (kqqbk, false, false, (T41<Q, Q, B>::IndCalcW), (T41<Q, Q, B>::IndCalcB),  41270973,  52418520)
        TB (kqqrk, false, false, (T41<Q, Q, R>::IndCalcW), (T41<Q, Q, R>::IndCalcB),  40916820,  52418520)
        TB (kqqqk, false, false, (T41<Q, Q, Q>::IndCalcW), (T41<Q, Q, Q>::IndCalcB),  12479974,  17472840)
#endif

#if defined (T33_INCLUDE)
        TB (knnknn, true,  false, (T33<N, N, N, N>::IndCalcW), (T33<N, N, N, N>::IndCalcB),  1301488080,  1301488080)
        TB (kbnknn, false, false, (T33<B, N, N, N>::IndCalcW), (T33<B, N, N, N>::IndCalcB),  2744597400,  2602976160)
        TB (kbbknn, false, false, (T33<B, B, N, N>::IndCalcW), (T33<B, B, N, N>::IndCalcB),  1398096450,  1301488080)
        TB (kbbkbn, false, false, (T33<B, B, B, N>::IndCalcW), (T33<B, B, B, N>::IndCalcB),  2796192900,  2744597400)
        TB (kbbkbb, true,  false, (T33<B, B, B, B>::IndCalcW), (T33<B, B, B, B>::IndCalcB),  1398096450,  1398096450)
        TB (krnknn, false, true,  (T33<R, N, N, N>::IndCalcW), (T33<R, N, N, N>::IndCalcB),  2723107830,  2602976160)
        TB (krnkbb, false, true,  (T33<R, N, B, B>::IndCalcW), (T33<R, N, B, B>::IndCalcB),  2723107830,  2796192900)
        TB (krbknn, false, true,  (T33<R, B, N, N>::IndCalcW), (T33<R, B, N, N>::IndCalcB),  2822371200,  2602976160)
        TB (krbkbb, false, false, (T33<R, B, B, B>::IndCalcW), (T33<R, B, B, B>::IndCalcB),  2822371200,  2796192900)
        TB (krrknn, false, false, (T33<R, R, N, N>::IndCalcW), (T33<R, R, N, N>::IndCalcB),  1376421030,  1301488080)
        TB (krrkbn, false, false, (T33<R, R, B, N>::IndCalcW), (T33<R, R, B, N>::IndCalcB),  2752842060,  2744597400)
        TB (krrkbb, false, false, (T33<R, R, B, B>::IndCalcW), (T33<R, R, B, B>::IndCalcB),  1376421030,  1398096450)
        TB (krrkrn, false, false,  (T33<R, R, R, N>::IndCalcW), (T33<R, R, R, N>::IndCalcB),  2752842060,  2723107830)
        TB (krrkrr, true,  false, (T33<R, R, R, R>::IndCalcW), (T33<R, R, R, R>::IndCalcB),  1376421030,  1376421030)
        TB (kqnknn, false, false, (T33<Q, N, N, N>::IndCalcW), (T33<Q, N, N, N>::IndCalcB),  2583520320,  2602976160)
        TB (kqnkbb, false, false, (T33<Q, N, B, B>::IndCalcW), (T33<Q, N, B, B>::IndCalcB),  2583520320,  2796192900)
        TB (kqnkrr, false, true,  (T33<Q, N, R, R>::IndCalcW), (T33<Q, N, R, R>::IndCalcB),  2583520320,  2752842060)
        TB (kqbknn, false, false, (T33<Q, B, N, N>::IndCalcW), (T33<Q, B, N, N>::IndCalcB),  2677137390,  2602976160)
        TB (kqbkbb, false, false, (T33<Q, B, B, B>::IndCalcW), (T33<Q, B, B, B>::IndCalcB),  2677137390,  2796192900)
        TB (kqbkrr, false, false, (T33<Q, B, R, R>::IndCalcW), (T33<Q, B, R, R>::IndCalcB),  2677137390,  2752842060)
        TB (kqrknn, false, false, (T33<Q, R, N, N>::IndCalcW), (T33<Q, R, N, N>::IndCalcB),  2656134570,  2602976160)
        TB (kqrkbb, false, false, (T33<Q, R, B, B>::IndCalcW), (T33<Q, R, B, B>::IndCalcB),  2656134570,  2796192900)
        TB (kqrkrr, false, false, (T33<Q, R, R, R>::IndCalcW), (T33<Q, R, R, R>::IndCalcB),  2656134570,  2752842060)
        TB (kqqknn, false, false, (T33<Q, Q, N, N>::IndCalcW), (T33<Q, Q, N, N>::IndCalcB),  1237357440,  1301488080)
        TB (kqqkbn, false, false, (T33<Q, Q, B, N>::IndCalcW), (T33<Q, Q, B, N>::IndCalcB),  2474714880,  2744597400)
        TB (kqqkbb, false, false, (T33<Q, Q, B, B>::IndCalcW), (T33<Q, Q, B, B>::IndCalcB),  1237357440,  1398096450)
        TB (kqqkrn, false, false, (T33<Q, Q, R, N>::IndCalcW), (T33<Q, Q, R, N>::IndCalcB),  2474714880,  2723107830)
        TB (kqqkrb, false, false, (T33<Q, Q, R, B>::IndCalcW), (T33<Q, Q, R, B>::IndCalcB),  2474714880,  2822371200)
        TB (kqqkrr, false, false, (T33<Q, Q, R, R>::IndCalcW), (T33<Q, Q, R, R>::IndCalcB),  1237357440,  1376421030)
        TB (kqqkqn, false, false, (T33<Q, Q, Q, N>::IndCalcW), (T33<Q, Q, Q, N>::IndCalcB),  2474714880,  2583520320)
        TB (kqqkqb, false, false, (T33<Q, Q, Q, B>::IndCalcW), (T33<Q, Q, Q, B>::IndCalcB),  2474714880,  2677137390)
        TB (kqqkqr, false, false, (T33<Q, Q, Q, R>::IndCalcW), (T33<Q, Q, Q, R>::IndCalcB),  2474714880,  2656134570)
        TB (kqqkqq, true,  false, (T33<Q, Q, Q, Q>::IndCalcW), (T33<Q, Q, Q, Q>::IndCalcB),  1237357440,  1237357440)
#endif
        };

#endif

#undef  P
#undef  N
#undef  B
#undef  R
#undef  Q

//      Helper structure
//      Used to classify on-board position

union CUTbReference             // Hungarian: utbr
        {
        int                             m_iDesc;                        // Negative if have to inverse
        int                             m_cPieces;
/* mob_pach */
#ifndef  __cplusplus
        union CUTbReference     *m_utbReference;
#else
        CUTbReference   *m_utbReference;
#endif
        };

//      Root of the search tree

/* mob_pach */
#ifndef  __cplusplus
static union CUTbReference rgutbReference [MAX_NON_KINGS + 2];
#else
static CUTbReference rgutbReference [MAX_NON_KINGS + 2];
#endif

// Convert TB name (e.g. KQKR) into set of counters

static const char *PchSetHalfCounters
        (
        int                     *piCounters,
        const char      *pch
        )
        {
        memset (piCounters, 0, 5 * sizeof (int));
        while ('\0' != *pch && 'k' != *pch)
                {
                piece pi;

                pi = x_piecePawn;       // To make compiler happy
                switch (*pch)
                        {
                case 'p':
                        pi = x_piecePawn;
                        break;
                case 'n':
                        pi = x_pieceKnight;
                        break;
                case 'b':
                        pi = x_pieceBishop;
                        break;
                case 'r':
                        pi = x_pieceRook;
                        break;
                case 'q':
                        pi = x_pieceQueen;
                        break;
                default:
                        assert (0);
                        }
                piCounters [pi-1] ++;
                pch ++;
                }
                return pch;
        };

static void VSetCounters
        (
        int                     *piCounters,
        const char      *pch
        )
        {
        assert ('k' == *pch);
        pch = PchSetHalfCounters (piCounters, pch+1);
        assert ('k' == *pch);
        pch = PchSetHalfCounters (piCounters+5, pch+1);
        assert ('\0' == *pch);
        }

//      Following functions return TB index
//      They differ by input arguments

/* mob_pach */
#ifndef  __cplusplus
int IDescFindFromCounters
#else
extern "C" int IDescFindFromCounters
#endif
        (
        int     *piCount
        )
        {
/* mob_pach */
#ifndef  __cplusplus
        union CUTbReference *putbr = rgutbReference;
#else
        CUTbReference *putbr = rgutbReference;
#endif

        if (piCount[0] > putbr->m_cPieces)
                goto not_found;
        putbr = putbr[1 + piCount[0]].m_utbReference;
        if (piCount[1] > putbr->m_cPieces)
                goto not_found;
        putbr = putbr[1 + piCount[1]].m_utbReference;
        if (piCount[2] > putbr->m_cPieces)
                goto not_found;
        putbr = putbr[1 + piCount[2]].m_utbReference;
        if (piCount[3] > putbr->m_cPieces)
                goto not_found;
        putbr = putbr[1 + piCount[3]].m_utbReference;
        if (piCount[4] > putbr->m_cPieces)
                goto not_found;
        putbr = putbr[1 + piCount[4]].m_utbReference;
        if (piCount[5] > putbr->m_cPieces)
                goto not_found;
        putbr = putbr[1 + piCount[5]].m_utbReference;
        if (piCount[6] > putbr->m_cPieces)
                goto not_found;
        putbr = putbr[1 + piCount[6]].m_utbReference;
        if (piCount[7] > putbr->m_cPieces)
                goto not_found;
        putbr = putbr[1 + piCount[7]].m_utbReference;
        if (piCount[8] > putbr->m_cPieces)
                goto not_found;
        putbr = putbr[1 + piCount[8]].m_utbReference;
        if (piCount[9] <= putbr->m_cPieces)
                return putbr[1 + piCount[9]].m_iDesc;
not_found:
        return 0;
        }

int     IDescFind
        (
        square  *p_piW, // IN | Pointer to array of white pieces (king excluded)
        square  *p_piB, // IN | Pointer to array of black pieces (king excluded)
        int             cWhite, // IN | Counter of white pieces (king excluded)
        int             cBlack  // IN | Counter of black pieces (king excluded)
        )
        {
        int     rgiCount[10];

        // Set pieces counters
        rgiCount[0] =
        rgiCount[1] =
        rgiCount[2] =
        rgiCount[3] =
        rgiCount[4] =
        rgiCount[5] =
        rgiCount[6] =
        rgiCount[7] =
        rgiCount[8] =
        rgiCount[9] = 0;
        while (cWhite)
                {
                rgiCount[(*p_piW)-1] ++;
                p_piW ++;
                cWhite --;
                }
        while (cBlack)
                {
                rgiCount[5-1+(*p_piB)] ++;
                p_piB ++;
                cBlack --;
                }
        return IDescFindFromCounters (rgiCount);
        }

int     IDescFindByName
        (
        char    *pchName
        )
        {
        int     rgiCount[10];

        VSetCounters (rgiCount, pchName);
        return IDescFindFromCounters (rgiCount);
        }

//-----------------------------------------------------------------------------
//
//      Function used during initialization

//      Set of functions to create search table

/* mob_pach */
#ifndef  __cplusplus
static union CUTbReference *PutbrCreateSubtable
#else
static CUTbReference *PutbrCreateSubtable
#endif
        (
        int     cPieces,        //      IN | # of pieces ramaining on board
        int     iDepth          //      IN | Recursion depth (# of piece classes left)
        )
        {
/* mob_pach */
#ifndef  __cplusplus
        union CUTbReference *putbr;
#else
        CUTbReference *putbr;
#endif

/* mob_pach */
#ifndef  __cplusplus
        putbr = (union CUTbReference *) PvMalloc ((cPieces + 2) * sizeof (union CUTbReference));
#else
        putbr = (CUTbReference *) PvMalloc ((cPieces + 2) * sizeof (CUTbReference));
#endif
        putbr[0].m_cPieces = cPieces;
        if (0 == iDepth)
                {
/* mob_pach */
#ifndef  __cplusplus
   {
      int i;
                for (i = 0; i <= cPieces; i ++)
#else
                for (int i = 0; i <= cPieces; i ++)
#endif
                        putbr[i+1].m_iDesc = 0;
/* mob_pach */
#ifndef  __cplusplus
   }
#endif
                }
        else
                {
/* mob_pach */
#ifndef  __cplusplus
   {
                int i;
      for (i = 0; i <= cPieces; i ++)
#else
                for (int i = 0; i <= cPieces; i ++)
#endif
                        putbr[i+1].m_utbReference = PutbrCreateSubtable (cPieces-i, iDepth-1);
/* mob_pach */
#ifndef  __cplusplus
   }
#endif
                }
        return putbr;
        }

static bool fTbTableCreated = false;

static void VCreateEmptyTbTable (void)
        {
        if (fTbTableCreated)
                return;
        fTbTableCreated = true;
        rgutbReference[0].m_cPieces = MAX_NON_KINGS;
/* mob_pach */
#ifndef  __cplusplus
{
        int i;
   for (i = 0; i <= MAX_NON_KINGS; i ++)
#else
        for (int i = 0; i <= MAX_NON_KINGS; i ++)
#endif
                rgutbReference[i+1].m_utbReference = PutbrCreateSubtable (MAX_NON_KINGS - i, 8);
/* mob_pach */
#ifndef  __cplusplus
}
#endif
        }

// Insert TB (e.g. KQKR) into search table

static bool FRegisterHalf
        (
        int             iTb,
        int             *piCount
        )
        {
/* mob_pach */
#ifndef  __cplusplus
        union CUTbReference     *putbr;
#else
        CUTbReference   *putbr;
#endif

        putbr = rgutbReference;
/* mob_pach */
#ifndef  __cplusplus
{
        int i;
   for (i = 0; i < 9; i ++)
#else
        for (int i = 0; i < 9; i ++)
#endif
                {
                if (piCount[i] > putbr->m_cPieces)
                        return false;
                putbr = putbr[1 + piCount[i]].m_utbReference;
                }
/* mob_pach */
#ifndef  __cplusplus
}
#endif
        if (piCount[9] > putbr->m_cPieces)
                return false;
        putbr[1 + piCount[9]].m_iDesc = iTb;
        return true;
        }

// Insert TB (both, e.g. KQKR and KRKQ) into search table

static bool FRegisterTb
        (
        CTbDesc *ptbDesc
        )
        {
        int             rgiCount[10];
        bool    fInserted;

        VSetCounters (rgiCount, ptbDesc->m_rgchName);
        fInserted = FRegisterHalf (ptbDesc->m_iTbId, rgiCount);
        if (fInserted)
                {
                if (ptbDesc->m_fSymmetric)
                        return true;
/* mob_pach */
#ifndef  __cplusplus
   {
                int i;
      for (i = 0; i < 5; i ++)
#else
                for (int i = 0; i < 5; i ++)
#endif
                        {
                        int     iTemp;

                        iTemp = rgiCount[i];
                        rgiCount[i] = rgiCount[i+5];
                        rgiCount[i+5] = iTemp;
                        }
/* mob_pach */
#ifndef  __cplusplus
   }
#endif

                fInserted = FRegisterHalf (-ptbDesc->m_iTbId, rgiCount);
                assert (fInserted);
                }
        return fInserted;
        }

// File mapping - Win32 code only

#if defined (_WIN32)

#include <windows.h>

static BYTE * PbMapFileForRead
        (
        char    *szName,
        HANDLE  *phFile,
        HANDLE  *phFileMapping
        )
        {
        HANDLE  hFile;
        HANDLE  hFileMapping;
        LPVOID  lpFileBase;

        hFile = CreateFile (szName, GENERIC_READ, FILE_SHARE_READ,
                                                NULL, OPEN_EXISTING,
                                                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
        if (INVALID_HANDLE_VALUE == hFile)
                {
                printf("*** Couldn't open file %s with CreateFile()\n", szName);
                exit (1);
                }
        hFileMapping = CreateFileMapping (hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (0 == hFileMapping)
                {
                CloseHandle (hFile);
                printf ("*** Couldn't open file %s mapping with CreateFileMapping()\n", szName);
                exit (1);
                }
        lpFileBase = MapViewOfFile (hFileMapping, FILE_MAP_READ, 0, 0, 0);
        if (0 == lpFileBase)
                {
                CloseHandle (hFileMapping);
                CloseHandle (hFile);
                printf ("*** Couldn't map view of file %s with MapViewOfFile()\n", szName);
                exit (1);
                }
        if (NULL != phFile)
                *phFile = hFile;
        if (NULL != phFileMapping)
                *phFileMapping = hFileMapping;
        return (BYTE*) lpFileBase;
        }

static void     VUnmapFile
        (
        BYTE    *pbFileBase,
        HANDLE  hFile,
        HANDLE  hFileMapping
        )
        {
        BOOL fFailed;

        fFailed = (0 == UnmapViewOfFile (pbFileBase)) |
                          (0 == CloseHandle (hFileMapping)) |
                          (0 == CloseHandle (hFile));
        if (fFailed)
                {
                printf ("*** Couldn't unmap file\n");
                exit (1);
                }
        }

#endif

//-----------------------------------------------------------------------------
//
//      TB caching

#if !defined (TB_CB_CACHE_CHUNK)
#define TB_CB_CACHE_CHUNK                       8192 /* Must be power of 2 */
#define LOG2_TB_CB_CACHE_CHUNK          13
#endif

#define TB_CHUNK(index)             ((index) >> LOG2_TB_CB_CACHE_CHUNK)
#define TB_OFFSET(index)            ((index) % TB_CB_CACHE_CHUNK)
#define TB_DIRECTORY_ENTRY(chunk)       ((chunk) % TB_DIRECTORY_SIZE)

#define WIDE_TB_CHUNK(index)             ((index) >> (LOG2_TB_CB_CACHE_CHUNK-1))
#define WIDE_TB_OFFSET(index)            ((index) % (TB_CB_CACHE_CHUNK/2))*2

struct CTbCache                 //Hungarian: tbc
        {
        volatile        int                     m_iTb;
        volatile        color           m_color;
                                INDEX           m_indChunk;
/* mob_pach */
#ifndef  __cplusplus
        volatile        struct CTbCache *m_ptbcNext;    // Next element in double-linked general LRU list
        volatile        struct CTbCache *m_ptbcPrev;    // Previous element in double-linked general LRU list
        volatile        struct CTbCache *m_ptbcTbNext;  // Next element in double-linked cache bucket LRU list
        volatile        struct CTbCache *m_ptbcTbPrev;  // Previous element in double-linked cache bucket LRU list
#else
        volatile        CTbCache        *m_ptbcNext;    // Next element in double-linked general LRU list
        volatile        CTbCache        *m_ptbcPrev;    // Previous element in double-linked general LRU list
        volatile        CTbCache        *m_ptbcTbNext;  // Next element in double-linked cache bucket LRU list
        volatile        CTbCache        *m_ptbcTbPrev;  // Previous element in double-linked cache bucket LRU list
#endif
                                BYTE            *m_pbData;
        };

/* mob_pach */
#ifndef  __cplusplus
static struct CTbCache  *ptbcTbCache;   // Cache memory
#else
static CTbCache *ptbcTbCache;   // Cache memory
#endif
static ULONG    ctbcTbCache;    // Cache size (in entries)

/* mob_pach */
#ifndef  __cplusplus
static volatile struct CTbCache *ptbcHead;              // Head of that list
static volatile struct CTbCache *ptbcTail;              // Last element in that list
static volatile struct CTbCache *ptbcFree;              // First free cache header
#else
static volatile CTbCache        *ptbcHead;              // Head of that list
static volatile CTbCache        *ptbcTail;              // Last element in that list
static volatile CTbCache        *ptbcFree;              // First free cache header
#endif

static INLINE void VTbCloseFile
        (
        int             iTb,
        color   side
        )
        {
        if (NULL != rgtbdDesc[iTb].m_rgfpFiles[side])
                {
                Lock (rgtbdDesc[iTb].m_rglockFiles[side]);
                fclose (rgtbdDesc[iTb].m_rgfpFiles[side]);
                rgtbdDesc[iTb].m_rgfpFiles[side] = NULL;
                UnLock (rgtbdDesc[iTb].m_rglockFiles[side]);
                }
        }

/* mob_pach */
#ifndef  __cplusplus
void VTbCloseFiles (void)
#else
extern "C" void VTbCloseFiles (void)
#endif
        {
        // Initialized?
        if (0 == ctbcTbCache)
                return;

        // Walk through TB cache and close all opened files
/* mob_pach */
#ifndef  __cplusplus
{
        int iTb;
   for (iTb = 1; iTb < cTb; iTb ++)
#else
        for (int iTb = 1; iTb < cTb; iTb ++)
#endif
                {
                VTbCloseFile (iTb, x_colorWhite);
                VTbCloseFile (iTb, x_colorBlack);
                }
/* mob_pach */
#ifndef  __cplusplus
}
#endif
        }

void VTbClearCache (void)
        {
/* mob_pach */
#ifndef  __cplusplus
        struct CTbCache *ptbc;
#else
        CTbCache *ptbc;
#endif
        BYTE *pb;
        ULONG i;

        // Initialized?
        if (0 == ctbcTbCache)
                return;
        VTbCloseFiles();

        // Initialize all lists
        pb = (BYTE *) & ptbcTbCache [ctbcTbCache];
        for (i = 0, ptbc = ptbcTbCache; i < ctbcTbCache; i ++, ptbc ++)
                {
                ptbc->m_pbData = pb + i*(TB_CB_CACHE_CHUNK+32+4);
                ptbc->m_ptbcTbPrev =
                ptbc->m_ptbcTbNext =
                ptbc->m_ptbcPrev = NULL;
                ptbc->m_ptbcNext = (ptbc + 1);
                }
        ptbc[-1].m_ptbcNext = NULL;

        // Clear references from TBs
/* mob_pach */
#ifndef  __cplusplus
{
        int iTb;
   for (iTb = 1; iTb < cTb; iTb ++)
#else
        for (int iTb = 1; iTb < cTb; iTb ++)
#endif
                {
                if (NULL != rgtbdDesc[iTb].m_prgtbcbBuckets[x_colorWhite])
                        memset (rgtbdDesc[iTb].m_prgtbcbBuckets[x_colorWhite], 0,
                                        TB_DIRECTORY_SIZE * sizeof (CTbCacheBucket));
                if (NULL != rgtbdDesc[iTb].m_prgtbcbBuckets[x_colorBlack])
                        memset (rgtbdDesc[iTb].m_prgtbcbBuckets[x_colorBlack], 0,
                                        TB_DIRECTORY_SIZE * sizeof (CTbCacheBucket));
                }
#ifndef  __cplusplus
}
#endif
        // Set globals
        ptbcHead = ptbcTail = NULL;
        ptbcFree = ptbcTbCache;
        }

/* mob_pach */
#ifndef  __cplusplus
int FTbSetCacheSize
#else
extern "C" int FTbSetCacheSize
#endif
        (
        void    *pv,
        ULONG   cbSize
        )
        {
        VTbCloseFiles();
        ctbcTbCache = 0;
        ptbcHead = NULL;
/* mob_pach */
#ifndef  __cplusplus
        if (cbSize < sizeof (struct CTbCache))
#else
        if (cbSize < sizeof (CTbCache))
#endif
                return false;
/* mob_pach */
#ifndef  __cplusplus
        ptbcTbCache = (struct CTbCache*) pv;
#else
        ptbcTbCache = (CTbCache*) pv;
#endif
/* mob_pach */
#ifndef  __cplusplus
        ctbcTbCache = cbSize / (sizeof (struct CTbCache) + TB_CB_CACHE_CHUNK+32+4);
#else
        ctbcTbCache = cbSize / (sizeof (CTbCache) + TB_CB_CACHE_CHUNK+32+4);
#endif
        VTbClearCache();
        return true;
        }

// Table registered

#define FRegistered(iTb, side)  (NULL != rgtbdDesc[iTb].m_rgpchFileName[rgtbdDesc[iTb].m_fSymmetric ? x_colorWhite : (side)])
/* mob_pach */
#ifndef  __cplusplus
int FRegisteredFun
#else
extern "C" int FRegisteredFun
#endif
        (
        int             iTb,
        color   side
        )
        {
        return FRegistered (iTb, side);
        }

// Return function that calculates the necessary index:

#define PfnIndCalc(iTb, side)   (rgtbdDesc[iTb].m_rgpfnCalcIndex[side])
/* mob_pach */
#ifndef  __cplusplus
PfnCalcIndex PfnIndCalcFun
#else
extern "C" PfnCalcIndex PfnIndCalcFun
#endif
        (
        int             iTb,
        color   side
        )
        {
        return PfnIndCalc (iTb, side);
        }

// Read whole file into memory

/* mob_pach */
#ifndef  __cplusplus
int FReadTableToMemory
#else
extern "C" int FReadTableToMemory
#endif
        (
        int             iTb,    // IN | Tablebase
        color   side,   // IN | Side to move
        BYTE    *pb             // IN | Either buffer or NULL
        )
        {
        char    *pszName;
        INDEX   cb;
        FILE    *fp;

        if (rgtbdDesc[iTb].m_fSymmetric)
                side = x_colorWhite;
        if (!FRegistered (iTb, side))
                return false;
        if (NULL != rgtbdDesc[iTb].m_rgpdiDecodeInfo[side])
                return false;
        pszName = rgtbdDesc[iTb].m_rgpchFileName[side];
        fp = fopen (pszName, "rb");
        if (NULL == fp)
                return false;

        // Find database size
#if defined (NEW)
        cb = rgtbdDesc[iTb].m_rgcbLength[side];
        if (0 == cb)
                {
#endif
                if (0 != fseek (fp, 0L, SEEK_END))
                        {
                        printf ("*** Seek in %s failed\n", pszName);
                        exit (1);
                        }
                cb = ftell (fp);
                if ((unsigned) -1 == cb)
                        {
                        printf ("*** Cannot find length of %s\n", pszName);
                        exit (1);
                        }
                if (0 != fseek (fp, 0L, SEEK_SET))
                        {
                        printf ("*** Seek in %s failed\n", pszName);
                        exit (1);
                        }
#if defined (NEW)
                }
#if defined (T33_INCLUDE) || defined (KPPKP_16BIT)
    else if (rgtbdDesc[iTb].m_f16bit)
        {
        if (cb * 2 / 2 != cb)   // Overflow
                    {
                    printf ("*** %s too big to read into memory\n", pszName);
                    exit (1);
                    }
        }
#endif
#endif

        // If buffer not specified, allocate memory for it
        if (NULL == pb)
                pb = (BYTE*) PvMalloc (cb);

        // Read file into memory
        if (cb != (INDEX) fread (pb, 1, cb, fp))
                {
                printf ("*** Read from %s failed\n", pszName);
                exit (1);
                }
        fclose (fp);

        // All done
        rgtbdDesc[iTb].m_rgpbRead[side] = pb;
        return true;
        }

#if defined (_WIN32)

// Map whole file into memory

/* mob_pach */
#ifndef  __cplusplus
int FMapTableToMemory
#else
extern "C" int FMapTableToMemory
#endif
        (
        int             iTb,    // IN | Tablebase
        color   side    // IN | Side to move
        )
        {
        char *pszName;

        if (rgtbdDesc[iTb].m_fSymmetric)
                side = x_colorWhite;
        if (!FRegistered (iTb, side))
                return false;
        if (NULL != rgtbdDesc[iTb].m_rgpdiDecodeInfo[side])
                return false;
        pszName = rgtbdDesc[iTb].m_rgpchFileName[side];
        if (NULL == rgtbdDesc[iTb].m_rgpbRead[side])
                {
                rgtbdDesc[iTb].m_rgpbRead[side] = PbMapFileForRead (pszName, NULL, NULL);
                if (fVerbose)
                        printf ("%s mapped\n", pszName);
                }
        return true;
        }

// Map whole file into memory

/* mob_pach */
#ifndef  __cplusplus
int FMapTableToMemory4
#else
int FMapTableToMemory
#endif
        (
        int             iTb,                    // IN  | Tablebase
        color   side,                   // IN  | Side to move
        HANDLE  *phFile,                // OUT | File handle will be written here
        HANDLE  *phFileMapping  // OUT | File mapping handle will be written here
        )
        {
        char *pszName;

        if (rgtbdDesc[iTb].m_fSymmetric)
                side = x_colorWhite;
        if (!FRegistered (iTb, side))
                return false;
        pszName = rgtbdDesc[iTb].m_rgpchFileName[side];
        if (NULL == rgtbdDesc[iTb].m_rgpbRead[side])
                {
                rgtbdDesc[iTb].m_rgpbRead[side] = PbMapFileForRead (pszName, phFile, phFileMapping);
                if (fVerbose)
                        printf ("%s mapped\n", pszName);
                }
        return true;
        }

// Unmap whole file from memory

int FUnMapTableFromMemory
        (
        int             iTb,                    // IN | Tablebase
        color   side,                   // IN | Side to move
        HANDLE  hFile,                  // IN | File handle will be written here
        HANDLE  hFileMapping    // IN | File mapping handle will be written here
        )
        {
        char *pszName;

        if (rgtbdDesc[iTb].m_fSymmetric)
                side = x_colorWhite;
        if (!FRegistered (iTb, side))
                return false;
        pszName = rgtbdDesc[iTb].m_rgpchFileName[side];
        if (NULL != rgtbdDesc[iTb].m_rgpbRead[side])
                {
                VUnmapFile (rgtbdDesc[iTb].m_rgpbRead[side], hFile, hFileMapping);
                rgtbdDesc[iTb].m_rgpbRead[side] = NULL;
                if (fVerbose)
                        printf ("%s unmapped\n", pszName);
                }
        return true;
        }

#endif

// Probe TB - lower level (not exportable) function

/* mob_pach */
#ifndef  __cplusplus
static int TB_FASTCALL TbtProbeTable4
#else
static int TB_FASTCALL TbtProbeTable
#endif
        (
        int             iTb,
        color   side,
        INDEX   indChunk,
        INDEX   indInChunk
        )
        {
        CTbDesc *ptbd;
        int iDirectory;
/* mob_pach */
#ifndef  __cplusplus
        volatile struct CTbCache        *ptbc;
        volatile struct CTbCache        *ptbcTbFirst;
#else
        volatile CTbCache       *ptbc;
        volatile CTbCache       *ptbcTbFirst;
#endif

        ptbd = & rgtbdDesc[iTb];
        iDirectory = TB_DIRECTORY_ENTRY (indChunk);

        // Head of the cache bucket LRU list
        Lock (ptbd->m_prgtbcbBuckets[side][iDirectory].m_lock);
        ptbcTbFirst = ptbd->m_prgtbcbBuckets[side][iDirectory].m_ptbcFirst;

        // First, search entry in the cache
        for (ptbc = ptbcTbFirst; NULL != ptbc; ptbc = ptbc->m_ptbcTbNext)
                {
                if (indChunk == ptbc->m_indChunk)
                        {
                        // Found - move cache entry to the head of the general LRU list
                        Lock (lockLRU);
                        if (ptbc != ptbcHead)
                                {
                                // Remove it from its current position
                                ptbc->m_ptbcPrev->m_ptbcNext = ptbc->m_ptbcNext;
                                if (NULL == ptbc->m_ptbcNext)
                                        ptbcTail = ptbc->m_ptbcPrev;
                                else
                                        ptbc->m_ptbcNext->m_ptbcPrev = ptbc->m_ptbcPrev;
                                // Insert it at the head
                                ptbc->m_ptbcPrev = NULL;
                                ptbc->m_ptbcNext = ptbcHead;
                                ptbcHead->m_ptbcPrev = ptbc;
                                ptbcHead = ptbc;
                                }
                        UnLock (lockLRU);
                        // Move cache entry to the head of the cache bucket LRU list
                        if (ptbc != ptbcTbFirst)
                                {
                                // Remove it from list
                                ptbc->m_ptbcTbPrev->m_ptbcTbNext = ptbc->m_ptbcTbNext;
                                if (NULL != ptbc->m_ptbcTbNext)
                                        ptbc->m_ptbcTbNext->m_ptbcTbPrev = ptbc->m_ptbcTbPrev;
                                // Insert it at head
                                ptbc->m_ptbcTbPrev = NULL;
                                ptbc->m_ptbcTbNext = ptbcTbFirst;
                                ptbcTbFirst->m_ptbcTbPrev = ptbc;
                                ptbd->m_prgtbcbBuckets[side][iDirectory].m_ptbcFirst = ptbc;
                                }
/* mob_pach */
#ifndef  __cplusplus
      {
#endif
                        int     tb;

                        tb = (tb_t) (ptbc->m_pbData[(ULONG)indInChunk]);
                        UnLock (ptbd->m_prgtbcbBuckets[side][iDirectory].m_lock);
                        return tb;
/* mob_pach */
#ifndef  __cplusplus
      }
#endif
                        }
                }
        // Not in the cache - have to read it from disk.
        // I decided to write simple code - so sometimes it's possible that
        // 2 threads will simultaneously read exactly the same chunk into 2
        // different cache entries. In that case, all subsequent cache probes
        // will hit the first cache entry, so the second one will 'drift' to
        // the end of general LRU list and will be reused.

        // Unlock cache bucket, so other threads can continue execution
        UnLock (ptbd->m_prgtbcbBuckets[side][iDirectory].m_lock);
        // First, find cache entry we can use
        Lock (lockLRU);
        // Get it either from a free list, or reuse last element of the LRU list
        if (NULL != ptbcFree)
                {
                ptbc = ptbcFree;
                ptbcFree = ptbc->m_ptbcNext;
                UnLock (lockLRU);
                }
        else
                {
                int             iTailDirectory;
                int             iTailTb;
                color   colorTail;

                assert (NULL != ptbcTail);
#if defined (SMP)
                // "Optimistic" model - assuming that there is low content
                // (not hundreds of threads)
                for (;;)
                        {
                        ptbc = ptbcTail;
                        iTailTb = ptbc->m_iTb;
                        iTailDirectory = TB_DIRECTORY_ENTRY (ptbc->m_indChunk);
                        colorTail = ptbc->m_color;
                        // To avoid deadlocks, have to first acquire cache buckets lock,
                        // and only then general LRU lock. So, free general LRU lock and
                        // acquire 2 locks in a proper order.
                        UnLock (lockLRU);
                        Lock (rgtbdDesc[iTailTb].m_prgtbcbBuckets[colorTail][iTailDirectory].m_lock);
                        Lock (lockLRU);
                        // Have structures been modified while we re-acquired locks?
                        // (to be more precise, it's Ok, if structures were modified,
                        // but cache entry again become the last element of the list,
                        // and TB, color, and cache bucket did not changed, so we locked
                        // proper locks).
                        if (ptbc == ptbcTail && ptbc->m_iTb == iTailTb &&
                                ptbc->m_color == colorTail &&
                                TB_DIRECTORY_ENTRY (ptbc->m_indChunk) == (unsigned) iTailDirectory)
                                break;
                        // Sorry - try once again...
                        UnLock (rgtbdDesc[iTailTb].m_prgtbcbBuckets[colorTail][iTailDirectory].m_lock);
                        }
#else
                ptbc = ptbcTail;
                iTailTb = ptbc->m_iTb;
                iTailDirectory = TB_DIRECTORY_ENTRY (ptbc->m_indChunk);
                colorTail = ptbc->m_color;
#endif

                // Remove cache entry from the general LRU list
                ptbcTail = ptbc->m_ptbcPrev;
                if (NULL == ptbcTail)
                        ptbcHead = NULL;
                else
                        ptbcTail->m_ptbcNext = NULL;
                UnLock (lockLRU);

                // Remove it from cache bucket list
                if (NULL != ptbc->m_ptbcTbNext)
                        ptbc->m_ptbcTbNext->m_ptbcTbPrev = ptbc->m_ptbcTbPrev;
                if (NULL == ptbc->m_ptbcTbPrev)
                        rgtbdDesc[iTailTb].m_prgtbcbBuckets[colorTail][iTailDirectory].m_ptbcFirst = ptbc->m_ptbcTbNext;
                else
                        ptbc->m_ptbcTbPrev->m_ptbcTbNext = ptbc->m_ptbcTbNext;
                UnLock (rgtbdDesc[iTailTb].m_prgtbcbBuckets[colorTail][iTailDirectory].m_lock);
                }

        // Ok, now we have "orphan" cache entry - it's excluded from all lists,
        // so other threads will never touch it.
        ptbc->m_iTb = iTb;
        ptbc->m_color = side;
        ptbc->m_indChunk = indChunk;

        // Now read it from the disk
/* mob_pach */
#ifndef  __cplusplus
{
#endif
        FILE    *fp;
        size_t  cb;

        // First, check: is necessary file opened?
        // As files are not thread-safe, lock file
        Lock (ptbd->m_rglockFiles[side]);
        fp = ptbd->m_rgfpFiles[side];
        if (NULL == fp)
                {
                // Not - try to open it
                fp = fopen (ptbd->m_rgpchFileName[side], "rb");
                if (NULL == fp)
                        {
                        // Failed. Close all the opened files and retry
                        UnLock (ptbd->m_rglockFiles[side]);
                        VTbCloseFiles ();
                        Lock (ptbd->m_rglockFiles[side]);
                        // Theoretically, it's possible that other threads opened a lot of
                        // files in the interval between VTbCloseFiles() and Lock(). If
                        // so, we'll fail - I don't like to have one more global lock
                        // especially for file open, at least not in first version.
                        // Problem can happen only on systems with small limit of
                        // simultaneously open files and high number of threads - unlikely
                        // combination.
                        fp = ptbd->m_rgfpFiles[side];
                        if (NULL == fp)
                                {
                                fp = fopen (ptbd->m_rgpchFileName[side], "rb");
                                if (NULL == fp)
                                        goto ERROR_LABEL;
                                }
                        }
                ptbd->m_rgfpFiles[side] = fp;
                }

        // File opened. Now seek and read necessary chunk
        if (NULL == ptbd->m_rgpdiDecodeInfo[side])
                {
                // Read uncompressed file
                if (fseek (fp, (long) indChunk*TB_CB_CACHE_CHUNK, SEEK_SET))
                        goto ERROR_LABEL;
                cb = fread (ptbc->m_pbData, 1, TB_CB_CACHE_CHUNK, fp);
                if (cb != TB_CB_CACHE_CHUNK)
                        {
                        // Could not read TB_CB_CACHE_CHUNK - check for error
                        if (ferror (fp) || ((ULONG) -1 == cb))
                                goto ERROR_LABEL;
                        }
                assert (cb > indChunk);
                UnLock (ptbd->m_rglockFiles[side]);
                }
        else
                {
                // Read compressed file
                int     fWasError;
                decode_block    *block;
                decode_info             *info = ptbd->m_rgpdiDecodeInfo[side];

#if defined (SMP)
                // Find free decode block
                decode_block    **pBlock;

                Lock (lockDecode);
                pBlock = rgpdbDecodeBlocks;
                while (NULL == *pBlock)
                        pBlock ++;
                block = *pBlock;
                *pBlock = NULL;
                UnLock (lockDecode);
#else
                block = rgpdbDecodeBlocks[0];
#endif

                // Initialize decode block and read chunk
                fWasError = 0 != comp_init_block (block, TB_CB_CACHE_CHUNK, ptbc->m_pbData) ||
                                        0 != comp_read_block (block, info, fp, indChunk);

                // Release lock on file, so other threads can proceed with that file
                UnLock (ptbd->m_rglockFiles[side]);

                // Decompress chunk
                if (!fWasError)
                        fWasError |= (0 != comp_decode_and_check_crc (block, info, block->orig.size, TB_CRC_CHECK));

                // Release block
#if defined (SMP)
                Lock (lockDecode);
                *pBlock = block;
                UnLock (lockDecode);
#endif

                // Read Ok?
                if (fWasError)
                        goto ERROR_LABEL_2;
                }
/* mob_pach */
#ifndef  __cplusplus
}
#endif

        // Read - now acquire locks and insert cache entry in both lists
        Lock (ptbd->m_prgtbcbBuckets[side][iDirectory].m_lock);
        Lock (lockLRU);

        // Insert cache entry into general LRU list
        ptbc->m_ptbcPrev = NULL;
        ptbc->m_ptbcNext = ptbcHead;
        if (NULL == ptbcHead)
                ptbcTail = ptbc;
        else
                ptbcHead->m_ptbcPrev = ptbc;
        ptbcHead = ptbc;

        // Insert cache entry into cache bucket LRU list
        ptbc->m_ptbcTbPrev = NULL;
        ptbc->m_ptbcTbNext = ptbd->m_prgtbcbBuckets[side][iDirectory].m_ptbcFirst;
        if (NULL != ptbc->m_ptbcTbNext)
                ptbc->m_ptbcTbNext->m_ptbcTbPrev = ptbc;
        ptbd->m_prgtbcbBuckets[side][iDirectory].m_ptbcFirst = ptbc;

        // All done

/* mob_pach */
#ifndef  __cplusplus
{
#endif
        int tb;

        tb = (tb_t) (ptbc->m_pbData[indInChunk]);
        // Release locks
        UnLock (ptbd->m_prgtbcbBuckets[side][iDirectory].m_lock);
        UnLock (lockLRU);
        return tb;
/* mob_pach */
#ifndef  __cplusplus
}
#endif

        // I/O error. Here I don't want to halt the program, because that can
        // happen in the middle of the important game. Just return failure.
ERROR_LABEL:
        UnLock (ptbd->m_rglockFiles[side]);
ERROR_LABEL_2:
        Lock (lockLRU);
        ptbd->m_rgpchFileName[side] = NULL;
        ptbc->m_ptbcNext = ptbcFree;
        ptbcFree = ptbc;
        UnLock (lockLRU);
        return L_bev_broken;
        }

// Probe TB - upper level function

static int      TB_FASTCALL TbtProbeTable
        (
        int             iTb,
        color   side,
        INDEX   indOffset
        )
        {
        CTbDesc *ptbd;

        assert (iTb > 0 && iTb < cTb);
        ptbd = & rgtbdDesc[iTb];
        if (ptbd->m_fSymmetric)
                side = x_colorWhite;

        // It's better for offset be smaller than TB size
        assert (!FRegistered (iTb, side) || indOffset < ptbd->m_rgcbLength[side]);

        // Entire file read/mapped to memory?
        if (NULL != ptbd->m_rgpbRead[side])
                return (tb_t) ptbd->m_rgpbRead[side][indOffset];

        // Cache initialized? TB registered?
        if (0 == ctbcTbCache || NULL == ptbd->m_prgtbcbBuckets[side])
                return bev_broken;

#if defined (T33_INCLUDE) || defined (KPPKP_16BIT)
    if (ptbd->m_f16bit)
        return bev_broken;
#endif

/* mob_pach */
#ifndef  __cplusplus
{
#endif
        int tb;

/* mob_pach */
#ifndef  __cplusplus
        tb = TbtProbeTable4 (iTb, side, TB_CHUNK (indOffset), TB_OFFSET (indOffset));
#else
        tb = TbtProbeTable (iTb, side, TB_CHUNK (indOffset), TB_OFFSET (indOffset));
#endif
        return (L_bev_broken == tb) ? bev_broken : (tb_t) tb;
/* mob_pach */
#ifndef  __cplusplus
}
#endif
        }

// 16-bit version (recommended)

#define FOutOfBound(iTb, side, indOffset) (tbid_kppkp == iTb && x_colorBlack == side && (indOffset == 0x0362BC7C || indOffset == 0x0362DE44 || indOffset == 0x03637648 || indOffset == 0x03639810 || indOffset == 0x038D4F29 || indOffset == 0x040A2CAB || indOffset == 0x043C778C))

/* mob_pach */
#ifndef  __cplusplus
int TB_FASTCALL L_TbtProbeTable
#else
extern "C" int TB_FASTCALL L_TbtProbeTable
#endif
        (
        int             iTb,
        color   side,
        INDEX   indOffset
        )
        {
        int     tbtScore;
        CTbDesc *ptbd;

        assert (iTb > 0 && iTb < cTb);
        ptbd = & rgtbdDesc[iTb];
        if (ptbd->m_fSymmetric)
                side = x_colorWhite;

        // Entire file read/mapped to memory?
        if (NULL != ptbd->m_rgpbRead[side])
        {
#if defined (KPPKP_16BIT)
        if (!ptbd->m_f16bit)
                        {
                        tbtScore = (tb_t) ptbd->m_rgpbRead[side][indOffset];
                return S_to_L (tbtScore);
                        }
        else
                return (((int) (((signed char) ptbd->m_rgpbRead[side][indOffset*2+1]))) << 8) +
                    ptbd->m_rgpbRead[side][indOffset*2];
#elif defined (T33_INCLUDE)
        if (!ptbd->m_f16bit)
            {
                    if (FOutOfBound (iTb, side, indOffset))
                            return -32639;
            else
                                {
                                tbtScore = (tb_t) ptbd->m_rgpbRead[side][indOffset];
                    return S_to_L (tbtScore);
                                }
            }
        else
                return (((int) (((signed char) ptbd->m_rgpbRead[side][indOffset*2+1]))) << 8) +
                    ptbd->m_rgpbRead[side][indOffset*2];
#else
                if (FOutOfBound (iTb, side, indOffset))
                        return -32639;
        else
                        {
                        tbtScore = (tb_t) ptbd->m_rgpbRead[side][indOffset];
                return S_to_L (tbtScore);
                        }
#endif
        }

        // Cache initialized? TB registered?
        if (0 == ctbcTbCache || NULL == ptbd->m_prgtbcbBuckets[side])
                return L_bev_broken;

#if defined (T33_INCLUDE)
        if (ptbd->m_f16bit)
                {
                // Inefficient, but very simple, code
                int     iLo;
		int     iHi;
#undef UNUSED_VARS	       
#ifdef UNUSED_VARS		   
                INDEX   indChunk;
                INDEX   indInChunk;
#endif
/* mob_pach */
#ifndef  __cplusplus
                iLo = TbtProbeTable4 (iTb, side, WIDE_TB_CHUNK (indOffset), WIDE_TB_OFFSET (indOffset));
                iHi = TbtProbeTable4 (iTb, side, WIDE_TB_CHUNK (indOffset), WIDE_TB_OFFSET (indOffset)+1);
#else
                iLo = TbtProbeTable (iTb, side, WIDE_TB_CHUNK (indOffset), WIDE_TB_OFFSET (indOffset));
                iHi = TbtProbeTable (iTb, side, WIDE_TB_CHUNK (indOffset), WIDE_TB_OFFSET (indOffset)+1);
#endif
        tbtScore = (L_bev_broken == iLo || L_bev_broken == iHi) ? L_bev_broken : ((iHi << 8) + (iLo & 0xFF));
                }
        else
                {
#if !defined (KPPKP_16BIT)
                if (FOutOfBound (iTb, side, indOffset))
                        return -32639;
#endif
/* mob_pach */
#ifndef  __cplusplus
                tbtScore = TbtProbeTable4 (iTb, side, TB_CHUNK (indOffset), TB_OFFSET (indOffset));
#else
                tbtScore = TbtProbeTable (iTb, side, TB_CHUNK (indOffset), TB_OFFSET (indOffset));
#endif
                tbtScore = L_bev_broken == tbtScore ? L_bev_broken : S_to_L (tbtScore);
                }
#elif !defined (KPPKP_16BIT)
        if (FOutOfBound (iTb, side, indOffset))
                return -32639;
        tbtScore = TbtProbeTable (iTb, side, TB_CHUNK (indOffset), TB_OFFSET (indOffset));
        tbtScore = L_bev_broken == tbtScore ? L_bev_broken : S_to_L (tbtScore);
#else
        if (tbid_kppkp != iTb)
                {
                // All tables but kppkp are 8-bit tables
                tbtScore = TbtProbeTable (iTb, side, TB_CHUNK (indOffset), TB_OFFSET (indOffset));
                tbtScore = L_bev_broken == tbtScore ? L_bev_broken : S_to_L (tbtScore);
                }
        else
                {
                // Special handling of kppkp - it's 16-bit table
                // Inefficient, but very simple, code
                int     iLo;
                int     iHi;

                iLo = TbtProbeTable (iTb, side, WIDE_TB_CHUNK (indOffset), WIDE_TB_OFFSET (indOffset));
                iHi = TbtProbeTable (iTb, side, WIDE_TB_CHUNK (indOffset), WIDE_TB_OFFSET (indOffset)+1);
        tbtScore = (L_bev_broken == iLo || L_bev_broken == iHi) ? L_bev_broken : ((iHi << 8) + (iLo & 0xFF));
                }
#endif
        return tbtScore;
        }

//-----------------------------------------------------------------------------
//
//      Global initialization

int FCheckExistance
        (
        char    *pszPath,
        int             iTb,
        color   side
        )
        {
        FILE                    *fp;
        char                    *pchCopy;
        const char              *pchExt = PchExt (side);
        char                    rgchTbName[256];
        CTbCacheBucket  *prgtbcbBuckets;
        INDEX                   cb;
        decode_info             *comp_info = NULL;

        if (FRegistered (iTb, side) || NULL != rgtbdDesc[iTb].m_rgpbRead[side])
                return true;

        strcpy (rgchTbName, pszPath);
        if (0 != pszPath[0])
                {
#if defined (_WIN32)
                strcat (rgchTbName, "\\");
#elif defined (__MWERKS__)
                strcat (rgchTbName, ":");
#else
                strcat (rgchTbName, "/");
#endif
                }
        strcat (rgchTbName, rgtbdDesc[iTb].m_rgchName);
        strcat (rgchTbName, pchExt);
        fp = fopen (rgchTbName, "rb");
#if !defined (NEW) && !defined (_WIN32)
        // For case-sensitive systems, have to try once more
        if (NULL == fp)
                {
/* mob_pach */
#ifndef  __cplusplus
   {
                int i;
      for (i = strchr(rgchTbName,'.')-rgchTbName-1; i>=0 && isalpha(rgchTbName[i]); i--)
#else
                for (int i = strchr(rgchTbName,'.')-rgchTbName-1; i>=0 && isalpha(rgchTbName[i]); i--)
#endif
                        rgchTbName[i] = toupper (rgchTbName[i]);
/* mob_pach */
#ifndef  __cplusplus
   }
#endif
                fp = fopen (rgchTbName, "rb");
                }
#endif
        if (NULL != fp)
                {
                // Found uncompressed table
                if (rgtbdDesc[iTb].m_f16bit) // Do not check the length for 16-bit file
                        cb = rgtbdDesc[iTb].m_rgcbLength[side];
                else
            {
            if (0 != fseek (fp, 0L, SEEK_END))
                            {
                            printf ("*** Seek in %s failed\n", rgchTbName);
                            exit (1);
                            }
                    cb = (INDEX) ftell (fp);
#if defined (NEW)
                    if (0 != rgtbdDesc[iTb].m_rgcbLength[side] && cb != rgtbdDesc[iTb].m_rgcbLength[side])
                            {
                            printf ("*** %s corrupted\n", rgchTbName);
                            exit (1);
                            }
            }
#endif
                }
        else
                {
                // Check for compressed table.
                // First, check for kxykz.nb?.emd
                strcat (rgchTbName, ".emd");
                fp = fopen (rgchTbName, "rb");
                if (NULL == fp)
                        {
                        // Check for kxykz_nb?.emd
                        rgchTbName [strlen(rgchTbName)-8] = '_';
                        fp = fopen (rgchTbName, "rb");
                        }
                if (NULL == fp)
                        {
                        // Check for kxykznb?.emd (8+3 format)
                        int     cch;

                        cch = strlen (rgchTbName);
                        memmove (rgchTbName+cch-8, rgchTbName+cch-7, 8);
                        fp = fopen (rgchTbName, "rb");
                        if (NULL == fp)
                                return false;
                        }
                cCompressed ++;
/* mob_pach */
#ifndef  __cplusplus
   {
#endif
                int iResult = comp_open_file (&comp_info, fp, TB_CRC_CHECK);
                if (0 != iResult)
                        {
                        printf ("*** Unable to read %s - ", rgchTbName);
                        switch (iResult & 0xFF)
                                {
                        case COMP_ERR_READ:
                                printf ("read error\n");
                                break;
                        case COMP_ERR_NOMEM:
                                printf ("out of memory\n");
                                break;
                        case COMP_ERR_BROKEN:
                                printf ("file broken\n");
                                break;
                        default:
                                printf ("error %d\n", iResult);
                                break;
                                }
                        exit (1);
                        }
/* mob_pach */
#ifndef  __cplusplus
   }
#endif
                if (comp_info->block_size != TB_CB_CACHE_CHUNK)
                        {
                        printf ("*** %s: Unsupported block size %d\n", rgchTbName, comp_info->block_size);
                        exit (1);
                        }
        if (rgtbdDesc[iTb].m_f16bit)
            {
                cb = comp_info->block_size/2*(comp_info->n_blk-1) + comp_info->last_block_size/2;
#if defined (NEW)
                if (0 != rgtbdDesc[iTb].m_rgcbLength[side] && cb != rgtbdDesc[iTb].m_rgcbLength[side])
                        {
                            printf ("*** %s corrupted\n", rgchTbName);
                        exit (1);
                        }
#endif
                        }
                else
            {
                cb = comp_info->block_size*(comp_info->n_blk-1) + comp_info->last_block_size;
#if defined (NEW)
                if (0 != rgtbdDesc[iTb].m_rgcbLength[side] && cb != rgtbdDesc[iTb].m_rgcbLength[side])
                        {
                            printf ("*** %s corrupted\n", rgchTbName);
                        exit (1);
                        }
#endif
            }
                }
        rgtbdDesc[iTb].m_rgcbLength[side] = cb;
        fclose (fp);
        if (FRegisterTb (& (rgtbdDesc[iTb])))
                {
                pchCopy = (char*) PvMalloc (strlen(rgchTbName)+1);
                strcpy (pchCopy, rgchTbName);
                rgtbdDesc[iTb].m_rgpchFileName[side] = pchCopy;
                prgtbcbBuckets = (CTbCacheBucket*) PvMalloc (TB_DIRECTORY_SIZE*sizeof(CTbCacheBucket));
                memset (prgtbcbBuckets, 0, TB_DIRECTORY_SIZE*sizeof(CTbCacheBucket));
#if defined (SMP)
/* mob_pach */
#ifndef  __cplusplus
   {
                int i;
      for (i = 0; i < TB_DIRECTORY_SIZE; i ++)
#else
                for (int i = 0; i < TB_DIRECTORY_SIZE; i ++)
#endif
                        LockInit (prgtbcbBuckets[i].m_lock);
/* mob_pach */
#ifndef  __cplusplus
   }
#endif
#endif
                rgtbdDesc[iTb].m_prgtbcbBuckets[side] = prgtbcbBuckets;
                rgtbdDesc[iTb].m_rgpdiDecodeInfo[side] = comp_info;
                if (fVerbose)
                        printf ("%s registered\n", pchCopy);
                return true;
                }
        else
                {
                printf ("*** Unable to register %s\n", rgchTbName);
                exit (1);
                }
        return false;
        }

/* mob_pach */
#ifndef  __cplusplus
int IInitializeTb
#else
extern "C" int IInitializeTb
#endif
        (
        char *pszPath
        )
        {
        char    szTemp[1024];
        color   sd;
        int             iTb, iMaxTb, i;

        cbAllocated = cbEGTBCompBytes = 0;
        // If there are open files, close those
        VTbCloseFiles ();
#if defined (SMP)
        // Init all locks
        LockInit (lockLRU);
        LockInit (lockDecode);
        for (iTb = 1; iTb < cTb; iTb ++)
                {
                LockInit (rgtbdDesc[iTb].m_rglockFiles[x_colorWhite]);
                LockInit (rgtbdDesc[iTb].m_rglockFiles[x_colorBlack]);
                }
#endif
#if defined (NEW)
        // Create enumeration tables
        VInitEnumerations ();
#endif
        // Create empty TB search table
        VCreateEmptyTbTable ();
        // Free memory from TB table
        for (iTb = 1; iTb < cTb; iTb ++)
                {
                for (sd = x_colorWhite; sd <= x_colorBlack; sd = (color) (sd + 1))
                        {
                        if (NULL != rgtbdDesc[iTb].m_prgtbcbBuckets[sd] &&
                                NULL == rgtbdDesc[iTb].m_rgpbRead[sd])
                                {
                                free (rgtbdDesc[iTb].m_prgtbcbBuckets[sd]);
                                rgtbdDesc[iTb].m_prgtbcbBuckets[sd] = NULL;
                                }
                        if (NULL != rgtbdDesc[iTb].m_rgpchFileName[sd])
                                {
                                free (rgtbdDesc[iTb].m_rgpchFileName[sd]);
                                rgtbdDesc[iTb].m_rgpchFileName[sd] = NULL;
                                }
                        if (NULL != rgtbdDesc[iTb].m_rgpdiDecodeInfo[sd])
                                {
                                free (rgtbdDesc[iTb].m_rgpdiDecodeInfo[sd]);
                                rgtbdDesc[iTb].m_rgpdiDecodeInfo[sd] = NULL;
                                }
                        }
                }
        // Free compressed blocks
        for (i = 0; i < CPUS; i ++)
                {
                if (NULL != rgpdbDecodeBlocks[i])
                        {
                        free (rgpdbDecodeBlocks[i]);
                        rgpdbDecodeBlocks[i] = NULL;
                        }
                }
        // Search for existing TBs
        iMaxTb = 0;
        for (;;)
                {
                for (i = 0; pszPath[i] != '\0' && pszPath[i] != ',' && pszPath[i] != ';'
#if !defined (_WIN32) && !defined (__MWERKS__)
                         && pszPath[i] != ':'
#endif
                         ; i ++)
                        {
                        szTemp[i] = pszPath[i];
                        }
                szTemp[i] = '\0';
                for (iTb = 1; iTb < cTb; iTb ++)
                        {
                        if (FCheckExistance (szTemp, iTb, x_colorWhite))
                                {
                                if (iTb > iMaxTb)
                                        iMaxTb = iTb;
                                }
                if (! rgtbdDesc[iTb].m_fSymmetric && FCheckExistance (szTemp, iTb, x_colorBlack))
                                {
                                if (iTb > iMaxTb)
                                        iMaxTb = iTb;
                                }
                        }
                pszPath += i;
                if ('\0' == *pszPath)
                        break;
                pszPath ++;
                }

        // If there were compressed files, have to allocate buffer(s)
        if (0 != cCompressed)
                {
                for (i = 0; i < CPUS; i ++)
                        {
                        int iResult = comp_alloc_block (&rgpdbDecodeBlocks[i], TB_CB_CACHE_CHUNK);
                        if (0 != iResult)
                                {
                                printf ("*** Cannot allocate decode block: error code %d\n", iResult);
                                exit (1);
                                }
                        }
                if (fVerbose)
                        printf ("Allocated %dKb for decompression tables, indices, and buffers.\n",
                                        (cbEGTBCompBytes+1023)/1024);
                }

        // All done!
#if defined T33_INCLUDE
        if (iMaxTb >= tbid_knnknn)
                return 6;
#endif
        if (iMaxTb >= tbid_kppkp)
                return 5;
        if (iMaxTb >= tbid_kpkp)
                return 4;
        if (iMaxTb >= tbid_kpk)
                return 3;
        return 0;
}
