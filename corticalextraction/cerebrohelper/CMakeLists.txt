cmake_minimum_required(VERSION 2.8.11)
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW) # CMake 3.0
endif()
if(POLICY CMP0043)
  cmake_policy(SET CMP0043 NEW) # CMake 3.0
endif()
if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW) # CMake 3.1
endif()

project(CerebroHelper)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
## Instruct CMake to run moc automatically when needed.
#set(CMAKE_AUTOMOC ON)

# Qt and project source code
file(GLOB Project_source_files
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/*.cf
	${CMAKE_CURRENT_SOURCE_DIR}/*.c
)
file(GLOB Project_header_files
	${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Add the include directories for VTK and Qt 5 Widgets module to the compile lines.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
)

# nifti2dicom_core target
#add_library(nifti2dicom_core STATIC ${Nifti_core_source_files} ${Nifti_core_header_files})
#target_link_libraries(nifti2dicom_core LINK_PRIVATE ${ITK_LIBRARIES})

add_executable(${PROJECT_NAME}
               ${Project_source_files}
			   ${Project_header_files}
)

target_link_libraries(${PROJECT_NAME})
