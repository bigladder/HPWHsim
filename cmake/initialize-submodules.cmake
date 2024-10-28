#update the named submodule

#attempt to add the submodule
#optional argument is module path
macro(add_submodule module_name)

    set(Args ${ARGN})
    list(LENGTH Args NumArgs)

    if(NumArgs GREATER 0)
        set(target_name "${ARGV1}")
    else()
        set(target_name "${module_name}")
    endif()

    set(module_path "${CMAKE_CURRENT_SOURCE_DIR}/${module_name}")
    if(NumArgs GREATER 1)
        set(module_path "${ARGV2}/vendor/${module_name}")
        set(is_submodule false)
    else()
        set(is_submodule true)
    endif()

    if(${is_submodule} AND GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
        message(STATUS "Updating submodule \"${module_name}\"")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive "${module_path}"
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "Unable to update submodule \"${module_name}\"")
        endif()
    endif()

    if (NOT TARGET ${target_name} AND (EXISTS "${module_path}"))
        message(STATUS "Adding subdirectory \"${module_path}\"")
        add_subdirectory(${module_path})
    endif()

endmacro()
