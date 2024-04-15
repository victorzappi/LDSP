# Display the directory being searched.
# message(STATUS "Directory to search: ${LDSP_PROJECT}")
file(GLOB FILES_TO_SEARCH "${LDSP_PROJECT}/*")
list(LENGTH VARIABLE_NAMES num_vars)

# message(STATUS "Number of variables and search groups: ${num_vars}")

foreach(idx RANGE 0 ${num_vars} 1)
    if(idx EQUAL num_vars)
        # message(STATUS "Index ${idx} has reached the maximum valid index, stopping loop.")
        break()  # Correctly stop the loop before an out-of-range access
    endif()

    list(GET VARIABLE_NAMES ${idx} VAR_NAME)
    list(GET SEARCH_STRINGS ${idx} SEARCH_STRING_GROUP)

    # message(STATUS "Processing ${VAR_NAME} with search strings: ${SEARCH_STRING_GROUP}")
    separate_arguments(SEARCH_STRINGS_LIST UNIX_COMMAND "${SEARCH_STRING_GROUP}")

    set(STRING_FOUND FALSE)
    foreach(SEARCH_STRING IN LISTS SEARCH_STRINGS_LIST)
        foreach(FILE_PATH ${FILES_TO_SEARCH})
            file(READ "${FILE_PATH}" FILE_CONTENTS)
            string(FIND "${FILE_CONTENTS}" "${SEARCH_STRING}" POSITION)
            if(NOT POSITION EQUAL -1)
                set(STRING_FOUND TRUE)
                # message(STATUS "Found ${SEARCH_STRING} in ${FILE_PATH}")
                break()
            endif()
        endforeach()
        if(STRING_FOUND)
            break()
        endif()
    endforeach()

    if(STRING_FOUND)
        set(${VAR_NAME} TRUE CACHE BOOL "Found at least one search string for ${VAR_NAME}" FORCE)
    #     message(STATUS "Setting ${VAR_NAME} to TRUE.")
    # else()
    #     message(STATUS "Did not find any strings associated with ${VAR_NAME}.")
    endif()
endforeach()
