
cl /nologo /LD /FeifFlif.spi spi00in.cpp -I win32 flif/maniac/bit.cpp flif/maniac/chance.cpp flif/image/crc32k.cpp flif/image/color_range.cpp flif/transform/factory.cpp flif/common.cpp flif/flif-dec.cpp flif/io.cpp spiFLIF_ex.cpp spi00in.def /WX /EHsc /DGETOPT_API= /Ox /Oy /MT /DNDEBUG
