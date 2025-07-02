#include "Logger.hpp"
#include "macros.hpp"

Logger::Logger() {}

auto Logger::instance() -> Logger&
{
	static Logger	logger;
	return ( logger );
}

auto Logger::timestamp() -> std::string
{
	using sysclock = std::chrono::system_clock;
	const std::time_t cTime = sysclock::to_time_t( sysclock::now() );

	std::ostringstream	oss;
	oss << std::put_time(std::localtime(&cTime), "%F %T");

	return (oss.str());
}

// TODO: status can be an integer or bool so we could colorize the logging outputs
auto Logger::log( const std::string& func, const std::string& status, const std::string& msg ) -> void
{
	std::string	time = timestamp();

	PRINT( time << " [" << status << "]" << " " << func << " " << msg );
}
