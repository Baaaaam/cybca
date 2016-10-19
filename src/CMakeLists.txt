### DO NOT DELETE THIS COMMENT: INSERT_ARCHETYPES_HERE ###


USE_CYCLUS("cyclass" "CLASSAdaptator")
USE_CYCLUS("cyclass" "fuel_fab")
USE_CYCLUS("cyclass" "reactor")

INSTALL_CYCLUS_MODULE("cyclass" "./")

# install header files
FILE(GLOB h_files "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
INSTALL(FILES ${h_files} DESTINATION include/cyclass COMPONENT cyclass)
