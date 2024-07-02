#update the named submodule

#attempt to add the submodule
#optional arguments:
# 1 - path
# 2 - alternate reference name
macro(add_submodule module_name)

  message(STATUS "Attempting to add \"${module_name}\" to project \"${PROJECT_NAME}\"")

  set(have_submodule FALSE)
  set(have_path FALSE)

  set(Args ${ARGN})
  list(LENGTH Args NumArgs)

  # first optional argument is module reference
  if(NumArgs GREATER 0)
    set(module_ref_name ${ARGV1})
  else()
    set(module_ref_name ${module_name})
  endif()

  # second optional argument is module path
  if(NumArgs GREATER 1)
    set(module_path ${ARGV2})
  else()
    set(module_path ${CMAKE_CURRENT_SOURCE_DIR}/${module_name})
  endif()

  set(is_submodule FALSE)
  if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    set(git_modules_file "${PROJECT_SOURCE_DIR}/.gitmodules")
    if (EXISTS ${git_modules_file})
      file(STRINGS ${git_modules_file} file_lines)
      set(have_path FALSE)

      foreach(line ${file_lines})
        if (NOT is_submodule)
          string(COMPARE EQUAL ${line} "[submodule \"${module_ref_name}\"]" is_submodule)
          if (is_submodule)
            message(STATUS "\"${module_name}\" is a submodule of project \"${PROJECT_NAME}\"")
            continue()
          endif()
        endif()

        if (is_submodule AND (NOT have_path))
          string(FIND ${line} "path =" pos)
          set(have_path NOT(pos EQUALS -1))
          if (have_path)
            string(REPLACE "path =" "" submodule_path ${line})
            string(STRIP "${submodule_path}" submodule_path)
            set(submodule_path "${PROJECT_SOURCE_DIR}/${submodule_path}")
            continue()
          endif()
        endif()
      endforeach()

      if(is_submodule)
        if(have_path)
          message(STATUS "Updating submodule \"${module_name}\" at ${submodule_path}")
          execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive "${submodule_path}"
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                  RESULT_VARIABLE GIT_SUBMOD_RESULT)
          if(GIT_SUBMOD_RESULT EQUAL "0")
            message(STATUS "Successfully updated submodule \"${module_name}\"")
          else()
            message(FATAL_ERROR "Unable to update submodule \"${module_name}\"")
          endif()
        endif()
      else()
        message(STATUS "\"${module_name}\" is not a submodule of project \"${PROJECT_NAME}\"")
      endif()

    endif()
  endif()

  if (TARGET ${module_name})
    message(STATUS "Submodule \"${module_name}\" is a target.")
  else()
    if(EXISTS "${module_path}")
      message(STATUS "Adding subdirectory ${module_path} to project \"${PROJECT_NAME}\"")
      add_subdirectory(${module_path})
    endif()
  endif()

  message("")

endmacro()
