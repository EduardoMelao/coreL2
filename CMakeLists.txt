cmake_minimum_required(VERSION 3.0.0)
project(coreL2 VERSION 0.1.0)

include(CTest)
enable_testing()

add_executable(coreL2.o
src/coreL2/CoreL2.cpp
src/common/lib5grange/lib5grange.cpp
src/coreL2/CLIL2Interface/CLIL2Interface.cpp
src/coreL2/CoreTunInterface/TunInterface.cpp
src/coreL2/Cosora/Cosora.cpp
src/coreL2/L1L2Interface/L1L2Interface.cpp
src/coreL2/LinkAdaptation/LinkAdaptation.cpp
src/coreL2/MacController/MacController.cpp
src/coreL2/Multiplexer/Multiplexer.cpp
src/coreL2/ProtocolControl/ProtocolControl.cpp
src/coreL2/ReceptionProtocol/ReceptionProtocol.cpp
src/coreL2/SduBuffers/MacAddressTable/MacAddressTable.cpp
src/coreL2/SduBuffers/SduBuffers.cpp
src/coreL2/SystemParameters/CurrentParameters.cpp
src/coreL2/SystemParameters/DynamicParameters.cpp
src/coreL2/TransmissionProtocol/TransmissionProtocol.cpp
src/coreL2/Scheduler/Scheduler.cpp)

target_link_libraries(coreL2.o
pthread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
