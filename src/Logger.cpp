#include "Logger.hpp"
#include "constants.hpp"

Logger::Logger() {}

Logger&	Logger::instance()
{
	static Logger	logger;
	return ( logger );
}

std::string	Logger::timestamp()
{
	using sysclock = std::chrono::system_clock;
	const std::time_t cTime = sysclock::to_time_t( sysclock::now() );

	std::ostringstream	oss;
	oss << std::put_time(std::localtime(&cTime), "%F %T");

	return (oss.str());
}

// TODO: status can be an integer or bool so we could colorize the logging outputs
void	Logger::log( const std::string& func, const std::string& status, const std::string& msg )
{
	std::string	time = timestamp();

	std::cout << time << " [" << status << "]" << " " << func << " " << msg << std::endl;
}
