/****************************************************************/
/*            Apple IIgs emulator                               */
/*            Copyright 1996 Kent Dickey                        */
/*                                                              */
/*    This code may not be used in a commercial product         */
/*    without prior written permission of the author.           */
/*                                                              */
/*    You may freely distribute this code.                      */ 
/*                                                              */
/*    You can contact the author at kentd@cup.hp.com.           */
/*    HP has nothing to do with this software.                  */
/****************************************************************/

const char rcsid_sound_driver_c[] = "@(#)$Header: sound_driver.c,v 1.4 99/05/03 22:02:52 kentd Exp $";

#include "defc.h"
#include "sound.h"

#ifdef HPUX
# include <sys/audio.h>
# include "Alib.h"
#endif

#if defined(__linux__) || defined(OSS)
# include <sys/soundcard.h>
#endif

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
extern int errno;
#else
#include <windows.h>
#include <mmsystem.h>
#include <tchar.h>
#define NUM_BUFFERS 10 
void CheckWaveError(TCHAR *,int);
void child_sound_init_win32(void *);

HWAVEOUT WaveHandle;
WAVEHDR  WaveHeader[NUM_BUFFERS];
extern int g_pipe_fd[2];
#endif

extern int g_audio_enable;
extern int Verbose;
extern int g_use_alib;
extern int g_audio_rate;

int    g_preferred_rate =48000;
int    g_audio_socket = -1;
int    g_bytes_written = 0;
#ifdef HPUX
Audio    *g_audio = 0;
ATransID g_xid;
#endif

#define ZERO_BUF_SIZE        2048

word32 zero_buf2[ZERO_BUF_SIZE];

#define ZERO_PAUSE_SAFETY_SAMPS     (g_audio_rate >> 5)
#define ZERO_PAUSE_NUM_SAMPS        (4*g_audio_rate)

void child_sound_init_linux();
void child_sound_init_hpdev();
void child_sound_init_alib();
void child_sound_init_win32(void *);

#ifdef _WIN32
unsigned int __stdcall child_sound_loop_win32(void *);
#endif

void
reliable_buf_write(word32 *shm_addr, int pos, int size)
{
    byte    *ptr;
    int     ret,i,found=0;
    int wave_buf=0;

    if(size < 1 || pos < 0 || pos > SOUND_SHM_SAMP_SIZE ||
                size > SOUND_SHM_SAMP_SIZE ||
                (pos + size) > SOUND_SHM_SAMP_SIZE) {
        printf("reliable_buf_write: pos: %04x, size: %04x\n",
            pos, size);
        exit(1);
    }

    ptr = (byte *)&(shm_addr[pos]);
    size = size * 4;

    #ifndef _WIN32
    while(size > 0) {
        ret = write(g_audio_socket, ptr, size);

        if(ret < 0) {
            printf("audio write, errno: %d\n", errno);
            exit(1);
        }
        size = size - ret;
        ptr += ret;
        g_bytes_written += ret;
    }
    #else
    i=0;
    found=0;
    while ((i < NUM_BUFFERS)) {
		if (WaveHeader[i].dwUser == FALSE) {
            wave_buf=i;
            found=1;
            break;
        }
        i++;
    }

    if (!found) {
        return ;
    }

    CopyMemory(WaveHeader[wave_buf].lpData,ptr,size);
    WaveHeader[wave_buf].dwBufferLength=size;
    WaveHeader[wave_buf].dwUser=TRUE;

    ret=waveOutWrite(WaveHandle,&WaveHeader[wave_buf],
                     sizeof(WaveHeader));
    CheckWaveError(TEXT("Writing wave out"),ret);

    g_bytes_written += (size);
    #endif
}

void
reliable_zero_write(int amt)
{
    int len;
    #ifndef _WIN32
    while (amt > 0) {
        len=MIN(amt,ZERO_BUF_SIZE);
        reliable_buf_write(zero_buf2,0,len);
        amt-=len;
    }
    #else
    reliable_buf_write(zero_buf2, 0, amt);
    #endif
}

