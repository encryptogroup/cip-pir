add_executable(raid_pir_server server.cpp)
target_link_libraries(raid_pir_server PUBLIC RAID_PIR::raid_pir ENCRYPTO_utils::encrypto_utils)

add_executable(raid_pir_client client.cpp)
target_link_libraries(raid_pir_client PUBLIC RAID_PIR::raid_pir ENCRYPTO_utils::encrypto_utils)

add_executable (makeDatabase database.cpp)
target_link_libraries(makeDatabase PUBLIC RAID_PIR::raid_pir ENCRYPTO_utils::encrypto_utils)