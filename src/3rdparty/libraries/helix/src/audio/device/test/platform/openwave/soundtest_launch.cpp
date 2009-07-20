#include <soundtest_launch.h>
#include "soundtest.h"

const char *
soundtest_launch_getname (void)
{
	return ("OaSoundTest");
}

OpApplication *
soundtest_launch_create (void)
{
	return (OaSoundTest::Create ());
}
