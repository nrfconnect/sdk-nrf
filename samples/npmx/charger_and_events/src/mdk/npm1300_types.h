/*

{License}

*/

#ifndef NPM1300_TYPES_H
#define NPM1300_TYPES_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdint.h>
#include "compiler_abstraction.h"


/* ============================================ Include required type specifiers ============================================= */

#ifndef __I
  #ifdef __cplusplus
    #define __I     volatile                         /*!< Defines 'read only' permissions                                      */
  #else
    #define __I     volatile const                   /*!< Defines 'read only' permissions                                      */
  #endif
#endif
#ifndef __O
  #define __O     volatile                           /*!< Defines 'write only' permissions                                     */
#endif
#ifndef __IO
  #define __IO    volatile                           /*!< Defines 'read / write' permissions                                   */
#endif

/* The following defines should be used for structure members */
#ifndef __IM
  #define __IM     volatile const                    /*!< Defines 'read only' structure member permissions                     */
#endif
#ifndef __OM
  #define __OM     volatile                          /*!< Defines 'write only' structure member permissions                    */
#endif
#ifndef __IOM
  #define __IOM    volatile                          /*!< Defines 'read / write' structure member permissions                  */
#endif


/* ========================================= Start of section using anonymous unions ========================================= */

#include "compiler_abstraction.h"

#if defined (__CC_ARM)
  #pragma anon_unions
#elif defined (__ICCARM__)
  #pragma language=extended
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wc11-extensions"
  #pragma clang diagnostic ignored "-Wreserved-id-macro"
  #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
  #pragma clang diagnostic ignored "-Wnested-anon-types"
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning 586
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#else
  #warning Unsupported compiler type
#endif

/* =========================================================================================================================== */
/* ================                                    Peripherals Section                                    ================ */
/* =========================================================================================================================== */


/* ========================================== End of section using anonymous unions ========================================== */

#if defined (__CC_ARM)
  #pragma pop
#elif defined (__ICCARM__)
  /* leave anonymous unions enabled */
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #pragma clang diagnostic pop
#elif defined (__GNUC__)
  /* anonymous unions are enabled by default */
#elif defined (__TMS470__)
  /* anonymous unions are enabled by default */
#elif defined (__TASKING__)
  #pragma warning restore
#elif defined (__CSMC__)
  /* anonymous unions are enabled by default */
#endif


#ifdef __cplusplus
}
#endif
#endif /* NPM1300_TYPES_H */

