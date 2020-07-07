PROJECT( StainEvaluation )
CMAKE_MINIMUM_REQUIRED( VERSION 3.13 )
# Enable C++17 features
SET(CMAKE_CXX_STANDARD 17)
INCLUDE(FetchContent)

#Note which version of Sedeen Viewer this plugin was last compiled and tested with
SET(SEDEENSDK_VERSION "5.5.0.20200610" CACHE STRING "Last version of Sedeen Viewer the plugin was compiled and tested with")

# Define project description variables
SET( DISPLAY_NAME_TEXT "Stain Evaluation" CACHE STRING "Name of the plugin as it should be displayed in Sedeen Viewer")
SET( SUPPORT_URL_TEXT "http://pathcore.com/support/plugin/info/${PROJECT_NAME}" CACHE STRING "Location users can find help with the plugin" )
SET( DEVELOPER_TEXT "Sunnybrook Research Institute" CACHE STRING "Name of the author or organization that created the plugin" )

# Load the Sedeen dependencies
SET(PROGRAMFILESX86 "PROGRAMFILES\(X86\)")
FIND_PACKAGE( SEDEENSDK REQUIRED 
  HINTS ../../.. 
        "$ENV{${PROGRAMFILESX86}}/Sedeen Viewer SDK/v5.5.0.20200610/msvc2017"
        "$ENV{PROGRAMFILES}/Sedeen Viewer SDK/v5.5.0.20200610/msvc2017" )

# Load the included OpenCV libs
FIND_PACKAGE(SEDEENSDK_OPENCV REQUIRED
  HINTS ../../..
        "$ENV{${PROGRAMFILESX86}}/Sedeen Viewer SDK/v5.5.0.20200610/msvc2017"
        "$ENV{PROGRAMFILES}/Sedeen Viewer SDK/v5.5.0.20200610/msvc2017" )

# Fetch TinyXML2 files. Do not build as a subproject
FetchContent_Declare(
  TinyXML2
  GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
  GIT_TAG 61a4c7d507322c9f494f5880d4c94b60e4ce9590
)
# Check if the tinyxml2 files have already been populated
FetchContent_GetProperties(TinyXML2)
STRING(TOLOWER "TinyXML2" TinyXML2Name)
IF(NOT ${TinyXML2Name}_POPULATED)
  #Fetch TinyXML2 using the details from FetchContent_Declare 	
  FetchContent_Populate(TinyXML2)
ENDIF()

#Fetch the OpticalDensityThreshold respository. Do not build as a subproject
FetchContent_Declare(
  OpticalDensityThreshold
  GIT_REPOSITORY https://github.com/sedeen-piip-plugins/OpticalDensityThreshold.git
  GIT_TAG 4c13adceeccbc51384d198541759be3b48280ef3
)
FetchContent_GetProperties(OpticalDensityThreshold)
STRING(TOLOWER "OpticalDensityThreshold" OpticalDensityThresholdName)
IF(NOT ${OpticalDensityThresholdName}_POPULATED)
  FetchContent_Populate(OpticalDensityThreshold)
ENDIF()
SET(OPTICAL_DENSITY_THRESHOLD_DIR ${${OpticalDensityThresholdName}_SOURCE_DIR})

# Fetch the StainAnalysis-plugin repository. Do not build as a subproject
FetchContent_Declare(
  StainAnalysisplugin
  GIT_REPOSITORY https://github.com/sedeen-piip-plugins/StainAnalysis-plugin.git
  GIT_TAG 4be9e69d71869026d824d7959ba30fa20e88c12e
)
FetchContent_GetProperties(StainAnalysisplugin)
STRING(TOLOWER "StainAnalysisplugin" StainAnalysispluginName)
IF(NOT ${StainAnalysispluginName}_POPULATED)
  FetchContent_Populate(StainAnalysisplugin)
ENDIF()
SET(STAIN_ANALYSIS_DIR ${${StainAnalysispluginName}_SOURCE_DIR})

