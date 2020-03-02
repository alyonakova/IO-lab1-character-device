# convert module source/object files to module object string
function(get_module_objects module_name input_files output_string)
    foreach(module_file ${input_files})

        # only files with extension .c and .o are valid
        string(REGEX MATCH "^.+\\.[oc]$" match_file ${module_file})
        if(NOT ${match_file})
            file(RELATIVE_PATH rel_file "${PROJECT_SOURCE_DIR}" ${module_file} )
            string(REPLACE ".c" ".o" module_object ${rel_file})
            list(APPEND module_objects "${module_name}-objs += ${module_object}")
        endif()
    endforeach()
    string(REPLACE ";" "\n" module_objects_string "${module_objects}")
    set(${output_string} ${module_objects_string} PARENT_SCOPE)
endfunction()

# convert include folders of current cmake source folder to ccflags strings
function(get_directory_includes output_string)
    get_property(input_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
    foreach(include_dir ${input_dirs})
        get_filename_component(abs_path ${include_dir} REALPATH)
        list(APPEND module_includes "ccflags-y += -I${abs_path}")
    endforeach()
    string(REPLACE ";" "\n" module_includes_string "${module_includes}")
    set(${output_string} ${module_includes_string} PARENT_SCOPE)
endfunction()

# convert compile definitions of current cmake source folder to ccflags strings
function(get_directory_definitions output_string)
    get_property(input_definitions DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS)
    foreach(input_definition ${input_definitions})
        list(APPEND module_definitions "ccflags-y += -D${input_definition}")
    endforeach()
    string(REPLACE ";" "\n" module_definitions_string "${module_definitions}")
    set(${output_string} ${module_definitions_string} PARENT_SCOPE)
endfunction()

# create content of the Kbuild file
function(module_kbuild module_name module_files output_string)
    get_module_objects("${module_name}" "${module_files}" kobjects_string)
    get_directory_definitions(kdefintions_string)
    get_directory_includes(kincludes_string)
    set(${output_string}
            "obj-m += ${module_name}.o
${kobjects_string}

${kincludes_string}

${kdefintions_string}" PARENT_SCOPE)
endfunction()

# Add an kernel module to the project using the specified source files and kernel build enviroment.
function(add_module module_name module_files kernel_dir)
    # set build path
    set(module_build_path  "${PROJECT_BINARY_DIR}/${module_name}")

    # create KBuild file
    module_kbuild("${module_name}" "${module_files}" Kbuild_string)
    file(WRITE "${module_build_path}/Kbuild" ${Kbuild_string})

    # create copy script, wich copy source files to binary folder with folder structur
    foreach(copy_file ${module_files})
        file(RELATIVE_PATH rel_copy_file "${PROJECT_SOURCE_DIR}" ${copy_file} )
        list(APPEND cmd_list "cp --parents ${rel_copy_file} ${module_build_path}")
    endforeach()

    string(REPLACE ";" "\n" cmd_list_string "${cmd_list}")
    file(WRITE "${module_build_path}/cp.sh" "#!/bin/sh\n${cmd_list_string}")
    file(COPY "${module_build_path}/cp.sh"
            DESTINATION "${module_build_path}/.tmp"
            FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    file(REMOVE "${module_build_path}/cp.sh")

    # define make modules command
    ADD_CUSTOM_COMMAND(
            OUTPUT  ${PROJECT_BINARY_DIR}/${module_name}.built
            COMMAND ${CMAKE_MAKE_PROGRAM} -C ${kernel_dir} M=${module_build_path} modules KBUILD_EXTRA_SYMBOLS=${module_build_path}/Module.symvers
            COMMAND cmake -E touch ${PROJECT_BINARY_DIR}/${module_name}.built
            COMMENT "Kernel make modules ${module_name}:
${CMAKE_MAKE_PROGRAM} -C ${kernel_dir} M=${module_build_path} modules KBUILD_EXTRA_SYMBOLS=${module_build_path}/Module.symvers

Kbuild:
${Kbuild_string}\n"
            DEPENDS ${PROJECT_SOURCE_DIR}/CMakeLists.txt ${module_files} ${module_include_dirs}
            VERBATIM)

    # define copy files command
    ADD_CUSTOM_COMMAND(
            OUTPUT  ${PROJECT_BINARY_DIR}/${module_name}-copy.built
            COMMAND "${module_build_path}/.tmp/cp.sh"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            COMMAND cmake -E touch ${PROJECT_BINARY_DIR}/${module_name}-copy.built
            DEPENDS ${PROJECT_SOURCE_DIR}/CMakeLists.txt ${module_files} ${module_include_dirs}
            VERBATIM)

    # define command target
    ADD_CUSTOM_TARGET("${module_name}" ALL
            DEPENDS ${PROJECT_BINARY_DIR}/${module_name}.built ${PROJECT_BINARY_DIR}/${module_name}-copy.built
            SOURCES ${module_files}
            COMMENT "Building Kernel Module ${module_name}")
endfunction()
