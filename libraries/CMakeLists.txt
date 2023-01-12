add_library(bela_biquad OBJECT Bela/Biquad/Biquad.cpp)
add_library(tinyalsa OBJECT tinyalsa/mixer.c tinyalsa/pcm.c)
add_library(xml OBJECT XML/pugixml.cpp)

target_include_directories(bela_biquad PUBLIC Bela/Biquad)
target_include_directories(tinyalsa PUBLIC tinyalsa/include)
target_include_directories(xml PUBLIC XML)
