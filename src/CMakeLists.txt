set(SRCS ts.c ts_parse.c ts_pes.c ts_out.c)
add_definitions("-g -o0")

add_executable(ts_tools ${SRCS})
