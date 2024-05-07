# Display the directory being searched
file(GLOB FILES_TO_SEARCH "${LDSP_PROJECT}/*")
list(LENGTH VARIABLE_NAMES num_vars)
set(strings_per_variable 2)  # Adjust this value as needed

# Calculate the final index for the outer loop
math(EXPR last_idx "${num_vars} - 1")

# message(STATUS "Number of variables and search groups: ${num_vars}")

# Iterate through each variable index
foreach(idx RANGE 0 ${last_idx})
    list(GET VARIABLE_NAMES ${idx} VAR_NAME)

    # Calculate the starting index for this variable's search strings
    math(EXPR start_idx "${idx} * ${strings_per_variable}")
    set(SEARCH_STRINGS_GROUP "")

    # Gather search strings associated with the current variable
    math(EXPR end_idx "${strings_per_variable} - 1")
    foreach(str_idx RANGE 0 ${end_idx})
        math(EXPR current_idx "${start_idx} + ${str_idx}")
        list(GET SEARCH_STRINGS ${current_idx} SEARCH_STRING)
        list(APPEND SEARCH_STRINGS_GROUP "${SEARCH_STRING}")
    endforeach()

    # message(STATUS "Processing ${VAR_NAME} with search strings: ${SEARCH_STRINGS_GROUP}")

    set(STRING_FOUND FALSE)
    foreach(FILE_PATH ${FILES_TO_SEARCH})
        file(READ "${FILE_PATH}" FILE_CONTENTS)

        # Check if any of the search strings are found in the file
        foreach(SEARCH_STRING IN LISTS SEARCH_STRINGS_GROUP)
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
    endif()
endforeach()

# message(STATUS "Index ${num_vars} has reached the maximum valid index, stopping loop.")
