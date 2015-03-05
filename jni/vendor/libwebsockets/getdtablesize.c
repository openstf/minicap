#include <unistd.h>

// This has apparently been deprecated and removed from the SDK, but
// libwebsockets still uses it. Provide this instead.
int getdtablesize(void) {
  return sysconf(_SC_OPEN_MAX);
}
