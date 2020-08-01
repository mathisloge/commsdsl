set (CC_FETCH_DEFAULT_REPO "https://github.com/arobenko/comms_champion.git")
set (CC_FETCH_DEFAULT_TAG "master")

function (cc_fetch)
    set (_prefix CC_FETCH)
    set (_options)
    set (_oneValueArgs SRC_DIR REPO TAG)
    set (_mutiValueArgs)
    cmake_parse_arguments(${_prefix} "${_options}" "${_oneValueArgs}" "${_mutiValueArgs}" ${ARGN})

    if (NOT CC_FETCH_SRC_DIR)
        message (FATAL_ERROR "The SRC_DIR parameter is not provided")
    endif ()     

    if (NOT CC_FETCH_REPO)
        set (CC_FETCH_REPO ${CC_FETCH_DEFAULT_REPO})
    endif ()  

    if (NOT CC_FETCH_TAG)
        set (CC_FETCH_TAG ${CC_FETCH_DEFAULT_TAG})
    endif ()

    if (NOT GIT_FOUND)
        find_package(Git REQUIRED)
    endif ()

    if (EXISTS "${CC_FETCH_SRC_DIR}/.git")
        return ()
    endif()    

    execute_process (
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CC_FETCH_SRC_DIR}"
    )
    
    execute_process (
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CC_FETCH_SRC_DIR}"
    )        
    
    execute_process (
        COMMAND 
            ${GIT_EXECUTABLE} clone -b ${CC_FETCH_TAG} --depth 1 ${CC_FETCH_REPO} ${CC_FETCH_SRC_DIR}
        RESULT_VARIABLE git_result
    )

    if (NOT "${git_result}" STREQUAL "0")
        message (WARNING "git clone/checkout failed")
    endif ()    

endfunction()