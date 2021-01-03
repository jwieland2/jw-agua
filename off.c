#include <stdio.h>
#include <wiringPi.h>

// turn off irrigation
int main()
{
	if (wiringPiSetup() == -1)
		return 1;
	digitalWrite (3, 1);	// water off
	digitalWrite (4,1);		// well off
}