void
child_sound_loop(int read_fd, int write_fd, word32 *shm_addr)
{
    word32 tmp;
    long   status_return;
    int    zeroes_buffered;
    int    zeroes_seen;
    int    sound_paused;
    int    vbl;
    int    size;
    int    pos;
    int    ret;

    doc_printf("Child pipe fd: %d\n", read_fd);

    g_audio_rate = g_preferred_rate;

#ifdef HPUX
    if(g_use_alib) {
        child_sound_init_alib();
    } else {
        child_sound_init_hpdev();
    }
#endif
#if defined(__linux__) || defined(OSS)
    child_sound_init_linux();
#endif
#if defined(_WIN32)
    child_sound_init_win32(shm_addr);
#endif

    tmp = g_audio_rate;
    
    ret = write(write_fd, &tmp, 4);
    if(ret != 4) {
        printf("Unable to send back audio rate to parent\n");
        exit(1);
    }

    #ifndef _WIN32
    close(write_fd);
    #endif

	if (!g_audio_enable) {
		return;
	}

    zeroes_buffered = 0;
    zeroes_seen = 0;
    sound_paused = 0;

    pos = 0;
    vbl = 0;
    while(1) {
        errno = 0;
        ret = read(read_fd, &tmp, 4);

        if(ret <= 0) {
            printf("child dying from ret: %d, errno: %d\n",
                ret, errno);
            break;
        }
        
        size = tmp & 0xffffff;

        if((tmp >> 24) == 0xa2) {
            /* play sound here */

            if(sound_paused) {
                //printf("Unpausing sound, zb: %d\n",
                //    zeroes_buffered);
                sound_paused = 0;
            }


#if 0 
            pos += zeroes_buffered;
            while(pos >= SOUND_SHM_SAMP_SIZE) {
                pos -= SOUND_SHM_SAMP_SIZE;
            }
#endif

            if(zeroes_buffered) {
                reliable_zero_write(zeroes_buffered);
            }

            zeroes_buffered = 0;
            zeroes_seen = 0;

			if((size + pos) > SOUND_SHM_SAMP_SIZE) {
				reliable_buf_write(shm_addr, pos,
						SOUND_SHM_SAMP_SIZE - pos);
				size = (pos + size) - SOUND_SHM_SAMP_SIZE;
				pos = 0;
			}
            reliable_buf_write(shm_addr, pos, size);

        } else if((tmp >> 24) == 0xa1) {
            if(sound_paused) {
                if(zeroes_buffered < ZERO_PAUSE_SAFETY_SAMPS) {
                    zeroes_buffered += size;
                }
            } else {
                /* not paused, send it through */
                zeroes_seen += size;

                reliable_zero_write(size);

                if(zeroes_seen >= ZERO_PAUSE_NUM_SAMPS) {
                    //printf("Pausing sound\n");
                    sound_paused = 1;
                }
            }
        } else {
            printf("tmp received bad: %08x\n", tmp);
            exit(3);
        }

        pos = pos + size;
        while(pos >= SOUND_SHM_SAMP_SIZE) {
            pos -= SOUND_SHM_SAMP_SIZE;
        }

        vbl++;
        if(vbl >= 60) {
            vbl = 0;
#if 0
            printf("sound bytes written: %06x\n", g_bytes_written);
            printf("Sample samples[0]: %08x %08x %08x %08x\n",
                shm_addr[0], shm_addr[1],
                shm_addr[2], shm_addr[3]);
            printf("Sample samples[100]: %08x %08x %08x %08x\n",
                shm_addr[100], shm_addr[101],
                shm_addr[102], shm_addr[103]);
#endif
            g_bytes_written = 0;
        }
    }


#ifdef HPUX
    if(g_use_alib) {
        ACloseAudio(g_audio, &status_return);
    } else {
        ioctl(g_audio_socket, AUDIO_DRAIN, 0);
    }
#endif
    close(g_audio_socket);

    exit(0);
}


