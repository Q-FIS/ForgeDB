add_executable(mycat mycat.c)
add_executable(mywc mywc.c)
add_executable(mycatpp mycatpp.cpp)

# Side quest: every side/<name>.c becomes a program called side_<name>.
file(GLOB SIDE_PROGRAMS CONFIGURE_DEPENDS side/*.c)
foreach(src ${SIDE_PROGRAMS})
  get_filename_component(name ${src} NAME_WE)
  add_executable(side_${name} ${src})
endforeach()
