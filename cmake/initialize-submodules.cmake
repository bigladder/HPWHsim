#update the named submodule

#attempt to add the submodule
#optional argument is module path
macro(add_submodule module_name)

  set(Args ${ARGN})
  list(LENGTH Args NumArgs)

  set(target_name ${module_name})
  if(NumArgs GREATER 0)
    set(module_path ${ARGV1})
  else()
    set(module_path ${module_name})
  endif()
  set(full_module_path ${module_path})

  if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    set(git_modules_file "${PROJECT_SOURCE_DIR}/.gitmodules")
    if (EXISTS ${git_modules_file})
      file(STRINGS ${git_modules_file} file_lines)

      foreach(line ${file_lines})

        if (${line} MATCHES "url =")
          string(REGEX REPLACE "\\s*url = .*/(.*).git" "\\1" submodule "${line}")
          string(STRIP "${submodule}" submodule)

          string(COMPARE EQUAL ${submodule} ${module_path} is_submodule)
          if(${is_submodule})
            message(STATUS "Updating submodule \"${submodule}\"")
            execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive "${submodule}"
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_SUBMOD_RESULT)
            if(GIT_SUBMOD_RESULT EQUAL "0")
              set(full_module_path ${CMAKE_CURRENT_SOURCE_DIR}/${module_path})
            else()
              message(FATAL_ERROR "Unable to update submodule \"${submodule}\"")
            endif()
            break()
          endif()
        endif()

      endforeach()

    endif()
  endif()

  if (NOT TARGET ${target_name} AND (EXISTS "${full_module_path}"))
    message(STATUS "Adding subdirectory \"${module_path}\" to project \"${PROJECT_NAME}\"")
    add_subdirectory(${module_path})
  endif()

endmacro()
