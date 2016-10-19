### DO NOT DELETE THIS COMMENT: INSERT_ARCHETYPES_HERE ###


USE_CYCLUS("cybca" "CLASSAdaptator")
USE_CYCLUS("cybca" "fuel_fab")
USE_CYCLUS("cybca" "reactor")

INSTALL_CYCLUS_MODULE("cybca" "./")

# install header files
FILE(GLOB h_files "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
INSTALL(FILES ${h_files} DESTINATION include/cybca COMPONENT cybca)
