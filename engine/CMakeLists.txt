#--------------------------------------------------------------------------
# engine
#--------------------------------------------------------------------------

SET(engine_files 
	file_watcher.h
	file_watcher.cc
	input.h
	input.cc
	window.h
	window.cc
	render_mesh.h
	render_mesh.cc
	default_meshes.h
	default_meshes.cc
	file_io.h
	file_io.cc
	shader.h
	shader.cc
	camera.h
	camera.cc
	marching_cubes.h
	marching_cubes.cc
	transform.h
	transform.cc
	animation.h
	animation.cc
)
SOURCE_GROUP("engine" FILES ${engine_files})
ADD_LIBRARY(engine STATIC ${engine_files})
ADD_DEPENDENCIES(engine glew glfw)
TARGET_INCLUDE_DIRECTORIES(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
TARGET_LINK_LIBRARIES(engine PUBLIC exts glew glfw ${OPENGL_LIBS})