# Fetch the CreateStainVectorProfile repository. Do not build as a subproject
FetchContent_Declare(
  CreateStainVectorProfile
  GIT_REPOSITORY https://github.com/sedeen-piip-plugins/CreateStainVectorProfile.git
  GIT_TAG 58e2442e67077de7cd60a6e63cd4fc094c6a1d56
)
FetchContent_GetProperties(CreateStainVectorProfile)
STRING(TOLOWER "CreateStainVectorProfile" CreateStainVectorProfileName)
IF(NOT ${CreateStainVectorProfile}_POPULATED)
  FetchContent_Populate(CreateStainVectorProfile)
ENDIF()
SET(CREATE_STAIN_VECTOR_PROFILE_DIR ${${CreateStainVectorProfileName}_SOURCE_DIR})

INCLUDE_DIRECTORIES( ${INCLUDE_DIRECTORIES} 
                     ${SEDEENSDK_INCLUDE_DIR} 
                     ${SEDEENSDK_OPENCV_INCLUDE_DIR}
                     ${${TinyXML2Name}_SOURCE_DIR}
                     ${OPTICAL_DENSITY_THRESHOLD_DIR}
                     ${STAIN_ANALYSIS_DIR}
                     ${CREATE_STAIN_VECTOR_PROFILE_DIR}  
                     ) 

LINK_DIRECTORIES( ${LINK_DIRECTORIES} 
                  ${SEDEENSDK_LIBRARY_DIR} 
                  ${SEDEENSDK_OPENCV_LIBRARY_DIR} 
                  ) 

# Build the code into a module library
ADD_LIBRARY( ${PROJECT_NAME} MODULE 
             ${PROJECT_NAME}.cpp 
             ${PROJECT_NAME}.h 
             ${${TinyXML2Name}_SOURCE_DIR}/tinyxml2.h 
             ${${TinyXML2Name}_SOURCE_DIR}/tinyxml2.cpp
             )

# Link the library against the Sedeen SDK libraries
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} 
                       ${SEDEENSDK_LIBRARIES} 
                       ${SEDEENSDK_OPENCV_LIBRARIES}
                       )

#Create or update the .info file in the build directory
STRING( TIMESTAMP DATE_CREATED_TEXT "%Y-%m-%d" )
CONFIGURE_FILE( "infoTemplate.info.in" "${PROJECT_NAME}.info" )

# Set the install destination directory
IF( NOT PLUGIN_DESTINATION_DIR )
  IF( ${SEDEEN_FOUND} )
	SET( TEMPPLUGINDIR "${PATHCORE_DIR}/plugins/cpp/piip/${PROJECT_NAME}" )
  ELSE()
	SET( TEMPPLUGINDIR "PLUGIN_DESTINATION_DIR-NOTFOUND" )
	MESSAGE( SEND_ERROR "PLUGIN_DESTINATION_DIR not found. Set this to the target installation directory of the plugin within Sedeen Viewer (e.g. $ENV{PROGRAMFILES}/Sedeen Viewer/plugins/cpp/piip/${PROJECT_NAME}).")
  ENDIF()
  SET(PLUGIN_DESTINATION_DIR ${TEMPPLUGINDIR} CACHE PATH "Installation directory for the plugin within Sedeen Viewer")
ENDIF()

# Install the plugin and .info file in the PLUGIN_DESTINATION_DIR directory
IF( ${SEDEEN_FOUND} )
  INSTALL(TARGETS ${PROJECT_NAME} LIBRARY
    DESTINATION "${PLUGIN_DESTINATION_DIR}")
  INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.info"
    DESTINATION "${PLUGIN_DESTINATION_DIR}")
  INSTALL(DIRECTORY "${SEDEENSDK_DIR}/bin/" 
     DESTINATION "${PLUGIN_DESTINATION_DIR}" 
     FILES_MATCHING PATTERN "opencv*0.dll" )
ENDIF()
