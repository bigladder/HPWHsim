# run 'poetry install' in python folder

macro(poetry_install pythonScriptDir)

    execute_process(
            COMMAND poetry install
            WORKING_DIRECTORY "${pythonScriptDir}"
            RESULT_VARIABLE result
            ERROR_VARIABLE errors
            OUTPUT_VARIABLE outputs)

endmacro()

macro(uv_install pythonScriptDir)

    execute_process(
            COMMAND uv sync
            WORKING_DIRECTORY "${pythonScriptDir}"
            RESULT_VARIABLE result
            ERROR_VARIABLE errors
            OUTPUT_VARIABLE outputs)

endmacro()

