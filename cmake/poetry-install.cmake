# run 'poetry install' in python folder

macro(install_poetry)

    # execute 'poetry install'
    set(pythonScriptDir "${PROJECT_SOURCE_DIR}/scripts/python")
    if (NOT ${PROJECT_NAME}_PYTHON_SETUP)
        execute_process(
                COMMAND poetry install
                WORKING_DIRECTORY "${pythonScriptDir}"
                RESULT_VARIABLE result0
                ERROR_VARIABLE errors0
                OUTPUT_VARIABLE outputs0)

        set(${PROJECT_NAME}_PYTHON_SETUP TRUE CACHE BOOL "Python environment is already setup" FORCE)
    endif ()

endmacro()
