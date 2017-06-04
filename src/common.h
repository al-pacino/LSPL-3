#pragma once

#include <map>
#include <set>
#include <list>
#include <array>
#include <regex>
#include <stack>
#include <tuple>
#include <bitset>
#include <locale>
#include <memory>
#include <string>
#include <vector>
#ifdef _WIN32
#include <codecvt>
#endif
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <exception>
#include <forward_list>
#include <unordered_map>
#include <unordered_set>

using namespace std;

// internal check of program logic like an assert
#define check_logic( condition ) \
	do { \
		if( !( condition ) ) { \
			throw logic_error( string( "Internal program error: " ) \
				+ __FILE__ + "," + to_string( __LINE__ ) ); \
		} \
	} while( false )

#ifdef _DEBUG
#define debug_check_logic check_logic
#else
#define debug_check_logic
#endif
