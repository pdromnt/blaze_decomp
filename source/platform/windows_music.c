#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <mmsystem.h>
#include <process.h>
#include "blaze.h"
#include "windows_music.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MUSIC_LOAD,MUSIC_PLAY,MUSIC_STOP };
typedef struct MusicRequest {
    struct MusicRequest *next;
    uint64_t sequence;
    unsigned operation,slot,track;
    int cd,loop;
    char path[1200];
} MusicRequest;
typedef struct {MCIDEVICEID device;int cd,valid;unsigned track;} MusicTrack;
struct BlazeWindowsMusic {
    /* Queue/publication lock is never held during I/O or an MCI call. */
    CRITICAL_SECTION lock;
    HANDLE wake,thread;
    MusicRequest *head,*tail;
    BlazeMusicStatus status;
    uint64_t finished_sequence,intent_sequence;int intent_slot;
    const char *root;
    int closing,muted;
    /* Everything below belongs exclusively to the worker. */
    MusicTrack tracks[2];MCIDEVICEID cd_device;
    int active,playing,loop;
    uint64_t play_sequence,starts,loops;
};
static int command(MCIDEVICEID device,UINT operation,DWORD_PTR flags,DWORD_PTR data) {
    MCIERROR result=mciSendCommandA(device,operation,flags,data);
    if(result) {char error[256];mciGetErrorStringA(result,error,sizeof(error));fprintf(stderr,"Audio MCI %x: %s\n",operation,error);return 0;}return 1;
}
static void stop_device(BlazeWindowsMusic *m) {
    if(m->active>=0 && m->tracks[m->active].device)command(m->tracks[m->active].device,MCI_STOP,MCI_WAIT,0);
    m->playing=0;
}
static void load_device(BlazeWindowsMusic *m,const MusicRequest *r) {
    MusicTrack *t=&m->tracks[r->slot];MCI_OPEN_PARMSA open={0};char alias[64];
    if(m->active==(int)r->slot)stop_device(m);
    if(t->device && !t->cd)command(t->device,MCI_CLOSE,MCI_WAIT,0);
    memset(t,0,sizeof(*t));t->cd=r->cd;t->track=r->track;
    if(r->cd) {
        if(m->muted) {t->valid=1;return;}
        if(!m->cd_device) {
            MCI_SET_PARMS set={0};open.lpstrDeviceType="cdaudio";
            if(!command(0,MCI_OPEN,MCI_OPEN_TYPE|MCI_OPEN_SHAREABLE|MCI_WAIT,(DWORD_PTR)&open))return;
            m->cd_device=open.wDeviceID;set.dwTimeFormat=MCI_FORMAT_TMSF;
            if(!command(m->cd_device,MCI_SET,MCI_SET_TIME_FORMAT|MCI_WAIT,(DWORD_PTR)&set)) {command(m->cd_device,MCI_CLOSE,MCI_WAIT,0);m->cd_device=0;return;}
        }
        t->device=m->cd_device;t->valid=1;return;
    }
    {
        uint8_t *bytes=NULL;size_t size=0;
        if(!r->path[0] || !blaze_file_read(r->path,&bytes,&size) || size<14 || memcmp(bytes,"MThd",4)) {
            fprintf(stderr,"Audio: missing/invalid MIDI %s\n",r->path);free(bytes);return;
        }
        free(bytes);
    }
    if(m->muted) {t->valid=1;return;}
    snprintf(alias,sizeof(alias),"blaze-%p-%u",(void *)m,r->slot);
    open.lpstrDeviceType="sequencer";open.lpstrElementName=r->path;open.lpstrAlias=alias;
    if(command(0,MCI_OPEN,MCI_OPEN_TYPE|MCI_OPEN_ELEMENT|MCI_OPEN_ALIAS|MCI_WAIT,(DWORD_PTR)&open)) {t->device=open.wDeviceID;t->valid=1;}
}
static int start_device(BlazeWindowsMusic *m) {
    MusicTrack *t=&m->tracks[m->active];MCI_PLAY_PARMS play={0};DWORD_PTR flags=MCI_FROM;
    if(m->muted)return t->valid;
    if(!t->device || !t->valid)return 0;
    if(t->cd) {
        MCI_STATUS_PARMS s={0};s.dwItem=MCI_STATUS_NUMBER_OF_TRACKS;
        if(!command(t->device,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT,(DWORD_PTR)&s))return 0;
        if(!t->track || t->track>s.dwReturn) {fprintf(stderr,"Audio: CD track %u unavailable\n",t->track);return 0;}
        play.dwFrom=MCI_MAKE_TMSF(t->track,0,0,0);
        if(t->track<s.dwReturn) {play.dwTo=MCI_MAKE_TMSF(t->track+1,0,0,0);flags|=MCI_TO;}
    }
    return command(t->device,MCI_PLAY,flags,(DWORD_PTR)&play);
}
static void publish(BlazeWindowsMusic *m,uint64_t completed,uint64_t finished) {
    EnterCriticalSection(&m->lock);
    if(completed)m->status.completed=completed;
    if(finished)m->finished_sequence=finished;
    m->status.active=m->active;m->status.playing=m->playing;
    m->status.valid[0]=m->tracks[0].valid;m->status.valid[1]=m->tracks[1].valid;
    m->status.starts=m->starts;m->status.loops=m->loops;
    LeaveCriticalSection(&m->lock);
}
static void poll_device(BlazeWindowsMusic *m) {
    if(m->playing && m->active>=0 && m->tracks[m->active].device) {
        MCI_STATUS_PARMS s={0};s.dwItem=MCI_STATUS_MODE;
        if(mciSendCommandA(m->tracks[m->active].device,MCI_STATUS,MCI_STATUS_ITEM|MCI_WAIT,(DWORD_PTR)&s)==0 &&
           (s.dwReturn==MCI_MODE_STOP || s.dwReturn==MCI_MODE_PAUSE)) {
            /* Game pause leaves audio running. The sequencer's PAUSE here is EOF. */
            if(m->loop) {m->playing=start_device(m);if(m->playing) {++m->starts;++m->loops;}publish(m,0,0);}
            else {m->playing=0;publish(m,0,m->play_sequence);}
        }
    }
}
static unsigned __stdcall music_thread(void *context) {
    BlazeWindowsMusic *m=context;MSG message;
    PeekMessageA(&message,NULL,0,0,PM_NOREMOVE); /* Own any MCI driver windows here. */
    for(;;) {
        MusicRequest *r;int closing;
        EnterCriticalSection(&m->lock);closing=m->closing;r=m->head;
        if(r && !closing) {m->head=r->next;if(!m->head)m->tail=NULL;}
        LeaveCriticalSection(&m->lock);
        if(closing)break;
        if(r) {
            if(r->operation==MUSIC_LOAD)load_device(m,r);
            else if(r->operation==MUSIC_STOP)stop_device(m);
            else {
                stop_device(m);m->active=(int)r->slot;m->loop=r->loop;m->play_sequence=r->sequence;
                m->playing=start_device(m);if(m->playing)++m->starts;
            }
            publish(m,r->sequence,0);free(r);continue;
        }
        poll_device(m);
        while(PeekMessageA(&message,NULL,0,0,PM_REMOVE)) {TranslateMessage(&message);DispatchMessageA(&message);}
        MsgWaitForMultipleObjects(1,&m->wake,FALSE,20,QS_ALLINPUT);
    }
    stop_device(m);
    {unsigned i;for(i=0;i<2;++i)if(m->tracks[i].device && !m->tracks[i].cd)command(m->tracks[i].device,MCI_CLOSE,MCI_WAIT,0);}
    if(m->cd_device)command(m->cd_device,MCI_CLOSE,MCI_WAIT,0);
    return 0;
}
BlazeWindowsMusic *blaze_windows_music_open(const char *root,int muted) {
    BlazeWindowsMusic *m=calloc(1,sizeof(*m));if(!m)return NULL;
    InitializeCriticalSection(&m->lock);m->root=root;m->muted=muted;m->active=-1;m->status.active=-1;m->intent_slot=-1;
    m->wake=CreateEventA(NULL,FALSE,FALSE,NULL);
    if(m->wake)m->thread=(HANDLE)_beginthreadex(NULL,0,music_thread,m,0,NULL);
    if(!m->thread) {if(m->wake)CloseHandle(m->wake);DeleteCriticalSection(&m->lock);free(m);return NULL;}
    return m;
}
static void submit(BlazeWindowsMusic *m,MusicRequest *r) {
    if(!m) {free(r);return;}
    if(!r) {EnterCriticalSection(&m->lock);m->status.failed=1;LeaveCriticalSection(&m->lock);fprintf(stderr,"Audio: cannot allocate music request\n");return;}
    EnterCriticalSection(&m->lock);
    if(m->closing) {LeaveCriticalSection(&m->lock);free(r);return;}
    r->sequence=++m->status.submitted;
    if(r->operation==MUSIC_PLAY) {m->intent_sequence=r->sequence;m->intent_slot=(int)r->slot;}
    else if(r->operation==MUSIC_STOP || (r->operation==MUSIC_LOAD && (int)r->slot==m->intent_slot)) {
        m->intent_sequence=r->sequence;m->intent_slot=-1;
    }
    if(m->tail)m->tail->next=r;else m->head=r;m->tail=r;
    LeaveCriticalSection(&m->lock);SetEvent(m->wake);
}
void blaze_windows_music_load(BlazeWindowsMusic *m,unsigned slot,const char *file,unsigned track,int cd) {
    MusicRequest *r;if(!m || slot>1)return;r=calloc(1,sizeof(*r));
    if(r) {
        char relative[100];r->operation=MUSIC_LOAD;r->slot=slot;r->track=track;r->cd=cd;
        snprintf(relative,sizeof(relative),"midi/%.79s",file);
        if(!cd && !blaze_path(r->path,sizeof(r->path),m->root,relative))fprintf(stderr,"Audio: MIDI path too long\n");
    }
    submit(m,r);
}
void blaze_windows_music_play(BlazeWindowsMusic *m,unsigned slot,int loop) {
    MusicRequest *r;if(slot>1)return;r=calloc(1,sizeof(*r));
    if(r) {r->operation=MUSIC_PLAY;r->slot=slot;r->loop=loop;}submit(m,r);
}
void blaze_windows_music_stop(BlazeWindowsMusic *m) {
    MusicRequest *r=calloc(1,sizeof(*r));if(r)r->operation=MUSIC_STOP;submit(m,r);
}
void blaze_windows_music_status(BlazeWindowsMusic *m,BlazeMusicStatus *s) {
    memset(s,0,sizeof(*s));if(!m)return;
    EnterCriticalSection(&m->lock);*s=m->status;
    /* Ignore an old song's EOF as soon as a newer request has been submitted. */
    s->finished=m->finished_sequence!=0 && m->finished_sequence==m->intent_sequence;
    LeaveCriticalSection(&m->lock);
}
int blaze_windows_music_ended(BlazeWindowsMusic *m) {BlazeMusicStatus s;blaze_windows_music_status(m,&s);return s.finished;}
void blaze_windows_music_close(BlazeWindowsMusic *m) {
    MusicRequest *r;if(!m)return;
    EnterCriticalSection(&m->lock);m->closing=1;LeaveCriticalSection(&m->lock);SetEvent(m->wake);
    WaitForSingleObject(m->thread,INFINITE);CloseHandle(m->thread);CloseHandle(m->wake);
    while((r=m->head)!=NULL) {m->head=r->next;free(r);}
    DeleteCriticalSection(&m->lock);free(m);
}
