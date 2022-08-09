if(USE_CUDA)
    add_library(CIP_PIR
            database/createDatabase.cpp
            database/manageDatabase.cpp
            CIP_PIR/PeerServer/CIP_pirServer.cpp CIP_PIR/CenterServer/RAID_pirCenterServer.cpp CIP_PIR/Client/CIP_pirClient.cpp CIP_PIR/Client/CIP_pirClient.h CIP_PIR/PeerServer/ClientConnection.cpp CIP_PIR/PeerServer/ClientConnection.h CIP_PIR/Computation/helper.cpp CIP_PIR/Computation/helper.h CIP_PIR/CenterServer/RAID_pirCenterServer.h CIP_PIR/CenterServer/RAID_pirCenterWorker.cpp CIP_PIR/CenterServer/RAID_pirCenterWorker.h CIP_PIR/CenterServer/CentralConnectionHandler.cpp CIP_PIR/CenterServer/CentralConnectionHandler.h
            CIP_PIR/Computation/Accelerator_CUDA.cu CIP_PIR/Computation/Accelerator_CUDA.h)
else()
    add_library(CIP_PIR
            database/createDatabase.cpp
            database/manageDatabase.cpp
            CIP_PIR/PeerServer/CIP_pirServer.cpp CIP_PIR/CenterServer/RAID_pirCenterServer.cpp CIP_PIR/Client/CIP_pirClient.cpp CIP_PIR/Client/CIP_pirClient.h CIP_PIR/PeerServer/ClientConnection.cpp CIP_PIR/PeerServer/ClientConnection.h CIP_PIR/Computation/helper.cpp CIP_PIR/Computation/helper.h CIP_PIR/CenterServer/RAID_pirCenterServer.h CIP_PIR/CenterServer/RAID_pirCenterWorker.cpp CIP_PIR/CenterServer/RAID_pirCenterWorker.h CIP_PIR/CenterServer/CentralConnectionHandler.cpp CIP_PIR/CenterServer/CentralConnectionHandler.h
            )
endif()
add_library(RAID_PIR::raid_pir ALIAS CIP_PIR)

if(ENCRYPTO_utils_FOUND)
    target_link_libraries(CIP_PIR
            PUBLIC ENCRYPTO_utils::encrypto_utils OpenMP::OpenMP_CXX)
else()
    target_link_libraries(CIP_PIR
            PUBLIC ENCRYPTO_utils::encrypto_utils)
endif()

install(TARGETS CIP_PIR
        EXPORT "${PROJECT_NAME}Targets"
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        INCLUDES DESTINATION lib
        )

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        DESTINATION include
        FILES_MATCHING PATTERN "*.h"
        )

include(CMakePackageConfigHelpers)

configure_package_config_file("${PROJECT_SOURCE_DIR}/cmake/RAID_PIRConfig.cmake.in"
        ${CMAKE_CURRENT_BINARY_DIR}/RAID_PIRConfig.cmake
        INSTALL_DESTINATION "lib/cmake/RAID_PIR"
        )

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/RAID_PIRConfig.cmake"
        DESTINATION "lib/cmake/${PROJECT_NAME}"
        )

add_subdirectory(application)