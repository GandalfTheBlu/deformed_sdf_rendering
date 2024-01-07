#--------------------------------------------------------------------------
# setup_test
#--------------------------------------------------------------------------

PROJECT(setup_test)

SET(setup_test_files 
	main.cc
	app.h
	app.cc
)
SOURCE_GROUP("code" FILES ${setup_test_files})

ADD_EXECUTABLE(setup_test ${setup_test_files})
TARGET_LINK_LIBRARIES(setup_test engine)
ADD_DEPENDENCIES(setup_test engine)

IF(MSVC)
	SET_PROPERTY(TARGET setup_test PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
ENDIF(MSVC)