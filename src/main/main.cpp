#include "headers.hpp"
#include "constants.hpp"
#include "macros.hpp"

auto main( int argc, char **argv ) -> int {
	static_cast<void>( argc );
	static_cast<void>( argv );
	PRINT( RED << "Hello world!" << CLEAR );

	return (0);
}
