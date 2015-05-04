 /** @file simple_client.c
 *
 * @brief This simple client demonstrates the basic features of JACK
 * as they would be used by many applications.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <jack/jack.h>
#include "WaveFileReader.h"
#include "CircularBuffer2.h"
#include "Mixer/include/Mixer3D.h"

jack_port_t *output_port1, *output_port2;
jack_client_t *client;
FILE *filename;

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE   (200)
#define CB_SIZE (65536)
#define TEMP_SIZE (16384)

typedef struct
{
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
}
paTestData;

static void signal_handler(int sig)
{
	jack_deactivate(client);
	jack_client_close(client);
	fclose(filename);
	fprintf(stderr, "signal received, exiting ...\n");
	exit(0);
}

/**
 * The process callback for this JACK application is called in a
 * special realtime thread once for each audio cycle.
 *
 * This client follows a simple rule: when the JACK transport is
 * running, copy the input port to the output.  When it stops, exit.
 */

int
process (jack_nframes_t nframes, void *arg)
{
	//clock_t t1,t2;
	//t1=clock();

	//time_t time1 = time(NULL);

	struct timespec tstart={0,0}, tend={0,0};
    	clock_gettime(CLOCK_MONOTONIC, &tstart);

	jack_default_audio_sample_t *out1, *out2;
	//paTestData *data = (paTestData*)arg;
	//WaveFileReader *waveFileReader = (WaveFileReader *)arg;
	//unsigned short *buf=(unsigned short*)arg;
	//CircularBuffer<double> *circularBuffer = (CircularBuffer<double> *)arg;
	Mixer3D *mixer = (Mixer3D *)arg;
	int i;

	out1 = (jack_default_audio_sample_t*)jack_port_get_buffer (output_port1, nframes);
	out2 = (jack_default_audio_sample_t*)jack_port_get_buffer (output_port2, nframes);

	//circularBuffer->read(out1,nframes);
	//for(i=0;i<nframes;i++){
	//	out2[i]=out1[i];
	//}

	mixer->overlapConvolution(out1,out2);

	//printf("\nBuffer size: %d",sizeof(out1[0]));
	//for( i=0; i<nframes; i++ )
    //{
        //out1[i] = data->sine[data->left_phase];  /* left */
        //out2[i] = data->sine[data->right_phase];  /* right */
        //data->left_phase += 1;
        //if( data->left_phase >= TABLE_SIZE ) data->left_phase -= TABLE_SIZE;
        //data->right_phase += 3; /* higher pitch so we can distinguish left and right. */
        //if( data->right_phase >= TABLE_SIZE ) data->right_phase -= TABLE_SIZE;
    //}
	//t2=clock();
	//printf("\n %d clicks, %f seconds",t2,((float)(t2-t1))/CLOCKS_PER_SEC);

	//time_t time2 = time(NULL);
	//printf("\n time elaspsed %f",time2-time1);

    	clock_gettime(CLOCK_MONOTONIC, &tend);
   	//printf("some_long_computation took about %.5f seconds\n",((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec));

	return 0;
}

/**
 * JACK calls this shutdown_callback if the server ever shuts down or
 * decides to disconnect the client.
 */

void
jack_shutdown (void *arg)
{
	exit (1);
}

