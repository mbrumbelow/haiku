#if !defined(_GRAPHIC_DRIVER_H_)
#define _GRAPHIC_DRIVER_H_

#include <Drivers.h>

/* The API for driver access is C, not C++ */

#ifdef __cplusplus
extern "C" {
#endif


// Haiku / BeOS Graphics calls
enum {
	B_GET_ACCELERANT_SIGNATURE = B_GRAPHIC_DRIVER_BASE
};
// DRM IOCTL's start at 8400


#ifdef __cplusplus
}
#endif

#endif
