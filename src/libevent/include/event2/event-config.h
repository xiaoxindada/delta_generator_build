/* We include this to get __BIONIC__,
 * but this means _BSD_SOURCE and/or _GNU_SOURCE
 * must be set in the .bp file. */
#include <sys/cdefs.h>

#if defined(__BIONIC__)
#  include <event2/event-config-bionic.h>
#else
#  if defined(__linux__)
#    include <event2/event-config-linux.h>
#  elif defined(__APPLE__)
#    include <event2/event-config-darwin.h>
#  else
#    error No event-config.h suitable for this distribution.
#  endif
#endif  /* ifdef __BIONIC__ */
