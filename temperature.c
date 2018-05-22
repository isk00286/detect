#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <wiringPi.h>

#define PUMP	21	// BCM_GPIO 5
#define MAXTIMINGS 85

void sig_handler(int signo);
void Bpluspinmodeset(void);

static int DHTPIN = 11;
static int dht22_dat[5] = {0,0,0,0,0};

int ret_temp;

int main (void)
{
	if(wiringPicheck()) printf("Fail");
	signal(SIGINT, (void *)sig_handler);	
	Bpluspinmodeset();
	
	int curtemp = 0;
	
	while(1)
	{
		curtemp = get_temperature_sensor();
		printf("%d \n", curtemp);
		
		if(curtemp > 20) // On    
		{

			act_waterpump_on();
			delay(2000);
			act_waterpump_off();
		}
		delay(5000);
	}

  
	return 0 ;
}

int wiringPicheck(void)
{
	if (wiringPiSetup () == -1)
	{
		fprintf(stdout, "Unable to start wiringPi: %s\n", strerror(errno));
		return 1 ;
	}
}

void Bpluspinmodeset(void)
{

	pinMode (PUMP, OUTPUT);
}



int get_temperature_sensor()
{
	int received_temp;

	DHTPIN = 11;

	if (wiringPiSetup() == -1)
		exit(EXIT_FAILURE) ;
	
	if (setuid(getuid()) < 0)
	{
		perror("Dropping privileges failed\n");
		exit(EXIT_FAILURE);
	}
	while (read_dht22_dat_temp() == 0)
	{
		delay(1000); // wait 1sec to refresh
	}
	received_temp = ret_temp ;
	printf("Temperature = %d\n", received_temp);
	
	return received_temp;
}

static uint8_t sizecvt(const int read)
{
  /* digitalRead() and friends from wiringpi are defined as returning a value
  < 256. However, they are returned as int() types. This is a safety function */

  if (read > 255 || read < 0)
  {
    printf("Invalid data from wiringPi library\n");
    exit(EXIT_FAILURE);
  }
  return (uint8_t)read;
}

int read_dht22_dat_temp()
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
  for ( i=0; i< MAXTIMINGS; i++) {
    counter = 0;
    while (sizecvt(digitalRead(DHTPIN)) == laststate) {
      counter++;
      delayMicroseconds(1);
      if (counter == 255) {
        break;
      }
    }
    laststate = sizecvt(digitalRead(DHTPIN));

    if (counter == 255) break;

    // ignore first 3 transitions
    if ((i >= 4) && (i%2 == 0)) {
      // shove each bit into the storage bytes
      dht22_dat[j/8] <<= 1;
      if (counter > 50 )
        dht22_dat[j/8] |= 1;
      j++;
    }
  }

  // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
  // print it out if data is good
  if ((j >= 40) && 
      (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) ) {
        float t;
		
        t = (float)(dht22_dat[2] & 0x7F)* 256 + (float)dht22_dat[3];
        t /= 10.0;
        if ((dht22_dat[2] & 0x80) != 0)  t *= -1;
		
		ret_temp = (int)t;
		printf("Temperature = %.2f *C \n", t );
		printf("Temperature = %d\n",ret_temp);
		
    return ret_temp;
  }
  else
  {
    printf("Data not good, skip\n");
    return 0;
  }
}


void act_waterpump_on()
{
	if(wiringPicheck()) printf("Fail");
	pinMode (PUMP, OUTPUT);
	digitalWrite (PUMP, 1) ; // On    
	
}
void act_waterpump_off()
{
	if(wiringPicheck()) printf("Fail");
	pinMode (PUMP, OUTPUT);
	digitalWrite (PUMP, 0) ; // On    
}
void sig_handler(int signo)
{
	printf("process stop\n");
	
	digitalWrite(PUMP, 0);
	exit(0);
}
