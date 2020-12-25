#include <stdio.h>
#include <wiringPi.h>
#include <time.h>
#include <math.h>

#define DEBUG 0
#define PULSE_LENGTH 10
#define SAMPLE_SIZE 20
#define SPEEDOFSOUND 0.034029	// cm / us

double mean(double* seriesptr);
double variance(double* seriesptr, double mean);
double ping();
double* getSeries();
void cleanSeries(double* seriesptr);
double allInOne(int log);
int compareDoubles(const void *p, const void *q);
void updateTimestamp();

FILE *fDaily, *fLog;
time_t now;
struct tm* now_tm;
char timestamp[17];	
char filePath[42];
int outliers=0;

// if (DEBUG) printf("");
int main()
{
	if (wiringPiSetup() == -1)
		return 1;
		
	pinMode (0, INPUT);		// Ultrasonic ECHO
	pinMode (1, OUTPUT);	// Ultrasonic TRIG
	pinMode (3, OUTPUT);	// Relay water
	pinMode (4, OUTPUT);	// Relay well
	
	digitalWrite (1, 0);	// Ultrasonic off
	digitalWrite(3,1);	// water off
	digitalWrite(4,1);	// well off
		
	double *seriesptr;
	double distance;
	int j, i;
	
	updateTimestamp();	
	strftime(filePath, 42, "/home/pi/zisterne3/log/%Y-%m-%d_log.txt",
		now_tm);
	
	if (DEBUG) printf(timestamp);
	
	// write log info
	// daily log
	fDaily = fopen(filePath, "a");
	fprintf(fDaily, "%s - DAILY START\n", timestamp);
	fclose(fDaily);
	// general log
	fLog = fopen("/home/pi/zisterne3/log/log.txt", "a");
	fprintf(fLog, "%s - DAILY START\n", timestamp);
	fclose(fLog);

	// 216 empty
	// 10 full
	// measure water level
	distance = allInOne(2);
	
	if(!distance)
	{
		if (DEBUG) printf("ERROR: 6 bad values in series. Ending program.");
		return 0;
	}
		
	//write logs
	fDaily = fopen(filePath, "a");
	fLog = fopen("/home/pi/zisterne3/log/log.txt", "a");
	if (distance > 76)
	{
		fprintf(fDaily, "\n%s - Refill Start\n", timestamp);
		fprintf(fLog, "\n%s - Refill Start\n", timestamp);	
	}
	else
	{
		fprintf(fDaily, "\n%s - Water level high - skipping refill\n", timestamp);
		fprintf(fLog, "\n%s - Water level high - skipping refill\n", timestamp);
	}		
	fclose(fDaily);
	fclose(fLog);
	
	// REFILL BLOCK
	// 0.6787 cm/min
	// 14min: 10cm
	// 5h: 2m
	// 193s / cycle
	// 3h: 55 cycles
	int cycle = 0;		
	while (distance > 76 && cycle < 55)
	{
		cycle++;
		digitalWrite(4,0); // well on
		delay(180000);
		distance = allInOne(1);
		fDaily = fopen(filePath, "a");
		fprintf(fDaily, "\tCycle %2i complete\n", cycle);
		fclose(fDaily);
	}
	digitalWrite(4,1);	// well off
	fLog = fopen("/home/pi/zisterne3/log/log.txt", "a");
	fprintf(fLog, "%s - Refill finished at %.2f cm after %2i cycles\n", timestamp, distance, cycle);
	fclose(fLog);
	
	updateTimestamp();
	
	// IRRIGATION BLOCK
	fDaily = fopen(filePath, "a");
	fLog = fopen("/home/pi/zisterne3/log/log.txt", "a");
	if (distance < 90)
	{		
		digitalWrite(3,0);	// water on		
		fprintf(fDaily, "%s - Irrigation Start\n", timestamp);
		fclose(fDaily);		
		fprintf(fLog, "%s - Irrigation Start\n", timestamp);
		fclose(fLog);		
		// 60000=1min
		// 3600000=1h
		// 7200000=2h
		delay(7200000);
		digitalWrite(3,1);	// water off
		distance = allInOne(2);
		
		//write logs
		fDaily = fopen(filePath, "a");
		fprintf(fDaily, "\n%s - Irrigation Stop\n", timestamp);
		fLog = fopen("/home/pi/zisterne3/log/log.txt", "a");
		fprintf(fLog, "\n%s - Irrigation Stop\n", timestamp);
	}
	else
	{
		fprintf(fDaily, "%s - Water level low - skipping irrigation\n", timestamp);
		fprintf(fLog, "%s - Water level low - skipping irrigation\n", timestamp);
	}	

	fprintf(fDaily, "%s - DAILY END\n", timestamp);
	fclose(fDaily);
	fprintf(fLog, "%s - DAILY END\n", timestamp);
	fclose(fLog);
	
	// Program finished
	return 0;
}

// distance to reflecting object in cm
double ping()
{
	// Send Pulse
	digitalWrite (1,1);	// On
	delayMicroseconds(PULSE_LENGTH);
	digitalWrite (1,0);	// Off

	// Wait for input
	while (digitalRead(0) == LOW)
		;

	long startTime = micros();
	while (digitalRead(0) == HIGH)
		;
	long travelTime = micros() - startTime;
	
	return SPEEDOFSOUND * (travelTime/2);
}	

