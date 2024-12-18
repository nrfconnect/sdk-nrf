#include "ncs_version.h"

#define REVSTR2(x)    #x
#define REVSTR(x)     REVSTR2(x)

/* the configured options and settings */
#define HW_VERSION_MAJOR 3
#define HW_VERSION_MINOR 0
#define FW_VERSION_MAJOR NCS_VERSION_MAJOR
#define FW_VERSION_MINOR NCS_VERSION_MINOR
#define FW_VERSION_PATCH NCS_PATCHLEVEL
#define FW_VERSION_HASH REVSTR()

#define HW_REV_STRING "v" REVSTR(HW_VERSION_MAJOR) "." REVSTR(HW_VERSION_MINOR)
#define FW_REV_STRING_FMT "v%d.%d.%d.%s"
#define BUILT_STRING REVSTR(__DATE__) " " REVSTR(__TIME__)
