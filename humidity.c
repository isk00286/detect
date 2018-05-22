#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<unistd.h>
#include<signal.h>
#include<wiringPi.h>

#define FAN 22
#define DCMOTOR 23
#define RGBLEDPOWER 24

#define RED 7
#define BLUE 8
#define GREEN 9

#define MAXTIMINGS 85

void Bpluspinmodeset(void);
void sig_handler(int signo);

int ret_humid;

static int DHTPIN = 11;
static int dht22_dat[5] = {0,0,0,0,0};
static uint8_t sizecvt(const int read)
{
	if (read>255 || read<0)
	{
		printf("Invalid data from wiringPi library\n");
		exit(EXIT_FAILURE);
	}
	return (uint8_t)read;
}
int main(void)
{
	if(wiringPicheck()) printf("Fail");
	signal(SIGINT, (void *)sig_handler);	
	Bpluspinmodeset();

	int curhumid = 0;

	while(1)
	{    
		curhumid = get_humidity_sensor();
		if(curhumid>70)
		{
			act_fan_on();
			act_dcmotor_on();
			act_rgbled_on();
			delay(5000);
			act_fan_off();
			act_dcmotor_off();
			act_rgbled_off();
		}
		delay(10000);
	}
	return 0;
}

int wiringPicheck(void)
{
	if(wiringPiSetup() == -1)
	{
		fprintf(stdout, "Unable to start wiringPi: %s\n", strerror(errno));
		return 1;
	}
}

void Bpluspinmodeset(void)
{
	pinMode(FAN, OUTPUT);
	pinMode(DCMOTOR, OUTPUT);
	pinMode(RGBLEDPOWER, OUTPUT);
	pinMode(RED, OUTPUT);
  pinMode(BLUE, OUTPUT);
	pinMode(GREEN, OUTPUT);
}

int get_humidity_sensor()
{
	int received_humid;
	
	DHTPIN=11;
	
	if(wiringPiSetup() == -1)
		exit(EXIT_FAILURE);
	
	if(setuid(getuid()) <0)
	{
		perror("Dropping privileges failed\n");
		exit(EXIT_FAILURE);
	}
	while(read_dht22_dat_humid() == 0)
	{
		delay(500);
	}
	
	received_humid = ret_humid;
	printf("Humidity = %d\n", received_humid);

	return received_humid;
}

int read_dht22_dat_humid()
{
	uint8_t laststate = HIGH;
	uint8_t counter = 0;
	uint8_t j = 0, i;

	dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;

	// pull pin down for 18 milliseconds
	pinMode(DHTPIN, OUTPUT);
	digitalWrite(DHTPIN, HIGH);
	delay(10);
	digitalWrite(DHTPIN, LOW);
	delay(18);
	// then pull it up for 40 microseconds
	digitalWrite(DHTPIN, HIGH);
	delayMicroseconds(40); 
	// prepare to read the pin
	pinMode(DHTPIN, INPUT);

	// detect change and read data
	for ( i=0; i< MAXTIMINGS; i++) 
	{
		counter = 0;
		while (sizecvt(digitalRead(DHTPIN)) == laststate) 
		{
			counter++;
			delayMicroseconds(1);
			if (counter == 255) {
			break;
		}
    }
    laststate = sizecvt(digitalRead(DHTPIN));

    if (counter == 255) break;

    // ignore first 3 transitions
    if ((i >= 4) && (i%2 == 0)) 
	{
		// shove each bit into the storage bytes
		dht22_dat[j/8] <<= 1;
		if (counter >70)
			dht22_dat[j/8] |= 1;
		j++;
    }
}

  // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
  // print it out if data is good
	if ((j >= 40) && 
      (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) ) {
        float h;
		
        h = (float)dht22_dat[0] * 256 + (float)dht22_dat[1];
        h /= 10;
		
		ret_humid = (int)h;
		printf("Humidity = %.2f%%\n", h);
		printf("Humidity = %d\n", ret_humid);
		
    return ret_humid;
	}
	else
	{
		printf("Data not good, skip\n");
		return 0;
	}
}

void act_fan_on()
{
	if(wiringPicheck()) printf("Fail");
	pinMode (FAN, OUTPUT);
	digitalWrite (FAN, 1) ; // On    
	
}
void act_fan_off()
{
	if(wiringPicheck()) printf("Fail");
	pinMode (FAN, OUTPUT);
	digitalWrite (FAN, 0) ; // On 
	
}
void act_dcmotor_on()
{
	if(wiringPicheck()) printf("Fail");
	pinMode (DCMOTOR, OUTPUT);
	digitalWrite(DCMOTOR, 1); //On    
}

void act_dcmotor_off()
{
	if(wiringPicheck()) printf("Fail");
	pinMode (DCMOTOR, OUTPUT);
	digitalWrite(DCMOTOR, 0); //On
}

void act_rgbled_on()
{
	if(wiringPicheck()) printf("Fail");
	pinMode(RGBLEDPOWER, OUTPUT);
	pinMode(RED, OUTPUT);
	pinMode(BLUE, OUTPUT);
	pinMode(GREEN, OUTPUT);
	
	digitalWrite(RGBLEDPOWER, 1); //PWR on
	digitalWrite(RED, 1); //PWR on
}
void act_rgbled_off()
{
	if(wiringPicheck()) printf("Fail");
	pinMode(RGBLEDPOWER, OUTPUT);
	pinMode(RED, OUTPUT);
	pinMode(BLUE, OUTPUT);
	pinMode(GREEN, OUTPUT);
 
	digitalWrite(RGBLEDPOWER, 0); //PWR on
	digitalWrite(RED, 0); //PWR o
}

void sig_handler(int signo)
{
	printf("process stop\n");
		digitalWrite(FAN, 0);
		digitalWrite(DCMOTOR, 0);
		digitalWrite(RED, 0);
		digitalWrite(GREEN, 0);
		digitalWrite(BLUE, 0);
		digitalWrite(RGBLEDPOWER, 0);
		
		exit(0);
}