#ifdef HPUX
void
child_sound_init_hpdev()
{
    struct audio_describe audio_descr;
    int    output_channel;
    char    *str;
    int    speaker;
    int    ret;
    int    i;

    g_audio_socket = open("/dev/audio", O_WRONLY, 0);
    if(g_audio_socket < 0) {
        printf("open /dev/audio failed, ret: %d, errno:%d\n",
            g_audio_socket, errno);
        exit(1);
    }

    ret = ioctl(g_audio_socket, AUDIO_DESCRIBE, &audio_descr);
    if(ret < 0) {
        printf("ioctl AUDIO_DESCRIBE failed, ret:%d, errno:%d\n",
            ret, errno);
        exit(1);
    }

    for(i = 0; i < audio_descr.nrates; i++) {
        printf("Audio rate[%d] = %d\n", i,
            audio_descr.sample_rate[i]);
    }

    ret = ioctl(g_audio_socket, AUDIO_SET_DATA_FORMAT,
            AUDIO_FORMAT_LINEAR16BIT);
    if(ret < 0) {
        printf("ioctl AUDIO_SET_DATA_FORMAT failed, ret:%d, errno:%d\n",
            ret, errno);
        exit(1);
    }

    ret = ioctl(g_audio_socket, AUDIO_SET_CHANNELS, NUM_CHANNELS);
    if(ret < 0) {
        printf("ioctl AUDIO_SET_CHANNELS failed, ret:%d, errno:%d\n",
            ret, errno);
        exit(1);
    }

    ret = ioctl(g_audio_socket, AUDIO_SET_TXBUFSIZE, 16*1024);
    if(ret < 0) {
        printf("ioctl AUDIO_SET_TXBUFSIZE failed, ret:%d, errno:%d\n",
            ret, errno);
        exit(1);
    }

    ret = ioctl(g_audio_socket, AUDIO_SET_SAMPLE_RATE, g_audio_rate);
    if(ret < 0) {
        printf("ioctl AUDIO_SET_SAMPLE_RATE failed, ret:%d, errno:%d\n",
            ret, errno);
        exit(1);
    }

    ret = ioctl(g_audio_socket, AUDIO_GET_OUTPUT, &output_channel);
    if(ret < 0) {
        printf("ioctl AUDIO_GET_OUTPUT failed, ret:%d, errno:%d\n",
            ret, errno);
        exit(1);
    }


    speaker = 1;
    str = getenv("SPEAKER");
    if(str) {
        if(str[0] != 'i' && str[0] != 'I') {
            speaker = 0;
        }
    }

    if(speaker) {
        printf("Sending sound to internal speaker\n");
        output_channel |= AUDIO_OUT_SPEAKER;
    } else {
        printf("Sending sound to external jack\n");
        output_channel &= (~AUDIO_OUT_SPEAKER);
        output_channel |= AUDIO_OUT_HEADPHONE;
    }

    ret = ioctl(g_audio_socket, AUDIO_SET_OUTPUT, output_channel);
    if(ret < 0) {
        printf("ioctl AUDIO_SET_OUTPUT failed, ret:%d, errno:%d\n",
            ret, errno);
        exit(1);
    }
}


