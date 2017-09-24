# This module tries to find libWebsockets library and include files
#
# LIBWEBSOCKETS_INCLUDE_DIR, path where to find libwebsockets.h
# LIBWEBSOCKETS_LIBRARIES, the library to link against
# LIBWEBSOCKETS_FOUND, If false, do not try to use libWebSockets

FIND_PATH(LIBWEBSOCKETS_INCLUDE_DIR libwebsockets.h)

FIND_LIBRARY(LIBWEBSOCKETS_LIBRARIES websockets)

# handle the QUIETLY and REQUIRED arguments and set ALLEGRO5_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBWEBSOCKETS DEFAULT_MSG LIBWEBSOCKETS_LIBRARIES LIBWEBSOCKETS_INCLUDE_DIR)

MARK_AS_ADVANCED(LIBWEBSOCKETS_LIBRARIES LIBWEBSOCKETS_INCLUDE_DIR )
