cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blackpill_f103c8_zyphyr)

# QPC framework directory
set(QPC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/qpc)

# option to turn QSPY software tracing ON or OFF
option(QSPY "QSPY software tracing" OFF)

# QPC framework includes...
zephyr_include_directories(
 ${QPC_DIR}/include
 ${QPC_DIR}/zephyr
)

# QPC framework sources...
target_sources(app PRIVATE
 ${QPC_DIR}/src/qf/qep_hsm.c
 ${QPC_DIR}/src/qf/qep_msm.c
 ${QPC_DIR}/src/qf/qf_act.c
 ${QPC_DIR}/src/qf/qf_qact.c
 ${QPC_DIR}/src/qf/qf_defer.c
 ${QPC_DIR}/src/qf/qf_dyn.c
 ${QPC_DIR}/src/qf/qf_mem.c
 ${QPC_DIR}/src/qf/qf_ps.c
 ${QPC_DIR}/src/qf/qf_qeq.c
 ${QPC_DIR}/src/qf/qf_qmact.c
 ${QPC_DIR}/src/qf/qf_time.c
 ${QPC_DIR}/zephyr/qf_port.c
)

# QPC configuration with QSPY...
if(QSPY)

add_compile_definitions(Q_SPY)

target_sources(app PRIVATE
 ${QPC_DIR}/src/qs/qs.c
 ${QPC_DIR}/src/qs/qs_rx.c
 ${QPC_DIR}/src/qs/qs_fp.c
 ${QPC_DIR}/src/qs/qs_64bit.c
 ${QPC_DIR}/src/qs/qstamp.c
)

endif()

# 添加头文件包含目录
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 添加源文件
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_sources(app PRIVATE src/temperature_control.c)
