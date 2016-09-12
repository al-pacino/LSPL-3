#pragma once

#include <map>
#include <list>
#include <stack>
#include <tuple>
#include <bitset>
#include <locale>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <exception>
#include <algorithm>
#include <unordered_map>

using namespace std;

// internal check of program logic like an assert
#define check_logic( condition ) \
	do { \
		if( !( condition ) ) { \
			throw logic_error( string( "Internal program error: " ) \
				+ __FILE__ + "," + to_string( __LINE__ ) ); \
		} \
	} while( false )
