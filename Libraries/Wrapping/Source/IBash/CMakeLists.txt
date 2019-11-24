find_package(BISON)
find_package(Patch)

if (BISON_FOUND AND Patch_FOUND)
	option(USE_JOBS_CONTROL "Enable using bash's jobs controler to manage concurency jobs" ON)

	include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
 			    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Base/Libraries/Data
			    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Base/Libraries/Base
			    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Base/Libraries/Base/Type
			    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Base/Libraries/Wrapping
			    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Base/Libraries/Wrapping/Implement
			    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Base/Libraries/Unittest
			    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Base/Libraries/Unittest/Implement
			    ${CMAKE_CURRENT_BINARY_DIR}/Bash
			    ${CMAKE_CURRENT_BINARY_DIR}/Bash/include
			    ${CMAKE_CURRENT_BINARY_DIR}/Bash/builtins
			    ${CMAKE_CURRENT_BINARY_DIR}/Bash/lib
			    ${CMAKE_CURRENT_BINARY_DIR}/Bash/lib/intl
			    ${CMAKE_CURRENT_BINARY_DIR}/Bash/lib/termcap
			    ${CMAKE_BINARY_DIR}/Bash/lib/readline)

	add_c_compiler_flag(-Wno-parentheses)
	add_c_compiler_flag(-Wno-format-security)

	set(ROOT "${CMAKE_CURRENT_BINARY_DIR}/Bash")
	set(VERSION "5.0")

	include(AddCXXCompilerFlag)
	include(AddCCompilerFlag)

	message(STATUS "Cloning https://git.savannah.gnu.org/git/bash.git, it may take time")
	execute_process(COMMAND /usr/bin/git clone https://git.savannah.gnu.org/git/bash.git ${ROOT}
			TIMEOUT 500
			RESULT_VARIABLE ECODE
			ERROR_VARIABLE ERROR_FOUND
			OUTPUT_QUIET)

	if (ECODE EQUAL "0")
		add_definitions(-DHAVE_CONFIG_H=1)
		add_definitions(-DSHELL=1)

		file(GLOB BASH_SOURCES "${ROOT}/*.c")
		file(GLOB BASH_HEADERS "${ROOT}/*.h" "${ROOT}/lib/malloc/*.h")
		file(GLOB SH_SOURCES "${ROOT}/lib/sh/*.c")
		file(GLOB SH_HEADERS "${ROOT}/lib/sh/*.h")
		file(GLOB TERM_SOURCES "${ROOT}/lib/termcap/*.c")
		file(GLOB TERM_HEADERS "${ROOT}/lib/termcap/*.c")
		file(GLOB ALLOC_SOURCES "${ROOT}/lib/malloc/*.c")
		file(GLOB ALLOC_HEADERS "${ROOT}/lib/malloc/*.c")
		file(GLOB GLOB_SOURCES "${ROOT}/lib/glob/*.c")
		file(GLOB GLOB_HEADERS "${ROOT}/lib/glob/*.c")
		file(GLOB RL_SOURCES "${ROOT}/lib/readline/*.c")
		file(GLOB RL_HEADERS "${ROOT}/lib/readline/*.c")
		file(GLOB COM_SOURCES "${ROOT}/builtins/*.c")
		file(GLOB COM_HEADERS "${ROOT}/builtins/*.h")
		file(GLOB DEFs "${ROOT}/builtins/*.def")

		list(REMOVE_ITEM BASH_SOURCES "${ROOT}/y.tab.c" "${ROOT}/mksyntax.c" "${ROOT}/variable.c")
		list(REMOVE_ITEM SH_SOURCES "${ROOT}/lib/sh/strdup.c")
		list(REMOVE_ITEM RL_SOURCES "${ROOT}/lib/readline/emacs_keymap.c" 
					    "${ROOT}/lib/readline/vi_keymap.c")
		list(REMOVE_ITEM COM_SOURCES "${ROOT}/builtins/gen-helpfiles.c")
		list(REMOVE_ITEM GLOB_SOURCES "${ROOT}/lib/glob/glob_loop.c"
					      "${ROOT}/lib/glob/sm_loop.c"
					      "${ROOT}/lib/glob/gm_loop.c")
		list(REMOVE_ITEM DEFs "${ROOT}/builtins/reserved.def" "${ROOT}/builtins/inlib.def")

		if (USE_JOBS_CONTROL)
			list(REMOVE_ITEM BASH_SOURCES "${ROOT}/nojobs.c")
			add_definitions(-DJOB_CONTROL=1)
		else()		
			list(REMOVE_ITEM BASH_SOURCES "${ROOT}/jobs.c")
			add_definitions(-DJOB_CONTROL_MISSING=1)
		endif()

		add_library(sh STATIC ${SH_SOURCES})
		add_library(malloc STATIC ${ALLOC_SOURCES})
		add_library(termcap STATIC ${TERM_SOURCES})
		add_library(glob STATIC ${GLOB_SOURCES})
		add_library(readline STATIC ${RL_SOURCES})

		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/pathnames.h
			  	COMMAND cp ${ROOT}/pathnames.h.in ${CMAKE_CURRENT_BINARY_DIR}/pathnames.h
				COMMENT "Generate pathnames.h")
		add_custom_target(pathnames DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/pathnames.h)

		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/builtins.c
				COMMAND mkbuiltins -D ${CMAKE_CURRENT_BINARY_DIR} -externfile ${ROOT}/builtins/builtext.h -structfile builtins.c ${DEFs}
				DEPENDS mkbuiltins
				COMMENT "Collect builtin functions")
		add_custom_target(builtext DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/builtins.c)

		add_custom_command(
			OUTPUT ${ROOT}/signames.h
			COMMAND mksignames ${ROOT}/signames.h
			DEPENDS mksignames
			COMMENT "Generate signams.h")
		add_custom_target(signames DEPENDS ${ROOT}/signames.h)

		add_custom_command(
			OUTPUT ${ROOT}/version.h
			COMMAND /bin/sh ${ROOT}/support/mkversion.sh -b -S . -s release -d ${VERSION} -o ${ROOT}/version.h
			COMMENT "Generate version.h")
		add_custom_target(version DEPENDS ${ROOT}/version.h)

		add_custom_command(
			OUTPUT syntax.c
			COMMAND mksyntax -o ${CMAKE_CURRENT_BINARY_DIR}/syntax.c
			DEPENDS mksyntax
			COMMENT "Generate syntax.c")
		add_custom_target(syntax DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/syntax.c)

		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/parse.y.c
			COMMAND ${BISON_EXECUTABLE} -y ${ROOT}/parse.y -o ${CMAKE_CURRENT_BINARY_DIR}/parse.y.c
			COMMENT "Generate parse.y.c")
		add_custom_target(yparse DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/parse.y.c)

		add_custom_command(
			OUTPUT ${ROOT}/builtins/pipesize.h
			COMMAND /bin/sh ${ROOT}/builtins/psize.sh > ${ROOT}/builtins/pipesize.h
			COMMENT "Generate pipesize.h")
		add_custom_target(pipesize DEPENDS ${ROOT}/builtins/pipesize.h)

		foreach(DEF ${DEFs})
			string(REPLACE "def" "c" FILE ${DEF})
			get_filename_component(BASENAME ${FILE} NAME)
			get_filename_component(BUILTIN ${FILE} NAME_WE)

			add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${BASENAME}
					   COMMAND ${CMAKE_CURRENT_BINARY_DIR}/mkbuiltins -D . ${DEF} ${CMAKE_CURRENT_BINARY_DIR}/${BASENAME}
					   DEPENDS mkbuiltins pipesize)

			add_custom_target(${BUILTIN}.def DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${BASENAME}")
			add_dependencies(builtext ${BUILTIN}.def)

			list(APPEND BUILTIN_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/${BASENAME}")
		endforeach()

		set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/builtins.c PROPERTIES GENERATED TRUE)

		add_executable(mkbuiltins ${ROOT}/builtins/mkbuiltins.c)
		add_executable(mksyntax ${ROOT}/mksyntax.c)
		add_executable(mksignames ${ROOT}/support/mksignames.c
					  ${ROOT}/support/signames.c)
	
		add_library(builtins ${CMAKE_CURRENT_BINARY_DIR}/builtins.c
				     ${BUILTIN_SOURCES} ${COM_SOURCES})
		add_executable(bash ${BASH_HEADERS} ${BASH_SOURCES}
			            ${CMAKE_CURRENT_BINARY_DIR}/syntax.c
	   		            ${CMAKE_CURRENT_BINARY_DIR}/parse.y.c)

		target_compile_definitions(bash PRIVATE -DCONF_HOSTTYPE="${CMAKE_HOST_SYSTEM_PROCESSOR}"
							-DCONF_MACHTYPE="linux"
							-DCONF_VENDOR="${CMAKE_HOST_SYSTEM_NAME}"
							-DCONF_OSTYPE="${CMAKE_HOST_SYSTEM_NAME}"
							-DLOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}"
							-DPACKAGE="${CMAKE_PROJECT_NAME}")

		target_link_libraries(bash base sh glob readline builtins malloc termcap ${CMAKE_DL_LIBS})
		target_link_libraries(builtins sh readline)

		add_dependencies(bash builtext signames version syntax yparse)
		add_dependencies(builtins builtext version)
		add_dependencies(glob pathnames)
		add_dependencies(sh pathnames)
	else()
		message(WARNING "Cloning fail, here is the log ${ERROR_VARIABLE}")
	endif()
endif()