// takes SAMPLE_SIZE measurements and fills an array
double* getSeries()
{
	double static distance[SAMPLE_SIZE];
	int i = 0, fails = 0;
	
	// reset global from last measurement
	outliers = 0;
	
	for (i = 0; i < SAMPLE_SIZE; i++)
	{
		distance[i] = ping();
		if (DEBUG) printf("\nping #%2i:\t%.2f", i, distance[i]);
		if (distance[i] < 10 || distance[i] > 250)
		{
			if (DEBUG) printf(" - FAIL");
			if (++fails > 5)
			{
				// return nullptr after 6 bad values
				// throw errors
				updateTimestamp();
				fDaily = fopen(filePath, "a");
				fLog = fopen("/home/pi/zisterne3/log/log.txt", "a");
				fprintf(fDaily, "%s - ERROR: 6 bad values in series. Ultrasonic module seems to be broken.\n", timestamp);				
				fprintf(fLog, "%s - ERROR: 6 bad values in series. Ultrasonic module seems to be broken.\n", timestamp);
				fclose(fDaily);
				fclose(fLog);
				digitalWrite (1, 0);	// Ultrasonic off
				digitalWrite (3, 1);	// water off
				digitalWrite (4, 1);	// well off
				return NULL;	
			}
		}
		delay(700);
	}
	if (DEBUG) printf("\nSeries finished with %i bad values.\n",fails);

	return distance;
}

// cleans array of outliers
void cleanSeries(double* seriesptr)
{
	double median, q1, q3, deltaQ;
	int q, i, j;
	
	// sort low -> high
	qsort(seriesptr, SAMPLE_SIZE, sizeof *seriesptr, &compareDoubles);
	
	// find median and low/high quartile q1/q3
	if (SAMPLE_SIZE % 2)
	{
		median = *(seriesptr+(int)(SAMPLE_SIZE/2));	
		q = (int)(SAMPLE_SIZE/2);
		q = q/2;
		q1 = (*(seriesptr+q-1) + *(seriesptr+q)) / 2;
		q3 = (*(seriesptr+3*q) + *(seriesptr+3*q+1)) / 2;
	}
	else
	{
		median = (*(seriesptr+(SAMPLE_SIZE/2)-1) + *(seriesptr+(SAMPLE_SIZE/2)))/2;
		q1 = *(seriesptr+(int)(SAMPLE_SIZE/4));
		q3 = *(seriesptr+(1+3*(int)(SAMPLE_SIZE/4)));
	}
	
	// only look for hard outliers
	deltaQ = (q3 - q1) * 3;
	
	if (DEBUG) printf("median\t%.2f\n",median);
	if (DEBUG) printf("q1\t%.2f\n",q1);
	if (DEBUG) printf("q3\t%.2f\n",q3);
	if (DEBUG) printf("deltaQ\t%.2f\n", deltaQ);
	
	// check for outliers and trim array
	for (j=0;j<SAMPLE_SIZE;j++)
		if (*(seriesptr+j) < (q1-deltaQ) || *(seriesptr+j) > (q3+deltaQ))
		{
			// outlier detected, move up all following elements
			if (DEBUG) printf("[%2i] = %.2f OUTLIER\n",j,*(seriesptr+j));
			outliers++;
			for (i=j;i<SAMPLE_SIZE-outliers;i++)
				*(seriesptr+i) = *(seriesptr+i+1);
		}
		else
			if (DEBUG) printf("[%2i] = %.2f OK\n",j,*(seriesptr+j));
	
	// new array size is now (SAMPLE_SIZE-outliers)
}

// takes measurements, cleans array and writes logs
double allInOne(int log)
{
	double distance, var, sd;
	double* seriesptr = getSeries();
	if (!seriesptr)
		return 0;
	cleanSeries(seriesptr);
	distance = mean(seriesptr);
	var = variance(seriesptr, distance);
	sd = sqrt(var);
	updateTimestamp();
	if (DEBUG) printf("Distance: %.1f cm with var=%.2f and sd=%.2f\n", distance, var, sd);
	switch(log)
	{
		case 2:		
		fLog = fopen("/home/pi/zisterne3/log/log.txt", "a");						
		fprintf(fLog, "%s - Distance: %.1f cm with var=%.2f and sd=%.2f", timestamp, distance, var, sd);		
		fclose(fLog);
		case 1:
		fDaily = fopen(filePath, "a");
		fprintf(fDaily, "%s - Distance: %.1f cm with var=%.2f and sd=%.2f", timestamp, distance, var, sd);	
		fclose(fDaily);
		default:
		break;
	}
	return distance;
}

// rules used in qsort
int compareDoubles(const void *p, const void *q)
{
	double x = *(const double *)p;
	double y = *(const double *)q;
	if (x < y)	return -1;
	else if (x > y)	return 1;
	return 0;
}

// updates the global timestamp string
void updateTimestamp()
{
	time(&now);	
	now_tm = localtime(&now);		
	strftime (timestamp, 17, "%Y-%m-%d %H:%M", now_tm);
}

// calculate mean of array
double mean(double* seriesptr)
{
	int i;
	double mean=0.0;
	for (i=0;i<SAMPLE_SIZE-outliers;i++)
		mean += *(seriesptr+i);
	return (mean / (SAMPLE_SIZE-outliers));
}

// calculate variance of array
double variance(double* seriesptr, double mean)
{
	int i;
	double variance=0.0;
	for (i=0;i<SAMPLE_SIZE-outliers;i++)
		variance += pow(*(seriesptr+i)-mean, 2);
	return (variance / (SAMPLE_SIZE-outliers));
}
