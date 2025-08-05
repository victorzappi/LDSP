# Gather all project files under LDSP_PROJECT
file(GLOB FILES_TO_SEARCH "${LDSP_PROJECT}/*")

# How many variables?
list(LENGTH VARIABLE_NAMES num_vars)
math(EXPR last_idx "${num_vars} - 1")

# Iterate each VARIABLE_NAMES entry
foreach(idx RANGE 0 ${last_idx})
    list(GET VARIABLE_NAMES ${idx} VAR_NAME)
    list(GET SEARCH_STRINGS ${idx} SEARCH_GROUP)

    # message(STATUS "[DEBUG] Variable #${idx}: '${VAR_NAME}' → search group: ${SEARCH_GROUP}")

    set(STRING_FOUND FALSE)
    set(FOUND_STRING "")

    # Scan through each file
    foreach(FILE_PATH IN LISTS FILES_TO_SEARCH)
        file(READ "${FILE_PATH}" FILE_CONTENTS)

        # Try each pattern in this group
        foreach(SEARCH_STRING IN LISTS SEARCH_GROUP)
            string(FIND "${FILE_CONTENTS}" "${SEARCH_STRING}" POSITION)
            if(NOT POSITION EQUAL -1)
                set(STRING_FOUND TRUE)
                set(FOUND_STRING "${SEARCH_STRING}")
                # message(STATUS "[DEBUG] Found '${SEARCH_STRING}' in '${FILE_PATH}'")
                break()
            endif()
        endforeach()

        if(STRING_FOUND)
            break()
        endif()
    endforeach()

    # Set the cache var and report final result
    if(STRING_FOUND)
        set(${VAR_NAME} TRUE CACHE BOOL "Found at least one search string for ${VAR_NAME}" FORCE)
        # message(STATUS "[DEBUG] -> ${VAR_NAME}=TRUE (matched '${FOUND_STRING}')")
    # else()
        # message(STATUS "[DEBUG] -> No matches found for ${VAR_NAME}")
    endif()
endforeach()

# message(STATUS "[DEBUG] All variables processed.")
