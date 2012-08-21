/* Simple C program that connects to MySQL Database server*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
// #define WITHMYSQL
#ifdef WITHMYSQL
#include </usr/include/mysql/mysql.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stddef.h>





//#define BAUDRATE B9600
#define BAUDRATE B57600
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FILE_RAW_MESSAGES    "data_raw.dat"
#define FILE_ENERGY          "data_energy.dat"

int keep_looping = TRUE;

unsigned int device_number, device_id; //generic data on package
unsigned long seq;

FILE *file;

void sigfun(int sig)
{
    printf("You have pressed Ctrl-C , aborting!");
    keep_looping = FALSE;
    exit(0);
}

#ifdef WITHMYSQL

int  put_into_energy_database(unsigned long dd, unsigned long ss, float ff, float ee, float kw, float hvac_power, unsigned long hvac_seq, float boiler_power, unsigned long boiler_seq, float ftx_power, unsigned long ftx_seq) {
// seq, frequency, effect, energy
                                // printf ("seq nr %lu; freqency: %0.1f; effect: %0.1f; energy: %0.1f @ %s\n\n", seq, freq, Wtot, kWhtot, asctime(local));
                                // put_into_energy_database(seq, light, wind, temperature, humidity);

// printf("A\n");
								
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
// printf("B\n");

//    char *server = "192.168.1.10"; // bubba
    char *server = "192.168.1.107"; // lamp64


//These are repeated in the next function as well, but can be made global...just in case you have different users for different databases...(or lazyness from my part)
    char *user = "jeemon";
    char *password = "jeemon"; /* set me first */
    char *database = "thermd";
    char *query;
    char query2 [800];

    conn = mysql_init(NULL);
// printf("C\n");

    /* Connect to database */
    if (!mysql_real_connect(conn, server,
                            user, password, database, 0, NULL, 0)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return(1);
    }
// printf("D\n");

    /* send SQL query */
    if (mysql_query(conn, "use thermd")) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return(1);
    }
    /* send SQL query */
// printf("E\n");

	time_t t;
	struct tm *local;
	char buffer[80];
	t = time(NULL);
	local = localtime(&t);
	strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", local);

    // sprintf(query, "insert into sensordata sensor_id, reading_dt, seq, freq, watt, kwh values(%u, %s, %lu, %.1f, %.1f, %.1f)", dd, asctime(local), ss, ff, ww, tt, hh);
    // sprintf(query2, "insert into energy (sensor_id, reading_dt, seq, freq, watt, kwh) values(%lu, %s, %lu, %.1f, %.1f, %.1f)", dd, asctime(local), ss, ff, ee, kw);
    sprintf(query2, "insert into energy (sensor_id, heating_seq, heating_freq, heating_power, heating_energy, hvac_power, hvac_seq, boiler_power, boiler_seq, ftx_power, ftx_seq) values(%lu, %lu, %.1f, %.1f, %.1f, %.1f, %lu, %.1f, %lu, %.1f, %lu)", dd, ss, ff, ee, kw, hvac_power, hvac_seq, boiler_power, boiler_seq, ftx_power, ftx_seq);
    // sprintf(query2, "insert into energy (sensor_id, reading_dt, seq, freq, watt, kwh) values(%lu, %s, %lu, %.1f, %.1f, %.1f)", dd, buffer, ss, ff, ee, kw);
// printf("%s\n", query2);
// printf("\n");

    if (mysql_query(conn, query2)) {
        fprintf(stderr, "%s\n", mysql_error(conn));
        return(1);
    }
// printf("G\n");

    mysql_close(conn);
}


#endif

