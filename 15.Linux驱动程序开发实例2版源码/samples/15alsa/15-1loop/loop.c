
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>  
#include <alsa/asoundlib.h>

static pthread_t id;
static snd_pcm_t *playback_handle;
static snd_pcm_t *capture_handle;
static int pcmrate=16000;
static int g_count=0;

#define BUFFSIZE_1M 1024*1024  

struct ring_buffer {  
    unsigned char buf[BUFFSIZE_1M];  
    unsigned int size;  
    unsigned int pin;  
    unsigned int pout;  
    pthread_mutex_t lock;  
};  

static struct ring_buffer *fifo = NULL;  

static int init_cycle_buffer(void)  
{
    fifo = (struct ring_buffer *) malloc(sizeof(struct ring_buffer));  
    if (NULL==fifo)  
	{
		 printf("malloc ring buffer error\n");
		 return -1;  
	}
    memset(fifo, 0, sizeof(struct ring_buffer));  
    fifo->size = BUFFSIZE_1M;  
    fifo->pin=0;
	fifo->pout=0;
    pthread_mutex_init(&fifo->lock, NULL);  
    return 0;  
}  

unsigned int fifo_get(unsigned char *buf, unsigned int len)  
{  
	if((NULL==buf)||(NULL==fifo))return 0;
	int ileftlen=0;
    if(fifo->pin>fifo->pout)
	{
		ileftlen=fifo->pin-fifo->pout;
	}
	else if(fifo->pin<fifo->pout)
	{
		ileftlen=fifo->size-fifo->pout+fifo->pin;
	}
	if(ileftlen==0)return 0;
	if(ileftlen<len)
	{
		len=ileftlen;
	}
	
	ileftlen=fifo->size-fifo->pout;
	if(ileftlen>=len)
	{
		memcpy(buf,&fifo->buf[fifo->pout],len);
	}
	else
	{
		char*pbuf=buf;
		memcpy(pbuf,&fifo->buf[fifo->pout],ileftlen);
		pbuf+=ileftlen;
		memcpy(pbuf,&fifo->buf[0],len-ileftlen);
	}
	fifo->pout+=len;
	if(fifo->pout>=fifo->size)
	{
		fifo->pout-=fifo->size;
	}
    return len;  
}  

unsigned int fifo_put(void *buf, unsigned int len)  
{  
	if((NULL==buf)||(NULL==fifo))return 0;
	if(len>=fifo->size)return 0;

	unsigned char*pbuf=(unsigned char*)buf;
	int ileftlen=fifo->size-fifo->pin;
	if(ileftlen>=len)
	{
		memcpy(&fifo->buf[fifo->pin],pbuf,len);
	}
	else
	{
		memcpy(&fifo->buf[fifo->pin],pbuf,ileftlen);
		pbuf+=ileftlen;
		memcpy(&fifo->buf[0],pbuf,len-ileftlen);
	}
	fifo->pin+=len;
	if(fifo->pin>=fifo->size)
	{
		fifo->pin-=fifo->size;
	}
    return len;  
}  

int set_hw_parameter(snd_pcm_t *audio_handle)
{
	int err;
	snd_pcm_hw_params_t *hw_params;
		
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n",
			 snd_strerror (err));
		return err;
	}
	if ((err = snd_pcm_hw_params_any (audio_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot initialize hardware parameter structure (%s)\n",
			 snd_strerror (err));
		return err;
	}
	if ((err = snd_pcm_hw_params_set_access (audio_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		fprintf (stderr, "cannot set access type (%s)\n",
			 snd_strerror (err));
		return err;
	}
	if ((err = snd_pcm_hw_params_set_format (audio_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		fprintf (stderr, "cannot set sample format (%s)\n",
			 snd_strerror (err));
		return err;
	}
	if ((err = snd_pcm_hw_params_set_rate_near (audio_handle, hw_params, &pcmrate, 0)) < 0) {
		fprintf (stderr, "cannot set sample rate (%s)\n",
			 snd_strerror (err));
		return err;
	}
	if ((err = snd_pcm_hw_params_set_channels (audio_handle, hw_params, 2)) < 0) {
		fprintf (stderr, "cannot set channel count (%s)\n",
			 snd_strerror (err));
		return err;
	}
	if ((err = snd_pcm_hw_params (audio_handle, hw_params)) < 0) {
		fprintf (stderr, "cannot set parameters (%s)\n",
			 snd_strerror (err));
		return err;
	}
	snd_pcm_hw_params_free (hw_params);
}


int initAlsa_playback()
{
	int err;
	if ((err = snd_pcm_open (&playback_handle, "plughw:0,1", SND_PCM_STREAM_PLAYBACK, 0)) < 0) 
	{
		fprintf (stderr, "cannot open audio device %s (%s)\n", 
			 "plughw:0,1",
			 snd_strerror (err));
		return err;
	}
	return set_hw_parameter(playback_handle);
}

int initAlsa_Record()
{
	int err;
	if ((err = snd_pcm_open (&capture_handle, "plughw:0,1", SND_PCM_STREAM_CAPTURE, 0)) < 0)
	{
		fprintf (stderr, "cannot open audio device %s (%s)\n", 
			 "plughw:0,1",
			 snd_strerror (err));
		return err;
	}
	return set_hw_parameter(capture_handle);
}

void playbackthread(void)
{
	int err;
	char buf[1024];  
     unsigned int n; 
	while(g_count<2);//缓冲一定的音频数据
	while(1)
	{
	   pthread_mutex_lock(&fifo->lock);  
        n = fifo_get(buf, sizeof(buf));  
        pthread_mutex_unlock(&fifo->lock);  
		if(n>0)
		{
			if ((err = snd_pcm_writei (playback_handle,buf, n/4)) != ( n/4)) 
			{
				fprintf (stderr, "write to audio interface failed (%s)\n",snd_strerror (err));
				snd_pcm_close (playback_handle);
				initAlsa_playback();
			}
		}
		else
		{
			usleep(1);
		}
	}
}

int main(void)
{
	int i=0;
	int err;
	short buf[10240];
	init_cycle_buffer();
	initAlsa_playback();
	initAlsa_Record();
	pthread_create(&id,NULL,(void *)playbackthread,NULL);
	if ((err = snd_pcm_prepare (capture_handle)) < 0) {
			fprintf (stderr, "cannot prepare audio interface for use (%s)\n",snd_strerror (err));
			exit (1);
	}
	while(1)
	{
		if ((err = snd_pcm_readi (capture_handle, buf, 256)) != 256) 
		{
			fprintf (stderr, "read from audio interface failed (%s) %d\n",snd_strerror (err),err);
			snd_pcm_prepare(capture_handle);
		}
		if(err>0)
		{
			pthread_mutex_lock(&fifo->lock);  
			fifo_put(buf,err*4);  
			pthread_mutex_unlock(&fifo->lock);  
			if(g_count<100)g_count++;
		}
	}
	snd_pcm_close (playback_handle);
	snd_pcm_close (capture_handle);
    exit(0);
}
