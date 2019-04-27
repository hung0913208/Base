if(CMAKE_C_COMPILER_LOADED)
  set(FOUND_C_ALIGNED_ALLOC_IMPL 0)

  include(CheckCSourceCompiles)
  check_c_source_compiles("
    #include <stdlib.h>

    int main()
    {
      aligned_alloc(64, 128);
      return 0;
    }" HAS_C_ALIGNED_ALLOC)

  if (HAS_C_ALIGNED_ALLOC)
    set(FOUND_C_ALIGNED_ALLOC_IMPL 1)
    add_definitions(-DHAS_C_ALIGNED_ALLOC=1)
  endif()

  if (NOT FOUND_C_ALIGNED_ALLOC_IMPL AND UNIX)
    check_c_source_compiles("
      #include <cstdlib>

      int main()
      {
        void* ptr;
        posix_memalign(&ptr, 64, 128);
        return 0;
      }" HAS_C_POSIX_MEMALIGN)

    if (HAS_C_POSIX_MEMALIGN)
      set(FOUND_C_ALIGNED_ALLOC_IMPL 1)
      add_definitions(-DHAS_C_POSIX_MEMALIGN=1)
    endif()
  endif()

  if (NOT FOUND_C_ALIGNED_ALLOC_IMPL AND WIN32)
    check_c_source_compiles("
      #include <malloc.h>

      int main()
      {
        _aligned_malloc(128, 64);
        return 0;
      }" HAS_C_ALIGNED_MALLOC)

    if (HAS_C_ALIGNED_MALLOC)
      set(FOUND_C_ALIGNED_ALLOC_IMPL 1)
      add_definitions(-DHAS_C_ALIGNED_MALLOC=1)
    endif()
  endif()
endif()

if(CMAKE_CXX_COMPILER_LOADED)
  set(FOUND_CXX_ALIGNED_ALLOC_IMPL 0)

  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("
    #include <cstdlib>

    int main()
    {
      ::aligned_alloc(64, 128);
      return 0;
    }" HAS_CXX_ALIGNED_ALLOC)

  if (HAS_CXX_ALIGNED_ALLOC)
    set(FOUND_CXX_ALIGNED_ALLOC_IMPL 1)
    add_definitions(-DHAS_CXX_ALIGNED_ALLOC=1)
  endif()

  if (NOT FOUND_CXX_ALIGNED_ALLOC_IMPL AND UNIX)
    check_cxx_source_compiles("
    #include <cstdlib>

    int main()
    {
        void* ptr;
        ::posix_memalign(&ptr, 64, 128);
        return 0;
    }" HAS_CXX_POSIX_MEMALIGN)

    if (HAS_CXX_POSIX_MEMALIGN)
      set(FOUND_CXX_ALIGNED_ALLOC_IMPL 1)
      add_definitions(-DHAS_CXX_POSIX_MEMALIGN=1)
    endif()
  endif()


  if (NOT FOUND_CXX_ALIGNED_ALLOC_IMPL AND WIN32)
    check_cxx_source_compiles("
      #include <malloc.h>

      int main()
      {
        ::_aligned_malloc(128, 64);
        return 0;
      }" HAS_CXX_ALIGNED_MALLOC)

    if (HAS_CXX_ALIGNED_MALLOC)
      set(FOUND_CXX_ALIGNED_ALLOC_IMPL 1)
      add_definitions(-DHAS_CXX_ALIGNED_MALLOC=1)
    endif()
  endif()
endif()

if (FOUND_CXX_ALIGNED_ALLOC_IMPL OR FOUND_C_ALIGNED_ALLOC_IMPL)
  set(FOUND_ALIGNED_ALLOC_IMPL 1)
endif()

if (FOUND_ALIGNED_ALLOC_IMPL)
  include(FindPackageHandleStandardArgs)

  find_package_handle_standard_args(AlignedAlloc REQUIRED_VARS ${FOUND_ALIGNED_ALLOC_IMPL})
  mark_as_advanced(FOUND_CXX_ALIGNED_ALLOC_IMPL FOUND_C_ALIGNED_ALLOC_IMPL)
endif()