int main(int argc, char *argv[]) {
    {
        int fd,c, res;

        unsigned long previous_seq = 0;
	unsigned long hvac_previous_seq = 0, hvac_seq = 0;
	unsigned long boiler_previous_seq = 0, boiler_seq = 0;
	unsigned long ftx_previous_seq = 0, ftx_seq = 0;

        struct termios oldtio,newtio;
        char buf[255];
        char message[255] = "still empty";
        int message_pointer = 0;
        int message_valid = 1;
        struct tm *local;
        time_t t;
	int message_length = 0;

        if(argc != 2)
        {
            printf("Incorrect number of args: usage jeelink_listerner <USB port> e.g. /dev/ttyUSB1\n");
            return(-1);
        }

        printf("Installing signal handler...");
        (void) signal(SIGINT, sigfun);
        printf("done\n");
#ifdef WITHMYSQL
        printf("With MySQL support\n");
#endif

        printf("connecting to port %s\n", argv[1]);

        fd = open(argv[1], O_RDWR | O_NOCTTY );
        if (fd <0) {perror(argv[1]); exit(-1); }

        tcgetattr(fd,&oldtio); /* save current port settings */

        bzero(&newtio, sizeof(newtio));
        newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;

        /* set input mode (non-canonical, no echo,...) */
        newtio.c_lflag = 0;

        newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
        newtio.c_cc[VMIN]     = 1;   /* blocking read until 1 chars received */

        tcflush(fd, TCIFLUSH);
        tcsetattr(fd,TCSANOW,&newtio);

        t = time(NULL);
        local = localtime(&t);


        printf("about to start listening\n");
        printf("Current time and date: %s", asctime(local));

        int jj;
/*
        res = read(fd,buf,255);
		printf("read length0, res: %i\n", res);
		for ( jj=0 ; jj<res ; jj=jj+1)
		{
			printf("[%i] - %x\n", jj, buf[jj]);
		}
*/
        while (keep_looping) {       /* loop for input */
            res = read(fd,buf,255);   /* returns after 1 chars have been input */

//printf("read length1, res: %i\n", res);

//buf[res]=0;               /* so we can printf... */
//            int jj;
            int value = 0;
//printf("buf[0] = %i\n", buf[0]);
            for ( jj=0 ; jj<res ; jj=jj+1)
            {
//                printf("[%i] - %x\n", jj, buf[jj]);
            }


            if ((buf[0] == 'O')) // and (buf[1] == "K")) == true)
//            if ((buf[0] == 'O') && (buf[1] == 'K')) //== true)
            {
                message[message_pointer] = 0;
                message_pointer = 0;
                //PRINT MESSAGE
//                printf("A: full message %s\n", message);
//printf("res2: %i\n", res);
                message_valid = 0;
            }

	    int msgLength = 4;
            for ( jj=0 ; jj<res ; jj=jj+1)
            {
//printf("buf: %c %i\n", buf[jj], buf[jj]);
//                if (buf[jj] == 'E')
		if ((buf[jj] == 'E') && (jj > msgLength))
		// Has entire message been read? 
//			if (1 == 0)
                {
                    //PRINT MESSAGE
                    message[message_pointer] = 0;
                    message_pointer++;
                    printf("B: full message: %s\n", message);
                    message_valid = 0;

                    //process it
                    t = time(NULL);
                    local = localtime(&t);

                    //print to raw data file
                    file = fopen(FILE_RAW_MESSAGES, "a+");
                    fprintf(file, "%s @ %s", message, asctime(local));
                    fclose(file);

                    char *lixo, *devid, *devl, *devm, *s0, *s1, *s2, *s3;

                    lixo = strtok (message, " "); //OK
                    lixo = strtok (NULL, " "); //1E  - size of package in bytes
                    devid = strtok (NULL, " "); //1   - rf12 header
                    lixo = strtok (NULL, " "); //1   - house number lsb, set by me in the JeeNode
                    lixo = strtok (NULL, " "); //0   - house number msb
                    devl = strtok (NULL, " "); //1   - device number lsb, set by me in the JeeNode lsb
                    devm = strtok (NULL, " "); //0   - device number msb
		    // Monitered devices
                    // 1 - Energy meter, electrical heating (EM24) 
                    // 2 - Energy meter, HVAC
                    // 3 - Energy meter, boiler
                    // 4 - Energy meter, FTX system

//                    device_number = atoi(devl)+atoi(devm)*256;
//                    printf("device number: %i\n", device_number);

			device_id = atoi(devid) & 0x1F;
			printf("device id: %i\n", device_id);

                    switch (device_id) {
			case 23:
			case 24:
			case 25:
				printf("JeeNode temp meter\n");

//				char *
				break;
                        case 1: 
				printf("Electricity energy meter\n");

                            char *flsb, *fmsb, *w0, *w1, *w2, *w3, *kwh0, *kwh1, *kwh2, *kwh3;
			char *hvac_seq0, *hvac_seq1, *hvac_seq2, *hvac_seq3;	// HVAC is on counter 1 in EM24
			char *boiler_seq0, *boiler_seq1, *boiler_seq2, *boiler_seq3;	// Boiler is on counter 1 in EM24
			char *ftx_seq0, *ftx_seq1, *ftx_seq2, *ftx_seq3;	// FTX is on counter 1 in EM24
			float freq, Wtot, kWhtot, counter1, hvac_power, hvac_energy, boiler_power, boiler_energy, ftx_power, ftx_energy;

                            s0 = strtok (NULL, " "); //sequence
                            s1 = strtok (NULL, " ");
                            s2 = strtok (NULL, " ");
                            s3 = strtok (NULL, " ");
                            flsb = strtok (NULL, " "); //frequency
                            fmsb = strtok (NULL, " ");
                            w0 = strtok (NULL, " "); //Wtot
                            w1 = strtok (NULL, " ");
                            w2 = strtok (NULL, " ");
                            w3 = strtok (NULL, " ");
                            kwh0 = strtok (NULL, " "); //kWtot
                            kwh1 = strtok (NULL, " ");
                            kwh2 = strtok (NULL, " ");
                            kwh3 = strtok (NULL, " ");
				hvac_seq0 = strtok(NULL, " "); // Counter 1
				hvac_seq1 = strtok (NULL, " ");
				hvac_seq2 = strtok (NULL, " ");
				hvac_seq3 = strtok (NULL, " ");

				boiler_seq0 = strtok(NULL, " "); // Counter 2
				boiler_seq1 = strtok (NULL, " ");
				boiler_seq2 = strtok (NULL, " ");
				boiler_seq3 = strtok (NULL, " ");

				ftx_seq0 = strtok (NULL, " "); // Counter 2
				ftx_seq1 = strtok (NULL, " ");
				ftx_seq2 = strtok (NULL, " ");
				ftx_seq3 = strtok (NULL, " ");
                            // printf ("processed: %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s that is it\n", s0, s1, s2, s3, flsb, fmsb, w0, w1, w2, w3, kwh0, kwh1, kwh2, kwh3);

                            seq = atoi(s0) + atoi(s1)*256 + atoi(s2)*256*256 + atoi(s3)*256*256*256;
							// unsigned long *t = &seq;
							// *t = atoi(s0);
							// *(t+1) = atoi(s1);
							// *(t+2) = atoi(s2);
							// *(t+3) = atoi(s3);

							// printf ("seq = %lu\n", seq);
                            if (seq != previous_seq)
                            {
//                                previous_seq = seq;

                                freq        = ((float) (atoi(flsb)+atoi(fmsb)*256)) / 10;
// printf("freq=%0.1f\n", freq);

				Wtot = ((float) (atoi(w0) + atoi(w1)*256 + atoi(w2)*256*256 + atoi(w3)*256*256*256)) / 10;
                                // *((&Wtot))  = atoi(w0);
                                // *((&Wtot)+1)  = atoi(w1);
                                // *((&Wtot)+2)  = atoi(w2);
                                // *((&Wtot)+3)  = atoi(w3);
                                // (&Wtot)[1]  = atoi(w1);
                                // (&Wtot)[2]  = atoi(w2);
                                // (&Wtot)[3]  = atoi(w3);
// printf("Wtot=%0.1f\n", Wtot);
				kWhtot = ((float) (atoi(kwh0) + atoi(kwh1)*256 + atoi(kwh2)*256*256 + atoi(kwh3)*256*256*256)) / 10;
								// long *t = &kWhtot;
								// *t = (unsigned short) atoi(kwh0);
								// *(t+1) = (unsigned short) atoi(kwh1);
                                // (&kWhtot)[0] = atoi(kwh1);
                                // (&kWhtot)[1] = atoi(kwh2);
                                // (&kWhtot)[2] = atoi(kwh3);
                                // (&kWhtot)[3] = atoi(kwh4);
// printf("kWhtot=%0.1f\n", kWhtot);

                                //printf ("converted: %i, %i, %i, %i, %i that is it\n", seq, light, wind, temp_raw, humi_raw);

				hvac_previous_seq = hvac_seq;
				hvac_seq = ((float) (atoi(hvac_seq0) + atoi(hvac_seq1)*256 + atoi(hvac_seq2)*256*256 + atoi(hvac_seq3)*256*256*256)) / 10;

				boiler_previous_seq = boiler_seq;
				boiler_seq = ((float) (atoi(boiler_seq0) + atoi(boiler_seq1)*256 + atoi(boiler_seq2)*256*256 + atoi(boiler_seq3)*256*256*256)) / 10;

				ftx_previous_seq = ftx_seq;
				ftx_seq = ((float) (atoi(ftx_seq0) + atoi(ftx_seq1)*256 + atoi(ftx_seq2)*256*256 + atoi(ftx_seq3)*256*256*256)) / 10;

printf("seq: %lu; previous_seq: %lu\n", seq, previous_seq);
printf("hvac_seq: %lu; hvac_previous_seq: %lu\n", hvac_seq, hvac_previous_seq);
printf("boiler_seq: %lu; boiler_previous_seq: %lu\n", boiler_seq, boiler_previous_seq);
printf("ftx_seq: %lu; ftx_previous_seq: %lu\n", ftx_seq, ftx_previous_seq);

				hvac_power = (hvac_seq - hvac_previous_seq) * 120 / (seq - previous_seq);
				boiler_power = (boiler_seq - boiler_previous_seq) * 1200 / (seq - previous_seq);
				ftx_power = (ftx_seq - ftx_previous_seq) * 1200 / (seq - previous_seq);

                                printf ("seq nr %lu; freqency:%0.1f; effect:%0.1f; energy:%0.1f; hvac_seq:%lu; hvac_power:%0.1f; boiler_seq:%lu; boiler_power:%0.1f; ftx_seq:%lu; ftx_power:%0.1f @ %s\n\n", seq, freq, Wtot, kWhtot, hvac_seq, hvac_power, boiler_seq, boiler_power, ftx_seq, ftx_power, asctime(local));

                                //print to raw data file
                                file = fopen(FILE_ENERGY, "a+");
                                fprintf(file, "%lu\t%0.1f\t%0.1f\t%0.1f\t@\t%s", seq, freq, Wtot, kWhtot, asctime(local));
                                fprintf(file, "%lu\t%0.1f\t%0.1f\t%0.1f\t%lu\t%0.1f\t%lu\t%0.1f\t%lu\t%0.1f\t@\t%s", seq, freq, Wtot, kWhtot, hvac_seq, hvac_power, boiler_seq, boiler_power, ftx_seq, ftx_power, asctime(local));
				fclose(file);

#ifdef WITHMYSQL
                                put_into_energy_database(device_number, seq, freq, Wtot, kWhtot, hvac_power, hvac_seq, boiler_power, boiler_seq, ftx_power, ftx_seq);
#endif
 				previous_seq = seq;	
                            }
				break;
                        default: printf("Unknown message %s\n\n\n", message);
                            break;
                    }
//
                }
                else
                {
                    message[message_pointer] = buf[jj];
                    message_pointer++;
// printf("BB\n");
                }
            }           

        }
        tcsetattr(fd,TCSANOW,&oldtio);
        return (0);
    }
}

