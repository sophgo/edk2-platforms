#ifndef _RESTORE_DEFAULTS_H_
#define _RESTORE_DEFAULTS_H_

#include <Uefi.h>

typedef struct _RESTORE_PROTOCOL {
    EFI_STATUS (EFIAPI *RestoreDefaults)(VOID);
} RESTORE_PROTOCOL;

#endif // _RESTORE_DEFAULTS_H_

