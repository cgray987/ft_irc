#include "Server.hpp"

static int stoi( std::string & s )
{
    int i;
    std::istringstream(s) >> i;
    return i;
}