void
child_sound_init_alib()
{
    AOutputChMask    chan_mask;
    AOutputDstMask    dest_mask;
    AudioAttributes attr;
    AudioAttributes *attr_ptr;
    SSPlayParams    play_params;
    SStream        audio_stream;
    AGainEntry    gain_entries[2];
    long        status_return;
    char    *str;
    int    speaker;
    int    ret;

    status_return = 0;
    g_audio = AOpenAudio((char *)0, &status_return);
    if(g_audio == (Audio *)0) {
        printf("AopenAudio failed, ret: %ld\n", status_return);
        exit(1);
    }

    chan_mask = AOutputChannels(g_audio);
    doc_printf("Output channel mask: %08x\n", (word32)chan_mask);

    dest_mask = AOutputDestinations(g_audio);
    doc_printf("Output destinations mask: %08x\n", (word32)dest_mask);

    attr_ptr = ABestAudioAttributes(g_audio);
    attr = *attr_ptr;

    attr.attr.sampled_attr.data_format = ADFLin16;
    attr.attr.sampled_attr.bits_per_sample = 8*SAMPLE_SIZE;
    attr.attr.sampled_attr.sampling_rate = g_audio_rate;
    attr.attr.sampled_attr.channels = NUM_CHANNELS;
    attr.attr.sampled_attr.interleave = 1;
    attr.attr.sampled_attr.duration.type = ATTFullLength;

    gain_entries[0].u.o.out_ch = AOCTLeft;
    gain_entries[1].u.o.out_ch = AOCTLeft;
    gain_entries[0].gain = AUnityGain;
    gain_entries[1].gain = AUnityGain;

    speaker = 1;
    str = getenv("SPEAKER");
    if(str) {
        if(str[0] != 'i' && str[0] != 'I') {
            speaker = 0;
        }
    }

    if(speaker) {
        printf("Sending sound to internal speaker\n");
        gain_entries[0].u.o.out_dst = AODTLeftIntSpeaker;
        gain_entries[1].u.o.out_dst = AODTRightIntSpeaker;
    } else {
        printf("Sending sound to external jack\n");
        gain_entries[0].u.o.out_dst = AODTLeftJack;
        gain_entries[1].u.o.out_dst = AODTRightJack;
    }

    play_params.gain_matrix.type = AGMTOutput;
    play_params.gain_matrix.num_entries = NUM_CHANNELS;
    play_params.gain_matrix.gain_entries = &(gain_entries[0]);
    play_params.play_volume = AUnityGain;
    play_params.priority = APriorityNormal;
    play_params.event_mask = 0;

    g_xid = APlaySStream(g_audio, -1, &attr, &play_params, &audio_stream,
        &status_return);

    doc_printf("g_xid: %ld, status_return: %ld\n", (long)g_xid,
        status_return);

    g_audio_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(g_audio_socket < 0) {
        printf("socket create, errno: %d\n", errno);
        exit(1);
    }

    doc_printf("g_audio_socket: %d\n", g_audio_socket);

    ret = connect(g_audio_socket,
            (struct sockaddr *)&audio_stream.tcp_sockaddr,
            sizeof(struct sockaddr_in) );

    if(ret < 0) {
        printf("connect failed, errno: %d\n", errno);
        exit(1);
    }
}
#endif

#if defined(__linux__) || defined(OSS)
void
child_sound_init_linux()
{
    int    stereo;
    int    sample_size;
    int    rate;
    int    fragment;
    int    fmt;
    char    *str;
    int    ret;
    int    i;

    g_audio_socket = open("/dev/dsp", O_WRONLY, 0);
    if(g_audio_socket < 0) {
        printf("open /dev/audio failed, ret: %d, errno:%d\n",
            g_audio_socket, errno);
        exit(1);
    }

#if 0
    fragment = 0x00200009;
    ret = ioctl(g_audio_socket, SNDCTL_DSP_SETFRAGMENT, &fragment);
    if(ret < 0) {
        printf("ioctl SETFRAGEMNT failed, ret:%d, errno:%d\n",
            ret, errno);
        exit(1);
    }
#endif

    sample_size = 16;
    ret = ioctl(g_audio_socket, SNDCTL_DSP_SAMPLESIZE, &sample_size);
    if(ret < 0) {
        printf("ioctl SNDCTL_DSP_SAMPLESIZE failed, ret:%d, errno:%d\n",
            ret, errno);
        exit(1);
    }

#ifdef KEGS_LITTLE_ENDIAN
    fmt = AFMT_S16_LE;
#else
    fmt = AFMT_S16_BE;
#endif
    ret = ioctl(g_audio_socket, SNDCTL_DSP_SETFMT, &fmt);
    if(ret < 0) {
        printf("ioctl SNDCTL_DSP_SETFMT failed, ret:%d, errno:%d\n",
            ret, errno);
        exit(1);
    }

    stereo = 1;
    ret = ioctl(g_audio_socket, SNDCTL_DSP_STEREO, &stereo);
    if(ret < 0) {
        printf("ioctl SNDCTL_DSP_STEREO failed, ret:%d, errno:%d\n",
            ret, errno);
        exit(1);
    }

    rate = g_audio_rate;
    ret = ioctl(g_audio_socket, SNDCTL_DSP_SPEED, &rate);
    if(ret < 0) {
        printf("ioctl SNDCTL_DSP_SPEED failed, ret:%d, errno:%d\n",
            ret, errno);
        exit(1);
    }
    if(ret > 0) {
        rate = ret;    /* rate is returned value */
    }
    if(rate < 8000) {
        printf("Audio rate of %d which is < 8000!\n", rate);
        exit(1);
    }
    
    g_audio_rate = rate;

    printf("Sound initialized\n");
}
#endif

