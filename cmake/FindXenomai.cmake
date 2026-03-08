# cmake/FindXenomai.cmake
# Locates the Xenomai POSIX skin using xeno-config.
#
# Imported targets:
#   Xenomai::POSIX   - compile flags + link libraries for the POSIX skin
#
# Variables set:
#   XENOMAI_FOUND       - TRUE if Xenomai was found
#   XENOMAI_CFLAGS      - compiler flags (list)
#   XENOMAI_LDFLAGS     - linker flags  (list)

find_program(XENO_CONFIG_EXECUTABLE
    NAMES xeno-config
    DOC   "Xenomai configuration utility"
)

if(NOT XENO_CONFIG_EXECUTABLE)
    if(Xenomai_FIND_REQUIRED)
        message(FATAL_ERROR "xeno-config not found. Install Xenomai or set PATH correctly.")
    else()
        message(STATUS "xeno-config not found — Xenomai disabled.")
        set(XENOMAI_FOUND FALSE)
        return()
    endif()
endif()

execute_process(
    COMMAND ${XENO_CONFIG_EXECUTABLE} --skin=posix --cflags
    OUTPUT_VARIABLE _raw_cflags
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _cflags_result
)

execute_process(
    COMMAND ${XENO_CONFIG_EXECUTABLE} --skin=posix --ldflags
    OUTPUT_VARIABLE _raw_ldflags
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _ldflags_result
)

if(_cflags_result OR _ldflags_result)
    if(Xenomai_FIND_REQUIRED)
        message(FATAL_ERROR "xeno-config invocation failed.")
    endif()
    set(XENOMAI_FOUND FALSE)
    return()
endif()

separate_arguments(XENOMAI_CFLAGS  UNIX_COMMAND "${_raw_cflags}")
separate_arguments(XENOMAI_LDFLAGS UNIX_COMMAND "${_raw_ldflags}")

set(XENOMAI_FOUND TRUE)
message(STATUS "Found Xenomai: ${XENO_CONFIG_EXECUTABLE}")

# Imported target
if(NOT TARGET Xenomai::POSIX)
    add_library(Xenomai::POSIX INTERFACE IMPORTED)
    target_compile_options(Xenomai::POSIX INTERFACE ${XENOMAI_CFLAGS})
    target_link_libraries(Xenomai::POSIX  INTERFACE ${XENOMAI_LDFLAGS})
endif()

mark_as_advanced(XENO_CONFIG_EXECUTABLE)
