# Gather all pd patches under LDSP_PROJECT
file(GLOB_RECURSE FILES_TO_SEARCH "${LDSP_PROJECT}/*.pd")

set(PD_SCOPE_FOUND FALSE CACHE BOOL "No scope dac~ found" FORCE)

set(SCOPE_FOUND FALSE)

# Scan through each file
foreach(FILE_PATH IN LISTS FILES_TO_SEARCH)
    file(READ "${FILE_PATH}" FILE_CONTENTS)
    # message(STATUS "[DEBUG] Searching scope dac~ in '${FILE_PATH}'")
    

    # Search for dac~ followed by 27, 28, 29, or 30
    if(FILE_CONTENTS MATCHES "dac~[^;]*[ \t]+(27|28|29|30)([^0-9]|$)")
        set(PD_SCOPE_FOUND TRUE CACHE BOOL "Found at least one scope dac~" FORCE)
        # message(STATUS "[DEBUG] Found scope dac~ in '${FILE_PATH}'")
        break()
    endif()
endforeach()

# if(NOT PD_SCOPE_FOUND)
#     message(STATUS "[DEBUG] Scope dac~ not found in any pd patch")
# endif()

