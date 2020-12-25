#include <stdio.h>
#include <wiringPi.h>
#include <time.h>

// initialize on pi boot
int main()
{
	if (wiringPiSetup() == -1)
		return 1;
		
	pinMode (0, INPUT);		// Ultrasonic ECHO
	pinMode (1, OUTPUT);	// Ultrasonic TRIG
	pinMode (3, OUTPUT);	// Relay water
	pinMode (4, OUTPUT);	// Relay well
	
	digitalWrite (1, 0);	// Ultrasonic off
	digitalWrite (3, 1);	// water off
	digitalWrite (4, 1);	// well off	
	
	// create timestamp
	time_t now;
	time(&now);	
	struct tm* now_tm;
	now_tm = localtime(&now);	
	char timestamp[17];
	strftime (timestamp, 17, "%Y-%m-%d %H:%M", now_tm);
	char filePath[42];
	strftime(filePath, 42, "/home/pi/zisterne3/log/%Y-%m-%d_log.txt",
		now_tm);
		
	// create logfile
	// [date]_log.txt for detailed daily logs - create new on boot or append
	// log.txt for overwiev - always append
	FILE *fDaily, *fLog;
	
	fDaily = fopen(filePath, "a");	
	fprintf(fDaily, "%s - Pi Restart\n", timestamp);
	fclose(fDaily);
	
	fLog = fopen("/home/pi/zisterne3/log/log.txt", "a");
	fprintf(fLog, "%s - Pi Restart\n", timestamp);	
	fclose(fLog);
	
	return 0;
}

