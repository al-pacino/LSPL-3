cmake_minimum_required( VERSION 2.5 )

project( LSPL-3 )

add_definitions( "-std=c++0x" )

include_directories( src )
include_directories( rapidjson-1.1.0/include )

set( SOURCE
	src/Attributes.cpp
	src/Configuration.cpp
	src/ErrorProcessor.cpp
	src/Parser.cpp
	src/Pattern.cpp
	src/PatternMatch.cpp
	src/PatternsFileProcessor.cpp
	src/Text.cpp
	src/TextLoader.cpp
	src/Tokenizer.cpp
	src/Tools.cpp
	src/TranspositionSupport.cpp
	src/main.cpp )

add_executable( lspl3 ${SOURCE} )
