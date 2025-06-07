#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>

#ifndef NDEBUG
	#define DEBUG_LOG(x) std::cerr << x << "\n"
#else
	#define DEBUG_LOG(x) do {} while(0)
#endif

#endif // DEBUG_H