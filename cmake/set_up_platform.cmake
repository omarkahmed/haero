# This macro identifies compilers and third-party library needs
# for particular hosts.
macro(set_up_platform)

  # Are we on Linux?
  if (UNIX AND NOT APPLE)
    set(LINUX ON)
  endif()

  # Do we have bash?
  find_program(BASH bash)
  if (BASH STREQUAL "BASH_NOTFOUND")
    message(FATAL_ERROR "Bash is required, but is not available on this system.")
  endif()

  # Do we have make?
  find_program(MAKE make)
  if (MAKE STREQUAL "MAKE_NOTFOUND")
    message(FATAL_ERROR "Make is required, but is not available on this system.")
  endif()

  # Do we have git?
  find_program(GIT git)
  if (GIT STREQUAL "GIT_NOTFOUND")
    message(WARNING "Git not found. Hope you're not developing on this system.")
    set(HAVE_GIT FALSE)
  else()
    set(HAVE_GIT TRUE)
  endif()

  include(GNUInstallDirs)

  if (EKAT_INCLUDE_DIR)
    if (NOT EXISTS ${EKAT_INCLUDE_DIR})
      message(FATAL_ERROR "Couldn't find EKAT include dir at ${EKAT_INCLUDE_DIR}.")
    endif()
    message(STATUS "Using EKAT include dir: ${EKAT_INCLUDE_DIR}.")
  else()
    set(EKAT_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
  endif()
  if (EKAT_LIBRARY)
    if (NOT EXISTS ${EKAT_LIBRARY})
      message(FATAL_ERROR "Couldn't find EKAT library at ${EKAT_LIBRARY}.")
    endif()
    message(STATUS "Using EKAT library at ${EKAT_LIBRARY}.")
    set(HAERO_BUILDS_EKAT OFF)
  else()
    set(EKAT_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/externals/ekat/src/ekat")
    set(EKAT_LIBRARY "${EKAT_LIBRARY_DIR}/libekat.a")
    message(STATUS "Building EKAT library: ${EKAT_LIBRARY}.")
    set(HAERO_BUILDS_EKAT ON)
  endif()
  get_filename_component(EKAT_LIBRARY_DIR ${EKAT_LIBRARY} DIRECTORY)

  # The following libraries are only needed when chemistry is enabled.
  if (HAERO_ENABLE_CHEMISTRY)
    if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Intel")
      if (OPENBLAS_INCLUDE_DIR)
        if (NOT EXISTS ${OPENBLAS_INCLUDE_DIR})
          message(FATAL_ERROR "Couldn't find OpenBLAS include dir at ${OPENBLAS_INCLUDE_DIR}.")
        endif()
        message(STATUS "Using OpenBLAS include dir: ${OPENBLAS_INCLUDE_DIR}.")
      else()
        set(OPENBLAS_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
      endif()
      if (OPENBLAS_LIBRARY)
        if (NOT EXISTS ${OPENBLAS_LIBRARY})
          message(FATAL_ERROR "Couldn't find OpenBLAS library at ${OPENBLAS_LIBRARY}.")
        endif()
        message(STATUS "Using OpenBLAS library at ${OPENBLAS_LIBRARY}.")
      else()
        # OpenBLAS gets installed in the lib directory--it doesn't know about lib64.
        set(OPENBLAS_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/lib")
        set(OPENBLAS_LIBRARY ${OPENBLAS_LIBRARY_DIR}/libopenblas.a)
        message(STATUS "Building OpenBLAS library: ${OPENBLAS_LIBRARY}.")
      endif()
      get_filename_component(OPENBLAS_LIBRARY_DIR ${OPENBLAS_LIBRARY} DIRECTORY)
    endif()

    if (TINES_INCLUDE_DIR)
      if (NOT EXISTS ${TINES_INCLUDE_DIR})
        message(FATAL_ERROR "Couldn't find Tines include dir at ${TINES_INCLUDE_DIR}.")
      endif()
      message(STATUS "Using Tines include dir: ${TINES_INCLUDE_DIR}.")
    else()
      set(TINES_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
    endif()
    if (TINES_LIBRARY)
      if (NOT EXISTS ${TINES_LIBRARY})
        message(FATAL_ERROR "Couldn't find Tines library at ${TINES_LIBRARY}.")
      endif()
      message(STATUS "Using Tines library at ${TINES_LIBRARY}.")
    else()
      set(TINES_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
      set(TINES_LIBRARY ${TINES_LIBRARY_DIR}/libtines.a)
      message(STATUS "Building Tines library: ${TINES_LIBRARY}.")
    endif()
    get_filename_component(TINES_LIBRARY_DIR ${TINES_LIBRARY} DIRECTORY)

    if (TCHEM_INCLUDE_DIR)
      if (NOT EXISTS ${TCHEM_INCLUDE_DIR})
        message(FATAL_ERROR "Couldn't find TChem include dir at ${TCHEM_INCLUDE_DIR}.")
      endif()
      message(STATUS "Using TChem include dir: ${TCHEM_INCLUDE_DIR}.")
    else()
      set(TCHEM_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
    endif()
    if (TCHEM_LIBRARY)
      if (NOT EXISTS ${TCHEM_LIBRARY})
        message(FATAL_ERROR "Couldn't find TChem library at ${TCHEM_LIBRARY}.")
      endif()
      message(STATUS "Using TChem library at ${TCHEM_LIBRARY}.")
    else()
      set(TCHEM_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
      set(TCHEM_LIBRARY ${TCHEM_LIBRARY_DIR}/libtchem.a)
      message(STATUS "Building TChem library: ${TCHEM_LIBRARY}.")
    endif()
    get_filename_component(TCHEM_LIBRARY_DIR ${TCHEM_LIBRARY} DIRECTORY)
  endif()

  if (SKYWALKER_INCLUDE_DIR)
    if (NOT EXISTS ${SKYWALKER_INCLUDE_DIR})
      message(FATAL_ERROR "Couldn't find Skywalker include dir at ${SKYWALKER_INCLUDE_DIR}.")
    endif()
    message(STATUS "Using Skywalker include dir: ${SKYWALKER_INCLUDE_DIR}.")
  else()
    set(SKYWALKER_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
  endif()
  if (SKYWALKER_LIBRARY)
    if (NOT EXISTS ${SKYWALKER_LIBRARY})
      message(FATAL_ERROR "Couldn't find Skywalker library at ${SKYWALKER_LIBRARY}.")
    endif()
    message(STATUS "Using Skywalker library at ${SKYWALKER_LIBRARY}.")
  else()
    set(SKYWALKER_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
    set(SKYWALKER_LIBRARY ${SKYWALKER_LIBRARY_DIR}/libskywalker_${HAERO_PRECISION}.a)
    message(STATUS "Building Skywalker library: ${SKYWALKER_LIBRARY}.")
  endif()
  if (SKYWALKER_F90_LIBRARY)
    if (NOT EXISTS ${SKYWALKER_F90_LIBRARY})
      message(FATAL_ERROR "Couldn't find Skywalker Fortran library at ${SKYWALKER_F90_LIBRARY}.")
    endif()
    message(STATUS "Using Skywalker Fortran library at ${SKYWALKER_F90_LIBRARY}.")
  else()
    set(SKYWALKER_F90_LIBRARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
    set(SKYWALKER_F90_LIBRARY ${SKYWALKER_F90_LIBRARY_DIR}/libskywalker_f90_${HAERO_PRECISION}.a)
    message(STATUS "Building Skywalker Fortran library: ${SKYWALKER_F90_LIBRARY}.")
  endif()
  get_filename_component(SKYWALKER_LIBRARY_DIR ${SKYWALKER_LIBRARY} DIRECTORY)
  get_filename_component(SKYWALKER_F90_LIBRARY_DIR ${SKYWALKER_F90_LIBRARY} DIRECTORY)

  if (HAERO_ENABLE_MAM4BOX)
    if (NOT MAM4BOX_DIR)
      message(FATAL_ERROR "HAERO_ENABLE_MAM4BOX option is set, but MAM4BOX_DIR is not given!")
    endif()
    if (NOT EXISTS ${MAM4BOX_DIR})
      message(FATAL_ERROR "Invalid MAM4BOX_DIR: ${MAM4BOX_DIR}")
    endif()
    if (NOT EXISTS ${MAM4BOX_DIR}/CMakeLists.txt)
      message(FATAL_ERROR "No CMakeLists.txt file found in ${MAM4BOX_DIR}")
    else()
      message(STATUS "Building MAM4 box model: ${MAM4BOX_DIR}")
    endif()
  endif()

  if (APPLE)
    set(NEED_LAPACK FALSE)
  else()
    set(NEED_LAPACK TRUE)
  endif()

  # Certain tools (e.g. patch) require TMPDIR to be defined. If it is not,
  # we do so here.
  set(TMPDIR_VAR $ENV{TMPDIR})
  if (NOT TMPDIR_VAR)
    set(ENV{TMPDIR} "/tmp")
  endif()

endmacro()
