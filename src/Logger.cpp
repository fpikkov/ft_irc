#include "Logger.hpp"
#include "constants.hpp"

size_t	Logger::_functionLength = 8;

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

/**
 * @brief Logs an event to the console with a timestamp.
 *
 * @param func Name of the event which occurred.
 * @param status The status of the event: SUCCESS, FAIL, DEBUG or INFO.
 * @param msg The message accompanied by the event.
 */
void	Logger::log( const std::string& func, const std::string& status, const std::string& msg )
{
	std::string	time		= timestamp();
	std::string eventName	= func;

	eventName.resize(_functionLength, ' ');

	std::cout << time << " [" << status << "]" << " [" << func << "] " << msg << std::endl;
}