int
main (int argc, char *argv[])
{
	const char **ports;
	const char *client_name;
	const char *server_name = NULL;
	jack_options_t options = JackNullOption;
	jack_status_t status;
	paTestData data;
	int i;

	if (argc >= 2) {		/* client name specified? */
		client_name = argv[1];
		if (argc >= 3) {	/* server name specified? */
			server_name = argv[2];
            int my_option = JackNullOption | JackServerName;
			options = (jack_options_t)my_option;
		}
	} else {			/* use basename of argv[0] */
		client_name = strrchr(argv[0], '/');
		if (client_name == 0) {
			client_name = argv[0];
		} else {
			client_name++;
		}
	}

	for( i=0; i<TABLE_SIZE; i++ )
    {
        data.sine[i] = 0.2 * (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }
    data.left_phase = data.right_phase = 0;

	/* Open a file waveFileReader*/
	filename= fopen("waterfall2.wav","r");
	WaveFileReader waveFileReader(filename,true);
	CircularBuffer<double> circularBuffer(CB_SIZE);
	double tempBuff[TEMP_SIZE];

       	while(circularBuffer.writeSizeRemaining() >= TEMP_SIZE){
		waveFileReader.read(tempBuff,TEMP_SIZE);
		circularBuffer.write(tempBuff,TEMP_SIZE);
	}

	Mixer3D mixer(1024,11025,16,&circularBuffer);

////////Testing purpose////////////////////////////
/*	struct timespec tstart={0,0}, tend={0,0};
    	clock_gettime(CLOCK_MONOTONIC, &tstart);

	float a[1024],b[1024];
	mixer.overlapConvolution(a,b);

    	clock_gettime(CLOCK_MONOTONIC, &tend);
   	printf("-----%.5f seconds\n",
           ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
           ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec));
*/
//////////////////////////////////////////

	/* open a client connection to the JACK server */
	client = jack_client_open (client_name, options, &status, server_name);
	if (client == NULL) {
		fprintf (stderr, "jack_client_open() failed, "
			 "status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		exit (1);
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name(client);
		fprintf (stderr, "unique name `%s' assigned\n", client_name);
	}


	/* to set the buffer size*/
	//jack_set_buffer_size(client, 8192);


	/* tell the JACK server to call `process()' whenever
	   there is work to be done.
	*/

	jack_set_process_callback (client, process, &mixer);
	/* tell the JACK server to call `jack_shutdown()' if
	   it ever shuts down, either entirely, or if it
	   just decides to stop calling us.
	*/

	jack_on_shutdown (client, jack_shutdown, 0);

	/* create two ports */

	output_port1 = jack_port_register (client, "output1",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);

	output_port2 = jack_port_register (client, "output2",
					  JACK_DEFAULT_AUDIO_TYPE,
					  JackPortIsOutput, 0);

	if ((output_port1 == NULL) || (output_port2 == NULL)) {
		fprintf(stderr, "no more JACK ports available\n");
		exit (1);
	}

	/* Tell the JACK server that we are ready to roll.  Our
	 * process() callback will start running now. */

	if (jack_activate (client)) {
		fprintf (stderr, "cannot activate client");
		exit (1);
	}

	/* Connect the ports.  You can't do this before the client is
	 * activated, because we can't make connections to clients
	 * that aren't running.  Note the confusing (but necessary)
	 * orientation of the driver backend ports: playback ports are
	 * "input" to the backend, and capture ports are "output" from
	 * it.
	 */

	ports = jack_get_ports (client, NULL, NULL,
				JackPortIsPhysical|JackPortIsInput);
	if (ports == NULL) {
		fprintf(stderr, "no physical playback ports\n");
		exit (1);
	}

	if (jack_connect (client, jack_port_name (output_port1), ports[0])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	if (jack_connect (client, jack_port_name (output_port2), ports[1])) {
		fprintf (stderr, "cannot connect output ports\n");
	}

	free (ports);

    /* install a signal handler to properly quits jack client */
#ifdef WIN32
	signal(SIGINT, signal_handler);
    signal(SIGABRT, signal_handler);
	signal(SIGTERM, signal_handler);
#else
	signal(SIGQUIT, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
#endif

	/* keep running until the Ctrl+C */

	while (1) {
		#ifdef WIN32
			Sleep(1000);
		#else
			sleep (1);
		#endif

		while(circularBuffer.writeSizeRemaining() >= TEMP_SIZE){
			waveFileReader.read(tempBuff,TEMP_SIZE);
			circularBuffer.write(tempBuff,TEMP_SIZE);
		}
	}

	jack_client_close (client);
	exit (0);
}