#ifdef _WIN32
void CheckWaveError(TCHAR *s, int res) {
    TCHAR message[256];
    if (res == MMSYSERR_NOERROR) {
        return;
    }
    waveOutGetErrorText(res,message,sizeof(message));
    _tprintf (TEXT("%s: %s\n"),s,message);
    exit(1);
}

unsigned int __stdcall child_sound_loop_win32(void *param) {
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
    child_sound_loop(g_pipe_fd[0],g_pipe_fd[1],param);
	return 0;
}

void CALLBACK handle_wav_snd(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance,
                             DWORD dwParam1, DWORD dwParam2)
{
    /* Only service "buffer done playing" messages */
    if ( uMsg == WOM_DONE ) {
		if (((LPWAVEHDR) dwParam1)->dwFlags == (WHDR_DONE | WHDR_PREPARED)) {
			((LPWAVEHDR)dwParam1)->dwUser=FALSE;
		}
        return;
    }
}

void child_sound_init_win32(void *shmaddr) {
    int res,i;
    WAVEFORMATEX WaveFmt;
    WAVEOUTCAPS caps;
    byte *tmp;
    int blen;

    memset(&WaveFmt, 0, sizeof(WAVEFORMATEX));
    WaveFmt.wFormatTag = WAVE_FORMAT_PCM;
    WaveFmt.wBitsPerSample = 16;
    WaveFmt.nChannels = 2;
    WaveFmt.nSamplesPerSec = g_audio_rate;
    WaveFmt.nBlockAlign = 
        WaveFmt.nChannels * (WaveFmt.wBitsPerSample/8);
    WaveFmt.nAvgBytesPerSec = 
        WaveFmt.nSamplesPerSec * WaveFmt.nBlockAlign;

    res=waveOutOpen(&WaveHandle,WAVE_MAPPER,&WaveFmt,0,0,WAVE_FORMAT_QUERY);

	if (res != MMSYSERR_NOERROR) {
		printf ("Cannot open audio device\n");
		g_audio_enable=0;
		return;
	}

    res=waveOutOpen(&WaveHandle,WAVE_MAPPER,&WaveFmt,(DWORD)handle_wav_snd,
                    0,CALLBACK_FUNCTION | WAVE_ALLOWSYNC );

	if (res != MMSYSERR_NOERROR) {
		printf ("Cannot open audio device\n");
		g_audio_enable=0;
		return;
	}

    g_audio_rate= WaveFmt.nSamplesPerSec;

    blen=SOUND_SHM_SAMP_SIZE*2;
	tmp=malloc(blen*NUM_BUFFERS);
    if (tmp==NULL) {
        printf ("Unable to allocate sound buffer\n");
        exit(1);
    }
    for (i=0;i<NUM_BUFFERS;i++) {
        memset(&WaveHeader[i],0,sizeof(WAVEHDR)); 
		// dwUser contains the busy state
        WaveHeader[i].dwUser=FALSE;
        WaveHeader[i].lpData=tmp+i*blen;
        WaveHeader[i].dwBufferLength=blen;
        WaveHeader[i].dwFlags=0L;
        WaveHeader[i].dwLoops=0L;
        res = waveOutPrepareHeader(WaveHandle,&WaveHeader[i],
                                   sizeof(WAVEHDR));
        CheckWaveError(TEXT("WaveOutPrepareHeader()"),res);
    }

    res = waveOutGetDevCaps((UINT)WaveHandle, &caps, sizeof(caps));
    CheckWaveError(TEXT("WaveOutGetDevCaps()"),res);
    _tprintf(TEXT("Using %s\n"), caps.szPname);
    printf ("--Bits Per Sample = %d\n",WaveFmt.wBitsPerSample);
    printf ("--Channel = %d\n",WaveFmt.nChannels);
    printf ("--Sampling Rate = %d\n",WaveFmt.nSamplesPerSec);
    printf ("--Average Bytes Per Second = %d\n",WaveFmt.nAvgBytesPerSec);
    
}
#endif
