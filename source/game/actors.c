#include "blaze.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Original numeric record owner, with native actor handles in the old source
   slot. Native enemies and effects share this registration/lifetime. */
static int supported(unsigned stage,unsigned type) {
    if(stage==3)return type==38 || type==3 || type==17 || type==1 || type==2 || type==4 || type==5 || type==6 || type==7 || type==12 || type==13 || type==14 || type==18 || type==20 || type==21 || type==22 || type==23 || type==32 || type==33 || type==34 || type==37 || type==39 || type==40 || type==16 || type==11 || type==26 || type==27 || type==41 || type==45 || type==46 || type==47;
    if(stage==4)return type==3 || type==4 || type==6 || type==7 || type==13 || type==14 || type==17 || type==20 || type==23 || type==25 || type==28 || type==29 || type==32 || type==33 || type==34 || type==42 || type==43 || type==44 || type==38 || type==45 || type==47 || type==11 || type==26 || type==27 || type==41 || type==46;
    if(stage==2)return type==4 || type==5 || type==7 || type==9 || type==13 || type==14 || type==17 || type==18 || type==29 || type==30 || type==34 || type==45 || type==47 || type==20 || type==0 || type==12 || type==21 || type==23 || type==3 || type==8 || type==16 || type==37 || type==22 || type==24 || type==36 || type==33 || type==10 || type==11 || type==26 || type==27 || type==41 || type==46;
    if(stage==1)return type==4 || type==7 || type==9 || type==13 || type==14 || type==38 || type==45 || type==47 || type==2 || type==16 || type==20 || type==33 || type==37 || type==31 || type==10 || type==35 || type==12 || type==21 || type==19 || type==6 || type==3 ||
        type==11 || type==26 || type==27 || type==41 || type==46 || type==44 || type==43;
    if(stage!=0)return 0;
    return type==4 || type==5 || type==6 || type==7 || type==9 || type==13 || type==14 || type==15 || type==17 || type==28 || type==38 || type==45 || type==47 || type==12 || type==21 || type==2 || type==8 || type==3 || type==16 || type==37 || type==0 || type==20 || type==1 || type==43 || type==22 || type==11 || type==26 || type==27 || type==41 || type==46 || type==44;}
int blaze_actor_create(BlazeWorld *w,uint32_t *s,const uint32_t rect[4],
    unsigned type,unsigned parameter,unsigned source,unsigned flags) {
    const BlazeSprite *base,*sprite;unsigned id=0x45,mask=0,parent=0,i;int health=32;
    if(s[4])return 0;
    if(!supported(w->level->stage,type))return -1;
    if(type==12 || type==4) {
        for(i=0;i<256;++i)if(s==w->actors[i]) {parent=i+1;break;}
        if(!parent)return -1;
        id=type==12?0x153:0x151;mask=type==12?0x900:0xb04;health=type==12?(w->level->stage>=2?640:w->level->stage==1?400:256):w->level->stage==3?4000:w->level->stage==4?5000:3000;
    }
    if(type==32 || type==18 || type==29 || type==30 || type==10 || type==5 || type==7 || type==9 || type==14 || type==15 || type==28 || type==38 || type==45)
        if(!source || source>256)return -1;
    if(type==5) {id=0x99;mask=0x800;health=256;}
    if(type==6) {id=0x86;mask=0x90;health=9;}
    if(type==7) {id=w->level->stage==4?0x29c:w->level->stage==3?0x2ae:w->level->stage==2?0x2cc:0x2b4;mask=0x442;}
    if(type==9) {id=0x143;mask=0x402;}
    if(type==13) {id=0x167;mask=0x502;}
    if(type==14) {id=0x81;mask=0x442;}
    if(type==15) {id=0x4f;mask=0x402;}
    if(type==17)id=0x13e;
    if(type==28)mask=0x482;
    if(type==38) {id=0x81;health=200;}
    if(type==45) {id=0xd8;mask=0x402;}
    if(type==47) {id=0x137;mask=0x402;}
    if(type==21) {if(!source || source>256)return -1;id=w->level->stage==2?0x2ca:w->level->stage==1 || w->level->stage==3?0x29c:0x2ac;mask=0x442;health=1000;}
    if(type==2) {id=w->level->stage==3?0x2a8:w->level->stage==1?0x2b0:0x2a0;mask=w->level->stage==1 || w->level->stage==3?0x100:0x900;health=16;}
    if(type==8) {id=w->level->stage==2?0x2ac:0x2ae;mask=0x900;health=256;}
    if(type==31) {id=0x2ae;mask=0x900;health=64;}
    if(type==10) {id=0x9b;mask=0x82;}
    if(type==25) {id=0x86;mask=0x92;health=132;}
    if(type==32) {id=0x2a5;mask=0x402;}
    if(type==39) {id=0xd6;mask=0x82;health=9;}
    if(type==40) {id=0x2a6;mask=0x900;health=2;}
    if(type==42) {id=0x9b;mask=0x182;}
    if(type==24) {id=0x29c;mask=0x900;health=100;}
    if(type==36) {id=0xbc;mask=0x182;}
    if(type==18) {id=0xa1;mask=0x92;health=128;}
    if(type==29) {id=0x162;mask=0x82;health=30;}
    if(type==30) {id=0x89;health=48;}
    if(type==34) {id=0x70;mask=0x92;health=8;}
    if(type==33) {id=0x6d;mask=0x92;health=3;}
    if(type==35) {id=0xd7;mask=0x92;health=64;}
    if(type==19) {id=0x29e;mask=0x900;health=128;}
    if(type==3) {id=w->level->stage==1?0x6b:0x69;mask=0x92;health=3;}
    if(type==16) {id=blaze_signed(parameter)<0?0x165:0x163;mask=0x80;}
    if(type==37) {id=0x79;mask=0x82;}
    if(type==0) {id=w->level->stage==2?0x2c2:0x2a4;mask=0x900;}
    if(type==20) {id=0x5b;mask=0x90;health=2;}
    if(type==1) {id=w->level->stage==3?0x2ac:0x29e;mask=0x900;health=w->level->stage==3?48:24;}
    if(type==44) {id=0xa4;mask=0x190;health=30;}
    if(type==43) {id=0x77;mask=0x92;health=30;}
    if(type==22) {id=w->level->stage==3?0x29e:w->level->stage==2?0x2b2:0x29c;mask=0x900;health=4;}
    if(type==11) {id=0x156;mask=0x80;health=24;}
    if(type==26) {id=0x81;mask=0x402;}
    if(type==27 || type==41) {id=0x137;mask=0x442;}
    if(type==46)mask=0x402;
    base=blaze_level_sprite(w->level,0x45);sprite=blaze_level_sprite(w->level,id);
    if(!base || !sprite)return -1;
    s[0]=id;s[3]=type==46?16:type==37?48:640;s[4]=1;s[11]=parameter;s[12]=(uint32_t)(type==20 || type==3?-6:type==25?-10:type==39?-6:type==16 || type==10 || type==36 || type==42?-7:-2);
    s[5]=rect[0]+(uint32_t)(blaze_signed(rect[2]-rect[0])/2-base->width/2+
        ((int)base->width-sprite->width)/2);
    s[6]=rect[1]+(uint32_t)(blaze_signed(rect[3]-rect[1])/2-base->visible_height/2+
        ((int)base->visible_height-sprite->visible_height)/2);
    s[7]=s[5]+sprite->width;s[8]=s[6]+sprite->visible_height;
    s[0x34/4]=source;s[0x38/4]=rect[0];s[0x3c/4]=rect[1];s[0x40/4]=0;s[0x44/4]=mask;
    s[0x4c/4]=type;s[0x54/4]=s[0x58/4]=s[0x64/4]=0;s[0x5c/4]=UINT32_MAX;
    s[0x60/4]=(uint32_t)(health+100);s[0x68/4]=flags;
    if(w->level->difficulty==0)health=health*3/4;
    if(w->level->difficulty==2)health=health*5/4;
    s[0x50/4]=(uint32_t)health;
    if(type==32 || type==4 || type==5 || type==7 || type==9)s[3]=50000;
    if(type==15)s[3]=32000;
    if(type==28)s[3]=32;
    if(type==38) {s[3]=6000;s[12]=parameter;}
    if(type==45)s[3]=96;
    if(type==33 || type==34)s[3]=500;
    if(type==18 || type==30)s[3]=96;
    if(type==29) {s[9]=parameter;s[10]=0;}
    if(type==30) {
        const uint32_t *p=w->actors[source-1];
        s[5]=p[5]+rect[0]+(uint32_t)(blaze_signed(p[7]-p[5])/2-sprite->width/2);
        s[6]=p[6]+rect[1]+(uint32_t)(blaze_signed(p[8]-p[6])/2-sprite->visible_height/2);
        s[7]=s[5]+sprite->width;s[8]=s[6]+sprite->visible_height;
        s[9]=(uint32_t)(w->trig[parameter&255]/30);s[10]=(uint32_t)(-w->trig[(parameter&255)+64]/30);
    }
    if(type==32 || (type==7 && w->level->stage==4))s[11]=0;
    if(type==42) {const uint32_t *p=w->player.words;const BlazeSprite *ps=blaze_level_sprite(w->level,p[0]);s[3]=32;s[0x48/4]=blaze_random_next(w->random)*3200/32767;s[11]=blaze_aim(blaze_signed(p[5]+ps->width/2),blaze_signed(p[6]+ps->visible_height/2),blaze_signed(s[5]+sprite->width/2),blaze_signed(s[6]+sprite->visible_height/2));}
    if(type==36) {s[3]=48;s[9]=(uint32_t)-32;s[10]=0;}
    if(type==7 && w->level->stage==1) {s[12]=32;s[11]=0;}
    if(type==15 || type==17)s[12]=(uint32_t)-7;
    if(type==10) {s[3]=32;s[0x48/4]=blaze_random_next(w->random)*3200/32767;}
    if(type==35) {s[3]=blaze_random_next(w->random)*32/32767+16;s[9]=0;s[10]=3;}
    if(type==31) {unsigned direction=parameter&255;s[9]=(uint32_t)(w->trig[direction]/30);s[10]=(uint32_t)(-w->trig[direction+64]/30);}
    if(type==44) {s[9]=(uint32_t)-2;s[10]=0;}
    if(type==13) {s[9]=(uint32_t)-4;s[10]=0;}
    if(type==8) {s[11]=2;s[0x48/4]=blaze_random_next(w->random)*2/32767;}
    if(type==37) {s[9]=(uint32_t)-16;s[10]=0;}
    if(type==3) {
        int divisor=(10-(int)w->level->difficulty)*3;unsigned direction=parameter&255;
        s[9]=(uint32_t)(w->trig[direction]/divisor);s[10]=(uint32_t)(-w->trig[direction+64]/divisor);
    }
    if(type==0) {s[9]=(uint32_t)-4;s[10]=0;}
    if(type==20) {
        int divisor=(16-(int)w->level->difficulty)*2;unsigned direction=parameter&255;
        s[9]=(uint32_t)(w->trig[direction]/divisor);s[10]=(uint32_t)(-w->trig[direction+64]/divisor);
    }
    if(type==2) {
        s[11]=blaze_random_next(w->random)*64/32767+128;
        s[9]=(uint32_t)(w->trig[s[11]&255]/12);s[10]=(uint32_t)(-w->trig[(s[11]&255)+64]/12);
    }
    if(type==1) {
        s[0x48/4]=(uint32_t)((int)(blaze_random_next(w->random)*16/32767)-8);
        s[9]=(uint32_t)(w->trig[128]/12);s[10]=(uint32_t)(-w->trig[192]/12);
        s[11]=blaze_random_next(w->random)*256/32767;
    }
    if(type==12) {
        /* Parent is already active here. Nested registration observes the same
           cursor before the outer registration consumes its own attempt. */
        if(!blaze_actor_attach(w,parent,w->level->stage==3?-120:w->level->stage==1 || w->level->stage==2?-134:-48,w->level->stage>=1?-32:-16,21,parameter,0)) {s[4]=0;return -1;}
        s[9]=(uint32_t)-5;s[10]=0;
    }
    if(type==4) {
        static const struct {int x,y;unsigned type,parameter;} children[]={
            {-16,-57,7,192},{64,32,5,192},{132,80,9,224},{0,64,15,192},
            {0,48,38,0},{8,-48,38,0},{0,96,38,0},{8,144,38,0},{0,0,45,0},{132,80,45,1}};
        if(w->level->stage==0) {
            for(i=0;i<sizeof(children)/sizeof(children[0]);++i)
                if(!blaze_actor_attach(w,parent,children[i].x,children[i].y,children[i].type,
                    children[i].type==5 || children[i].type==7 || children[i].type==15?parameter:children[i].parameter,0)) {s[4]=0;return -1;}
        } else if(w->level->stage==1) {
            static const struct {int x,y;unsigned type,parameter;} feria[]={
                {-132,-60,7,0},{-160,-16,9,64},{64,0,38,1},{64,0,45,0},{0,0,45,0},{-160,0,45,1}};
            s[9]=(uint32_t)-16;s[10]=0;
            for(i=0;i<sizeof(feria)/sizeof(feria[0]);++i)
                if(!blaze_actor_attach(w,parent,feria[i].x,feria[i].y,feria[i].type,
                    feria[i].type==7?parameter:feria[i].parameter,0)) {s[4]=0;return -1;}
            /* The original enlarges collision bounds after child construction. */
            s[5]-=96;s[6]-=16;s[7]+=96;s[8]+=16;
        } else if(w->level->stage==2) {
            static const struct {int x,y;unsigned type,parameter;} sesiru[]={
                {-320,-54,7,0},{352,0,5,0},{-32,64,5,0},{-64,64,5,0},{256,0,5,0},{280,80,9,192}};
            s[9]=s[10]=UINT32_MAX;s[0x44/4]=0xf06;
            for(i=0;i<sizeof(sesiru)/sizeof(sesiru[0]);++i)
                if(!blaze_actor_attach(w,parent,sesiru[i].x,sesiru[i].y,sesiru[i].type,
                    sesiru[i].type==9?sesiru[i].parameter:parameter,0)) {s[4]=0;return -1;}
        } else if(w->level->stage==3) {
            static const struct {int x,y;unsigned type,parameter;} isabel[]={
                {-96,-16,7,0},{-32,-32,5,0},{64,-32,5,0},{-16,64,38,16},{0,128,38,16},{0,0,45,0},{-16,64,45,1},{0,128,45,1}};
            for(i=0;i<sizeof(isabel)/sizeof(isabel[0]);++i)if(!blaze_actor_attach(w,parent,isabel[i].x,isabel[i].y,isabel[i].type,isabel[i].type==7 || isabel[i].type==5?parameter:isabel[i].parameter,0)) {s[4]=0;return -1;}
        } else {
            static const struct {int x,y;unsigned type,parameter;} raiya[]={
                {-200,-88,7,0},{-200,-88,32,0},{8,-64,38,16},{8,64,38,16},{0,0,45,0}};
            s[9]=(uint32_t)-16;s[10]=0;
            for(i=0;i<sizeof(raiya)/sizeof(raiya[0]);++i)if(!blaze_actor_attach(w,parent,raiya[i].x,raiya[i].y,raiya[i].type,raiya[i].type==7 || raiya[i].type==32?parameter:raiya[i].parameter,0)) {s[4]=0;return -1;}
        }
    }
    if(mask&0x800)++w->enemies_created;
    return 1;
}
static int register_actor(BlazeWorld *w,const uint32_t rect[4],unsigned type,unsigned parameter,unsigned source,unsigned flags) {
    unsigned i;
    if(!supported(w->level->stage,type))return 0;
    for(i=0;i<256;++i) {
        unsigned candidate=(w->actor_cursor+i)%256;
        int result=blaze_actor_create(w,w->actors[candidate],rect,type,parameter,source?source:i+1,flags);
        if(result<0)return 0;
        if(result)break;
    }
    ++w->actor_cursor;return 1; /* The original also consumes full-pool attempts. */
}
int blaze_actor_spawn(BlazeWorld *w,const uint32_t rect[4],unsigned type,unsigned parameter,unsigned flags) {
    return register_actor(w,rect,type,parameter,0,flags);
}
int blaze_actor_attach(BlazeWorld *w,unsigned parent,int x,int y,unsigned type,unsigned parameter,unsigned flags) {
    uint32_t rect[4]={(uint32_t)x,(uint32_t)y,0,0};
    if(!parent || parent>256 || (type!=32 && type!=18 && type!=29 && type!=30 && type!=10 && type!=37 && type!=21 && type!=5 && type!=7 && type!=9 && type!=14 && type!=15 && type!=28 && type!=38 && type!=45))return 0;
    return register_actor(w,rect,type,parameter,parent,flags);
}
int blaze_actor_register(BlazeWorld *w,const BlazeSpawn *spawn) {
    uint32_t rect[4]={spawn->x,spawn->y,spawn->x+32,spawn->y+32};
    return blaze_actor_spawn(w,rect,spawn->parameter,192,spawn->kind);
}
static int blocked(char *error,size_t capacity,const char *route) {
    snprintf(error,capacity,"Actor %s is not reconstructed",route);return 0;
}
static void move(uint32_t *s) {s[5]+=s[9];s[7]+=s[9];s[6]+=s[10];s[8]+=s[10];}
static int draw(BlazeWorld *w,const uint32_t *s,unsigned frame,int scale,int palette,int blend) {
    const BlazeSprite *sprite=blaze_level_sprite(w->level,s[0]);BlazeActorDraw *d;
    if(w->actor_draw_count==BLAZE_ACTOR_DRAWS)return 0;
    d=&w->actor_draws[w->actor_draw_count++];memset(d,0,sizeof(*d));d->sprite=s[0]+frame;d->scale=scale;d->palette=palette;d->blend=blend;
    d->x=blaze_signed(s[5])-w->camera_x+((int)sprite->width-(int)((int64_t)sprite->width*scale/256))/2;
    d->y=blaze_signed(s[6])-w->camera_y+((int)sprite->visible_height-(int)((int64_t)sprite->visible_height*scale/256))/2;
    return 1;
}
static int label(BlazeWorld *w,unsigned sprite,int value,int x,int y) {
    BlazeActorDraw *d;if(w->actor_draw_count==BLAZE_ACTOR_DRAWS)return 0;
    d=&w->actor_draws[w->actor_draw_count++];memset(d,0,sizeof(*d));d->sprite=sprite;d->palette=value;
    d->scale=256;d->blend=0;d->x=x;d->y=y;return 1;
}
/* 0x419740: endpoints are measured from the first position toward the second.
   Both inequalities may hold: in the dead band the two adjustments cancel.
   Integer endpoint arithmetic wraps before conversion, as in the original. */
unsigned blaze_turn(const int32_t trig[320],uint32_t angle,int from_x,int from_y,
    int target_x,int target_y,unsigned step) {
    unsigned plus=(angle+step)&255,minus=(angle-step)&255;
    double px=blaze_signed((uint32_t)from_x+(uint32_t)trig[plus]-(uint32_t)target_x);
    double py=blaze_signed((uint32_t)from_y-(uint32_t)trig[plus+64]-(uint32_t)target_y);
    double mx=blaze_signed((uint32_t)from_x+(uint32_t)trig[minus]-(uint32_t)target_x);
    double my=blaze_signed((uint32_t)from_y-(uint32_t)trig[minus+64]-(uint32_t)target_y);
    double a=sqrt(px*px+py*py),b=sqrt(mx*mx+my*my);
    if(a>b-step)angle+=step;
    if(a<b+step)angle-=step;
    /* Signed remainder also preserves negative legacy angles at this boundary. */
    return (unsigned)(blaze_signed(angle+256)%256);
}
static void steer(BlazeWorld *w,uint32_t *s,unsigned step) {
    const uint32_t *p=w->player.words;
    const BlazeSprite *ps=blaze_level_sprite(w->level,p[0]),*as=blaze_level_sprite(w->level,s[0]);
    s[11]=blaze_turn(w->trig,s[11],blaze_signed(p[5]+ps->width/2),blaze_signed(p[6]+ps->visible_height/2),
        blaze_signed(s[5]+as->width/2),blaze_signed(s[6]+as->visible_height/2),step);
}
static int rotated(BlazeWorld *w,const uint32_t *s,unsigned frame,unsigned angle) {
    BlazeActorDraw *d;
    if(blaze_signed(s[5])>=w->camera_x+320 || blaze_signed(s[7])<=w->camera_x ||
       blaze_signed(s[6])>=w->camera_y+240 || blaze_signed(s[8])<=w->camera_y)return 1;
    if(w->actor_draw_count==BLAZE_ACTOR_DRAWS)return 0;
    d=&w->actor_draws[w->actor_draw_count++];memset(d,0,sizeof(*d));
    d->sprite=s[0]+frame;d->angle=angle&255;d->scale=256;d->palette=-1;d->rotated=1;
    d->pivot_x=blaze_signed(s[7]-s[5])/2;d->pivot_y=blaze_signed(s[8]-s[6])/2;
    d->x=blaze_signed(s[5])-w->camera_x+d->pivot_x;
    d->y=blaze_signed(s[6])-w->camera_y-4+d->pivot_y;return 1;
}
static int vertical_target(BlazeWorld *w,const uint32_t *s) {
    const uint32_t *p=w->player.words;
    const BlazeSprite *ps=blaze_level_sprite(w->level,p[0]),*as=blaze_level_sprite(w->level,s[0]);
    int player=blaze_signed(p[6]+ps->visible_height/2),actor=blaze_signed(s[6]+as->visible_height/2);
    return (player>actor)-(player<actor);
}
static void bank(BlazeWorld *w,uint32_t *s) {
    int frame=blaze_signed(s[11]),vy=blaze_signed(s[10]);
    if(!(w->tick&3))return;
    if(!vy)frame+=(frame<2)-(frame>2);
    if(vy<0)frame+=(frame<0)-(frame>0);
    if(vy>0)frame+=(frame<5)-(frame>5);
    s[11]=(uint32_t)frame;
}
static int offset_spawn(BlazeWorld *w,const uint32_t *s,int x,int y,unsigned type,unsigned parameter) {
    uint32_t rect[4]={s[5]+(uint32_t)x,s[6]+(uint32_t)y,s[7]+(uint32_t)x,s[8]+(uint32_t)y};
    return blaze_actor_spawn(w,rect,type,parameter,0);
}
static int parent_effect(BlazeWorld *w,const uint32_t *s) {
    unsigned i;for(i=0;i<256;++i)if(s==w->actors[i])return blaze_actor_attach(w,i+1,-32,0,37,0,0);
    return 0;
}
static int hit_sparks(BlazeWorld *w,const uint32_t *s) {
    unsigned i;const BlazeSprite *sprite=blaze_level_sprite(w->level,s[0]);
    if(s[0x5c/4]==UINT32_MAX)return 1;
    for(i=0;i<3;++i) {
        BlazeActorDraw *d;unsigned scale=blaze_random_next(w->random)*192/32767+80;
        unsigned id=blaze_random_next(w->random)*4/32767+129;
        int y=blaze_signed(s[6]+sprite->visible_height/2)-w->camera_y+(int)(blaze_random_next(w->random)*64/32767)-32;
        int x=blaze_signed(s[5]+sprite->width/2)-w->camera_x+(int)(blaze_random_next(w->random)*64/32767)-32;
        if(w->actor_draw_count==BLAZE_ACTOR_DRAWS)return 0;
        d=&w->actor_draws[w->actor_draw_count++];memset(d,0,sizeof(*d));
        d->sprite=id;d->scale=(int32_t)scale;d->palette=-1;d->x=x;d->y=y;
    }
    return 1;
}
static void place(BlazeWorld *w,uint32_t *s,uint32_t x,uint32_t y) {
    const BlazeSprite *p=blaze_level_sprite(w->level,s[0]);
    s[5]=x;s[6]=y;s[7]=x+p->width;s[8]=y+p->visible_height;
}
static void resize_actor(BlazeWorld *w,uint32_t *s,unsigned id,int expansion) {
    const BlazeSprite *a=blaze_level_sprite(w->level,s[0]),*b=blaze_level_sprite(w->level,id);
    uint32_t x=s[5]+(uint32_t)(((int)a->width-b->width)/2),y=s[6]+(uint32_t)(((int)a->visible_height-b->visible_height)/2);
    s[0]=id;s[12]=(uint32_t)expansion;place(w,s,x,y);
}
static uint32_t *follow(BlazeWorld *w,uint32_t *s,int offset) {
    unsigned h=s[0x34/4],i;uint32_t *p;
    if(!h || h>256)return NULL;
    p=w->actors[h-1];for(i=0;i<4;++i)s[5+i]=p[5+i]+s[(i&1)?0x3c/4:0x38/4]+(uint32_t)offset;
    return p;
}
static void phase(BlazeWorld *w,uint32_t *s,unsigned count,unsigned first) {
    unsigned next;
    if(w->loadout->mode==4) {next=s[0x58/4]+1;if(next<first || next>=first+count)next=first;}
    else do {next=blaze_random_next(w->random)*count/32767+first;}while(next==s[0x58/4]);
    s[0x58/4]=next;s[0x54/4]=0;
}
static int caption(BlazeWorld *w,unsigned id,int x,int y,unsigned font,unsigned alignment,unsigned count) {
    BlazeActorDraw *d;if(w->actor_draw_count==BLAZE_ACTOR_DRAWS)return 0;
    d=&w->actor_draws[w->actor_draw_count++];memset(d,0,sizeof(*d));
    d->caption=id;d->sprite=font;d->x=x;d->y=y;d->scale=256;d->palette=-1;d->alignment=alignment;d->characters=count;return 1;
}
static int fill(BlazeWorld *w,int left,int top,int right,int bottom,int color) {
    BlazeActorDraw *d;if(w->actor_draw_count==BLAZE_ACTOR_DRAWS)return 0;
    d=&w->actor_draws[w->actor_draw_count++];memset(d,0,sizeof(*d));d->sprite=UINT32_MAX-2;
    d->x=left;d->y=top;d->pivot_x=right-left;d->pivot_y=bottom-top;d->palette=color;return 1;
}
/* Original clamp uses sprite dimensions even when the boss collision rectangle
   was enlarged by its constructor. Keep those two notions distinct. */
static int clamp_actor(BlazeWorld *w,uint32_t *s) {
    const BlazeSprite *a=blaze_level_sprite(w->level,s[0]);int changed=0;
    if(blaze_signed(s[5])<w->camera_x) {changed=1;s[5]=(uint32_t)w->camera_x;s[7]=s[5]+a->width;}
    if(blaze_signed(s[7])>=w->camera_x+320) {changed=1;s[7]=(uint32_t)(w->camera_x+320);s[5]=s[7]-a->width;}
    if(blaze_signed(s[6])<0) {changed=1;s[6]=0;s[8]=a->visible_height;}
    if(blaze_signed(s[8])>=320) {changed=1;s[8]=320;s[6]=320-a->visible_height;}
    return changed;
}
static int horizontal_target(BlazeWorld *w,const uint32_t *s) {
    const uint32_t *p=w->player.words;
    const BlazeSprite *a=blaze_level_sprite(w->level,s[0]),*b=blaze_level_sprite(w->level,p[0]);
    return blaze_signed(p[5]+b->width/2-s[5]-a->width/2);
}
static int vertical_delta(BlazeWorld *w,const uint32_t *s,int bias) {
    const uint32_t *p=w->player.words;
    const BlazeSprite *a=blaze_level_sprite(w->level,s[0]),*b=blaze_level_sprite(w->level,p[0]);
    int delta=blaze_signed(p[6]+b->visible_height/2-s[6]-a->visible_height/2+(uint32_t)bias);
    return (delta>0)-(delta<0);
}
static int feria(BlazeWorld *w,uint32_t *s,int age) {
    unsigned state=s[0x58/4],i;int part=age%128;
    switch(state) {
    case 0:if(age>12)phase(w,s,4,1);move(s);break;
    case 1:
        for(i=0;i<2;++i) {int dx=horizontal_target(w,s);s[9]=(uint32_t)((dx>0)-(dx<0));s[10]=(uint32_t)vertical_delta(w,s,64);move(s);}
        clamp_actor(w,s);
        if(age%64<48) {int triangle=age%32;triangle=triangle<16?triangle:31-triangle;
            if(!offset_spawn(w,s,-64,0,3,(unsigned)(triangle*20-64)))return 0;}
        break;
    case 2:
        s[9]=(uint32_t)-16;s[10]=(uint32_t)(vertical_delta(w,s,-64)*2);move(s);
        if(part==1)s[11]=blaze_random_next(w->random)*256/32767;
        if(part<64) {unsigned direction=s[11]&255;s[9]=(uint32_t)(w->trig[direction]/32);s[10]=(uint32_t)(-w->trig[direction+64]/32);move(s);}
        else if(age%8==0 && !offset_spawn(w,s,-64,-32,6,64))return 0;
        clamp_actor(w,s);break;
    case 3:
        if(age%32<16) {if(age%4==0 && !offset_spawn(w,s,16,32,16,1))return 0;}
        else for(i=0;i<3;++i) {s[9]=(uint32_t)-4;s[10]=(uint32_t)(vertical_delta(w,s,-32)*2);move(s);clamp_actor(w,s);}
        break;
    case 4:
        s[9]=(uint32_t)(part<64?-2:2);s[10]=(uint32_t)(part<64?-5:5);move(s);clamp_actor(w,s);
        if(age%8==4 && part<96 && !offset_spawn(w,s,-16,0,33,blaze_random_next(w->random)*16/32767-8+s[11]))return 0;
        break;
    case 10:
        s[9]=(uint32_t)(part<64?-4:4);s[10]=(uint32_t)-2;move(s);clamp_actor(w,s);
        if(age%8==4 && part>96 && !offset_spawn(w,s,-64,0,35,0))return 0;
        break;
    case 11:
        for(i=0;i<2;++i) {s[9]=(uint32_t)(part<64?1:-1);s[10]=(uint32_t)vertical_delta(w,s,part<64?64:-64);move(s);clamp_actor(w,s);}
        if(part>32 && !offset_spawn(w,s,-64,0,20,blaze_random_next(w->random)*128/32767-64+s[11]))return 0;
        break;
    case 12:
        if(part<64)for(i=0;i<6;++i) {s[9]=0;s[10]=(uint32_t)vertical_delta(w,s,0);move(s);clamp_actor(w,s);}
        if(age%32==0 && !offset_spawn(w,s,-64,0,2,blaze_random_next(w->random)*256/32767))return 0;
        break;
    case 100:s[0x54/4]=0;s[0x58/4]=101;break;
    case 101:s[5]+=8;s[7]+=8;s[6]+=(uint32_t)((age&1)?-2:6);s[8]+=(uint32_t)((age&1)?-2:6);if(age>192)s[4]=0;break;
    }
    if(part==127 && state>=1 && state<=12) {
        if(state>=10 || blaze_signed(s[0x50/4])<800)phase(w,s,3,10);
        else phase(w,s,4,1);
    }
    steer(w,s,1);return 1;
}
static int sesiru(BlazeWorld *w,uint32_t *s,int age) {
    unsigned state=s[0x58/4],i,h=0;int part=age%192;
    for(i=0;i<256;++i)if(s==w->actors[i]) {h=i+1;break;}
    switch(state) {
    case 0:
        if(age<160) {s[9]=s[10]=UINT32_MAX;move(s);}
        if(age>=160 && age<800) {s[9]=UINT32_MAX;s[10]=0;move(s);}
        if(age>959 && age<1328) {s[9]=2;s[10]=0;move(s);}
        if(age>1329) {
            s[0x60/4]=s[0x50/4]+100;s[0x44/4]=0xb04;
            if(!blaze_actor_attach(w,h,0,0,45,0,0) || !blaze_actor_attach(w,h,-132,82,45,1,0))return 0;
            phase(w,s,7,1);
        }
        steer(w,s,1);return 1;
    case 1:
        if(age<32) {s[9]=(uint32_t)-16;s[10]=0;move(s);}
        if(age>160) {s[9]=16;s[10]=0;move(s);}
        if(part==32 || part==48 || part==64) {
            unsigned angle=part==32?208:part==48?224:240;
            if(!blaze_actor_attach(w,h,512,0,30,angle,0) || !blaze_actor_attach(w,h,512,0,30,384-angle,0))return 0;
        }
        break;
    case 2:
        if(age%64<20 && !offset_spawn(w,s,-32,-32,34,blaze_random_next(w->random)*96/32767-48+s[11]))return 0;
        break;
    case 3:
        if(part==16 && !blaze_actor_attach(w,h,0,0,29,(unsigned)-16,0))return 0;
        break;
    case 4:
        if(part%20<5) {
            int offset=(int)(blaze_random_next(w->random)*240/32767)-120;
            if(!offset_spawn(w,s,0,offset,16,UINT32_MAX))return 0;
        }
        if(part==191) {if(!blaze_actor_spawn(w,s+5,17,s[11],0))return 0;phase(w,s,7,1);}
        steer(w,s,1);return 1;
    case 5:
        if(age%24==0 && part>32 && !offset_spawn(w,s,-132,82,23,192))return 0;
        break;
    case 6:
        s[9]=0;s[10]=(uint32_t)(vertical_delta(w,s,0)*2);move(s);
        if((part==32 || part==96) && !blaze_actor_attach(w,h,-32,0,18,192,0))return 0;
        break;
    case 7:
        if(age>16 && age<96 && (age&1)) {
            unsigned y=blaze_random_next(w->random)*640/32767;
            uint32_t rect[4]={(uint32_t)w->camera_x+320,y,(uint32_t)w->camera_x+352,y+32};
            if(!blaze_actor_spawn(w,rect,36,192,0))return 0;
        }
        if(age<32) {s[9]=16;s[10]=0;move(s);}
        if(age>160) {s[9]=(uint32_t)-16;s[10]=0;move(s);}
        break;
    case 100:s[0x54/4]=0;s[0x58/4]=101;steer(w,s,1);return 1;
    case 101:s[5]-=8;s[7]-=8;s[6]+=(uint32_t)((age&1)?4:-4);s[8]+=(uint32_t)((age&1)?4:-4);if(age>192)s[4]=0;steer(w,s,1);return 1;
    default:steer(w,s,1);return 1; /* Inclusive RNG can select unhandled phase 8. */
    }
    if(part==191)phase(w,s,7,1);
    steer(w,s,1);return 1;
}
static int isabel(BlazeWorld *w,uint32_t *s,int age) {
    unsigned state=s[0x58/4],i,h=0;int part=age%64;
    for(i=0;i<256;++i)if(s==w->actors[i]) {h=i+1;break;}
    switch(state) {
    case 0:if(age>12)phase(w,s,4,1);s[9]=(uint32_t)-8;s[10]=0;move(s);break;
    case 1:
        s[0x40/4]=age%8<4?2:3;
        if(age%32>8 && age%32<16) {for(i=0;i<2;++i) {s[9]=(uint32_t)-8;s[10]=(uint32_t)vertical_delta(w,s,0);move(s);}s[0x40/4]=1;}
        if(age%32>24) {for(i=0;i<2;++i) {s[9]=8;s[10]=(uint32_t)vertical_delta(w,s,0);move(s);}s[0x40/4]=2;}
        break;
    case 2:
        if(part<14)for(i=0;i<20;++i) {s[9]=0;s[10]=(uint32_t)vertical_delta(w,s,0);move(s);}
        if(part>32 && !blaze_actor_spawn(w,s+5,3,blaze_random_next(w->random)*96/32767-48+s[11],0))return 0;
        break;
    case 3:
        s[0x40/4]=0;
        if(age%16==0) {static const int angles[]={32,64,-32,-64};for(i=0;i<4;++i)if(!offset_spawn(w,s,32,64,6,s[11]+(unsigned)angles[i]))return 0;}
        s[9]=(uint32_t)(part<32?3:-3);s[10]=0;move(s);break;
    case 4:
        s[0x40/4]=3;
        if(part>8 && part<24 && (age&1) && !blaze_actor_attach(w,h,32,(part-16)*16,37,0,0))return 0;
        break;
    case 10:
        s[0x40/4]=3;
        if(age%3==0 && !offset_spawn(w,s,64,64,39,(unsigned)(-(int)(blaze_random_next(w->random)*10/32767))))return 0;
        if(part>32)for(i=0;i<4;++i) {s[9]=0;s[10]=(uint32_t)vertical_delta(w,s,-32);move(s);}
        break;
    case 11:
        if(part<33) {int d=blaze_signed(480-s[6]);s[9]=0;s[10]=(uint32_t)((d>0)-(d<0));move(s);}
        else {if(age%3==0 && !offset_spawn(w,s,-32,96,34,blaze_random_next(w->random)*128/32767-64))return 0;
            for(i=0;i<20;++i) {s[9]=0;s[10]=(uint32_t)vertical_delta(w,s,-96);move(s);}}
        s[0x40/4]=age%16<8?2:3;break;
    case 12:
        s[0x40/4]=0;
        if(part>32) {int d=blaze_signed(0-s[6]);if(age%3==0 && !blaze_actor_spawn(w,s+5,20,s[11],0))return 0;s[9]=0;s[10]=(uint32_t)((d>0)-(d<0));move(s);}break;
    case 13:
        if(age%16==0 && !offset_spawn(w,s,64,32,23,0))return 0;
        {int d=blaze_signed((part<33?480u:0u)-s[6]);s[9]=0;s[10]=(uint32_t)((d>0)-(d<0));move(s);}break;
    case 100:s[0x54/4]=0;s[0x58/4]=101;break;
    case 101:s[5]+=(uint32_t)((age&1)?8:-12);s[7]+=(uint32_t)((age&1)?8:-12);if(age>192)s[4]=0;break;
    default:steer(w,s,1);return 1;
    }
    if(part==63 && state>=1 && state<=13) {
        phase(w,s,4,state>=10 || blaze_signed(s[0x50/4])<2000?10:1);
        if(state==2 && !blaze_actor_spawn(w,s+5,17,s[11],0))return 0;
    }
    steer(w,s,1);return 1;
}
static int raiya(BlazeWorld *w,uint32_t *s,int age) {
    unsigned state=s[0x58/4],i,h=0;int part=age%128;
    for(i=0;i<256;++i)if(s==w->actors[i]) {h=i+1;break;}
    switch(state) {
    case 0:if(age>64)phase(w,s,5,1);s[9]=UINT32_MAX;s[10]=0;move(s);break;
    case 1:
        s[9]=(uint32_t)(part<64?-6:6);s[10]=0;move(s);
        if(age%24==0) {unsigned angle=blaze_random_next(w->random)*96/32767-48+s[11];int y=(int)(blaze_random_next(w->random)*2/32767)*64-32;if(!offset_spawn(w,s,0,y,25,angle))return 0;}
        break;
    case 2:
        if(age%64<48 && (age&1))for(i=0;i<2;++i) {unsigned angle=blaze_random_next(w->random)*96/32767-48+s[11];int x=-(int)(blaze_random_next(w->random)*128/32767);if(!offset_spawn(w,s,x,i?64:-64,3,angle))return 0;}
        break;
    case 3:
        if(part<96 && age%16==0) {static const int y[]={-32,-16,16,32},angle[]={32,16,-16,-32};for(i=0;i<4;++i)if(!offset_spawn(w,s,0,y[i],6,s[11]+(unsigned)angle[i]))return 0;}
        if(part==127 && !blaze_actor_spawn(w,s+5,17,s[11],0))return 0;break;
    case 4:
        if(age%3==0 && (!blaze_actor_spawn(w,s+5,20,s[11]-(unsigned)(age%64)+32,0) || !blaze_actor_spawn(w,s+5,20,s[11]+(unsigned)(age%64)-32,0)))return 0;
        if(age%16==0 && !blaze_actor_spawn(w,s+5,44,0,0))return 0;break;
    case 5:
        s[11]=(unsigned)(part<64?part:127-part)+144;
        if(age%64==0 && (!blaze_actor_attach(w,h,0,-64,28,s[11],0) || !blaze_actor_attach(w,h,0,64,28,s[11],0)))return 0;break;
    case 10:
        s[9]=(uint32_t)(part<64?-2:2);s[10]=0;move(s);
        if(age%16==0 && !blaze_actor_spawn(w,s+5,23,blaze_random_next(w->random)*256/32767,0))return 0;break;
    case 11:
        s[9]=(uint32_t)(part<64?2:-2);s[10]=0;move(s);
        if(age%8==0) {if(!offset_spawn(w,s,-150,-64,43,blaze_random_next(w->random)*256/32767) || !offset_spawn(w,s,-150,64,43,blaze_random_next(w->random)*256/32767))return 0;}
        if(part==32 && !blaze_actor_attach(w,h,0,0,29,(unsigned)-16,0))return 0;
        if(part==127 && !blaze_actor_spawn(w,s+5,17,s[11],0))return 0;break;
    case 12:
        if(age%64<32 && !blaze_actor_spawn(w,s+5,34,blaze_random_next(w->random)*128/32767-64+s[11],0))return 0;break;
    case 13:
        if(age%64<16) {int t=age%32;t=t<16?t:31-t;if(!blaze_actor_spawn(w,s+5,33,(unsigned)(t*20-64),0))return 0;}break;
    case 100:s[0x54/4]=0;s[0x58/4]=101;break;
    case 101:s[5]+=(uint32_t)((age&1)?8:-16);s[7]+=(uint32_t)((age&1)?8:-16);s[6]+=(uint32_t)((age&1)?-4:4);s[8]+=(uint32_t)((age&1)?-4:4);if(age>192)s[4]=0;break;
    default:steer(w,s,1);return 1;
    }
    if(part==127 && state>=1 && state<=13)phase(w,s,state>=10 || blaze_signed(s[0x50/4])<2000?4:5,state>=10 || blaze_signed(s[0x50/4])<2000?10:1);
    steer(w,s,1);return 1;
}
static int boss(BlazeWorld *w,uint32_t *s,int age) {
    unsigned state=s[0x58/4],i;int part=age%128;
    switch(state) {
    case 0:if(age>12)phase(w,s,4,1);s[9]=(uint32_t)-8;s[10]=8;move(s);break;
    case 1:case 2:
        for(i=0;i<2;++i) {int dy; /* Recompute after every move. */
            const uint32_t *p=w->player.words;const BlazeSprite *a=blaze_level_sprite(w->level,s[0]),*b=blaze_level_sprite(w->level,p[0]);
            int delta=blaze_signed(p[6]+b->visible_height/2-s[6]-a->visible_height/2);
            if(state==1)delta+=part<64?64:-64;
            dy=(delta>0)-(delta<0);s[9]=state==2?0:part<64?UINT32_MAX:1;s[10]=(uint32_t)dy;move(s);
        }
        if(state==2 && part<14 && !blaze_actor_spawn(w,s+5,3,blaze_random_next(w->random)*96/32767-48+s[11],0))return 0;
        if(part==127) {phase(w,s,4,1);if(state==2 && !blaze_actor_spawn(w,s+5,17,s[11],0))return 0;}
        break;
    case 3:
        s[9]=part<64?1:UINT32_MAX;s[10]=0;move(s);
        if(part<64 && age%16==0) {
            static const int angles[]={32,64,-32,-64};
            for(i=0;i<4;++i)if(!blaze_actor_spawn(w,s+5,6,s[11]+(uint32_t)angles[i],0))return 0;
        }
        if(part==127)phase(w,s,4,1);break;
    case 4:
        if(part>64 && age%3==0) {
            if(!blaze_actor_spawn(w,s+5,20,s[11]+32,0) || !blaze_actor_spawn(w,s+5,20,s[11]-32,0))return 0;
        }
        {int delta=part<65?480-blaze_signed(s[6]):-blaze_signed(s[6]);s[9]=0;s[10]=(uint32_t)((delta>0)-(delta<0));move(s);}
        if(part==127)phase(w,s,4,1);break;
    case 100:s[0x54/4]=0;s[0x58/4]=101;break;
    case 101:s[5]+=(uint32_t)((age&1)?8:-12);s[7]+=(uint32_t)((age&1)?8:-12);if(age>192)s[4]=0;break;
    }
    steer(w,s,1);return 1;
}
int blaze_actor_update(BlazeWorld *w,uint32_t *s,unsigned pass,char *error,size_t capacity) {
    int age,target_y,actor_y;unsigned type=s[0x4c/4];
    if(!s[4] || (pass==1 && (s[0x44/4]&64)) || (pass==0 && !(s[0x44/4]&64)))return 1;
    if(!supported(w->level->stage,type))return blocked(error,capacity,"update variant");
    if(blaze_signed(s[0x64/4])>0) {
        if(w->tick%3==0) {s[0x64/4]-=4;s[0x5c/4]=UINT32_MAX;blaze_sound(w,BLAZE_HIT);}
        else s[0x5c/4]=0;
    }
    age=blaze_signed(s[0x54/4]);
    if(type==4) {
        if(!(w->level->stage==4?raiya(w,s,age):w->level->stage==3?isabel(w,s,age):w->level->stage==2?sesiru(w,s,age):w->level->stage==1?feria(w,s,age):boss(w,s,age)))return blocked(error,capacity,"boss firing");
    } else if(type==5) {
        unsigned h=s[0x34/4];uint32_t *p;if(!h || h>256)return blocked(error,capacity,"invalid turret parent");p=w->actors[h-1];
        place(w,s,p[5]+s[0x38/4],p[6]+s[0x3c/4]);steer(w,s,3);
        if(w->level->stage==2) {
            resize_actor(w,s,0x140,-2);
            if(!draw(w,s,s[3]%32<6?s[3]&1:0,256,blaze_signed(s[0x5c/4]),0))return blocked(error,capacity,"draw capacity");
        } else if(!rotated(w,s,s[3]%32<6?s[3]&1:0,s[11]))return blocked(error,capacity,"draw capacity");
        if(!p[4] || blaze_signed(p[0x58/4])>99)s[4]=0;
        if(s[3]%32==6 && (!blaze_actor_spawn(w,s+5,3,s[11]-3,0) || !blaze_actor_spawn(w,s+5,3,s[11]+3,0) || !blaze_actor_spawn(w,s+5,3,s[11],0)))return blocked(error,capacity,"turret firing");
    } else if(type==6) {
        unsigned direction=s[11]&255;s[9]=(uint32_t)(w->trig[direction]/24);s[10]=(uint32_t)(-w->trig[direction+64]/24);move(s);
        if(!rotated(w,s,0,s[11]))return blocked(error,capacity,"draw capacity");
        if(age<16)steer(w,s,2);
        if(blaze_player_contact(w,s,10))s[4]=0;
    } else if(type==7) {
        uint32_t *p=follow(w,s,0);if(!p)return blocked(error,capacity,"invalid body parent");
        if(w->level->stage==1 && (w->tick&3)) {
            int frame=blaze_signed(s[11]),target=blaze_signed(p[10])<0?0:blaze_signed(p[10])>0?5:2;
            s[11]+=(uint32_t)((frame<target)-(frame>target));
        }
        if(w->level->stage==3)s[11]=p[0x40/4];
        if(w->level->stage==4)s[11]=p[0x58/4]==2?0:p[0x58/4]==4 || p[0x58/4]==11?(unsigned)age%9:(unsigned)(age/16)%9;
        if(!draw(w,s,w->level->stage==1 || w->level->stage>=3?s[11]:0,256,p[0x5c/4]==0?1:-1,0))return blocked(error,capacity,"draw capacity");
        if(w->level->stage==4 && age<32)w->actor_draws[w->actor_draw_count-1].wave_age=(unsigned)age+1;
        if(!p[4])s[4]=0;
    } else if(type==32) {
        uint32_t *p=follow(w,s,0);unsigned state;if(!p)return blocked(error,capacity,"invalid final body parent");state=p[0x58/4];
        s[11]=state==2?0:state==4?(unsigned)age%9:state==11?8-(unsigned)age%9:(unsigned)(age/16)%9;
        if(!draw(w,s,s[11],256,p[0x5c/4]==0?1:-1,0))return blocked(error,capacity,"draw capacity");
        if(age<32)w->actor_draws[w->actor_draw_count-1].wave_age=(unsigned)age+1;
        if(!p[4])s[4]=0;
    } else if(type==9) {
        unsigned h=s[0x34/4];uint32_t *p;if(!h || h>256)return blocked(error,capacity,"invalid blade parent");p=w->actors[h-1];
        if(s[3]&1) {follow(w,s,0);if(!rotated(w,s,0,s[11]))return blocked(error,capacity,"draw capacity");blaze_player_contact(w,s,1);}
        if(!p[4] || blaze_signed(p[0x58/4])>99)s[4]=0;
    } else if(type==13) {
        const BlazeSprite *a=blaze_level_sprite(w->level,0x167);
        if(!age && w->media && w->media->stop)w->media->stop(w->media->context);
        if(age%32==0)blaze_sound(w,BLAZE_PI);
        if(age<50)move(s);
        if(!caption(w,4,0,208,0xf7,0,UINT32_MAX))return blocked(error,capacity,"draw capacity");
        if(age%32<28 && (age&1)) {
            if(!draw(w,s,0,256,-1,0))return blocked(error,capacity,"draw capacity");
            w->actor_draws[w->actor_draw_count-1].x=160-a->width/2;w->actor_draws[w->actor_draw_count-1].y=64;
        }
        if(!age) {w->warning_characters=0;if(w->level->stage<4)w->encounter_phase=1;}
        if(!caption(w,3,0,0,0,0x24,w->warning_characters/2))return blocked(error,capacity,"draw capacity");
        ++w->warning_characters;if(age==120) {if(w->media && w->media->play)w->media->play(w->media->context,1,1);s[4]=0;}
    } else if(type==14) {
        unsigned h=s[0x34/4];uint32_t *p;if(!h || h>256)return blocked(error,capacity,"invalid death parent");p=w->actors[h-1];
        if(!age && w->media && w->media->stop)w->media->stop(w->media->context);
        if(age==7)blaze_sound(w,BLAZE_DED);
        if(!age && !fill(w,0,0,320,240,255))return blocked(error,capacity,"draw capacity");
        if(age==96) {s[4]=0;w->requested_screen=4;}
        if(blaze_random_next(w->random)*8/32767==0)blaze_sound(w,BLAZE_GOO);
        {const BlazeSprite *a=blaze_level_sprite(w->level,s[0]);place(w,s,p[5]+(uint32_t)(blaze_signed(p[7]-p[5])/2-a->width/2),p[6]+(uint32_t)(blaze_signed(p[8]-p[6])/2-a->visible_height/2));}
        if(age<9) {resize_actor(w,s,0x155,-2);if(!draw(w,s,0,(age+4)*64,-1,1))return blocked(error,capacity,"draw capacity");}
        else {uint32_t rect[4];unsigned scale=blaze_random_next(w->random)*128/32767+400;
            int dy=(int)(blaze_random_next(w->random)*256/32767)-128,dx=(int)(blaze_random_next(w->random)*256/32767)-128;
            rect[0]=s[5]+(uint32_t)dx;rect[1]=s[6]+(uint32_t)dy;rect[2]=s[7]+(uint32_t)dx;rect[3]=s[8]+(uint32_t)dy;
            if(!blaze_actor_spawn(w,rect,47,scale,0))return blocked(error,capacity,"boss debris");
        }
        if(w->loadout->mode==4 && !caption(w,5,0,0,0,0x24,s[11]))return blocked(error,capacity,"draw capacity");
    } else if(type==15 || type==28 || type==38 || type==45) {
        uint32_t *p=follow(w,s,type==45?16:0);if(!p)return blocked(error,capacity,"invalid boss child parent");
        if(type==15) {
            const BlazeSprite *a=blaze_level_sprite(w->level,s[0]);
            place(w,s,p[5]+s[0x38/4]+(uint32_t)(blaze_signed(p[7]-p[5])/2-a->width/2),p[6]+s[0x3c/4]+(uint32_t)(blaze_signed(p[8]-p[6])/2-a->visible_height/2));
            resize_actor(w,s,0x2b5,-2);
            if(!rotated(w,s,0,s[11]))return blocked(error,capacity,"draw capacity");
            if(age<32)s[11]=(uint32_t)(blaze_signed(age<16?s[11]-s[0x58/4]:s[11]+s[0x58/4])%256);
            else steer(w,s,1);
            if(p[0x58/4]==4 && age>64) {unsigned angle=s[11],i,h=0;for(i=0;i<256;++i)if(s==w->actors[i]) {h=i+1;break;}
                if(!blaze_actor_attach(w,h,0,0,28,s[11],0))return blocked(error,capacity,"boss laser");s[0x54/4]=0;steer(w,s,1);s[0x58/4]=angle-s[11];if(!s[0x58/4])s[0x58/4]=1;}
            if(blaze_signed(s[11])>224)s[11]=224;if(blaze_signed(s[11])<160)s[11]=160;
            if(!p[4])s[4]=0;
        } else if(type==28) {
            if(age==0 || age==16)blaze_sound(w,BLAZE_LIG);
            unsigned i,angle=p[11]&255;const BlazeSprite *a=blaze_level_sprite(w->level,s[0]);
            place(w,s,p[5]+s[0x38/4]+(uint32_t)(blaze_signed(p[7]-p[5])/2-a->width/2),p[6]+s[0x3c/4]+(uint32_t)(blaze_signed(p[8]-p[6])/2-a->visible_height/2));
            resize_actor(w,s,age<16?0x4f:0xab,age<16?-7:-2);s[9]=(uint32_t)(w->trig[angle]/(age<16?20:10));s[10]=(uint32_t)(-w->trig[angle+64]/(age<16?20:10));
            if(age==32)s[4]=0;
            for(i=0;i<(age<16?32u:16u);++i) {move(s);if(age<16 && blaze_random_next(w->random)*5/32767==0)continue;
                if(!rotated(w,s,age<16?0:s[3]&1,p[11]))return blocked(error,capacity,"draw capacity");
                if(age>=16 && blaze_player_contact(w,s,6))break;
            }
            if(!p[4])s[4]=0;
        } else if(type==38) {
            const BlazeSprite *a=blaze_level_sprite(w->level,s[0]);
            place(w,s,p[5]+s[0x38/4]+(uint32_t)(blaze_signed(p[7]-p[5])/2-a->width/2),p[6]+s[0x3c/4]+(uint32_t)(blaze_signed(p[8]-p[6])/2-a->visible_height/2));
            if(!s[11])blaze_player_contact(w,s,2);
            if(blaze_signed(s[0x50/4])<200)p[0x50/4]+=s[0x50/4]-200;s[0x50/4]=200;
            if(!p[4] || blaze_signed(p[0x58/4])>99)s[4]=0;
        } else {
            const BlazeSprite *a=blaze_level_sprite(w->level,s[0]);int expand=age<64?0:(age-64)*(age-64);
            int x=blaze_signed(s[5])+a->width/2-w->camera_x,y=blaze_signed(s[6])+a->visible_height/2-w->camera_y,color=s[11]?249:250;
            if(!fill(w,0,y-expand,320,y+1-expand,color) || !fill(w,0,y+expand,320,y+1+expand,color) || !fill(w,x-expand,0,x+1-expand,240,color) || !fill(w,x+expand,0,x+1+expand,240,color) || !caption(w,s[11]?2:1,x,y,0xf7,0,UINT32_MAX))return blocked(error,capacity,"draw capacity");
            if(!draw(w,s,0,256,-1,0))return blocked(error,capacity,"draw capacity"); /* Original shared tail draws sprite zero. */
        }
    } else if(type==17) {
        if(!age)place(w,s,(uint32_t)w->camera_x+320,120);s[9]=(uint32_t)-2;s[10]=0;move(s);
        if(!draw(w,s,0,256,blaze_signed(s[0x5c/4]),0))return blocked(error,capacity,"draw capacity");
        if(blaze_player_contact(w,s,10))s[4]=0;
        if(blaze_signed(s[0x50/4])<0) {
            unsigned a=blaze_random_next(w->random)*10/32767+1;if(!offset_spawn(w,s,0,-32,11,a))return blocked(error,capacity,"weapon drop");
            a=blaze_random_next(w->random)*10/32767+1;if(!offset_spawn(w,s,0,32,11,a))return blocked(error,capacity,"weapon drop");
        }
    } else if(type==47) {
        if(age==17)s[4]=0;if(!draw(w,s,(unsigned)age/3,256,-1,1))return blocked(error,capacity,"draw capacity");s[9]=(uint32_t)-16;s[10]=0;move(s);
    } else if(type==12) {
        if(age<=20)move(s);
        else {
            if(!draw(w,s,0,256,blaze_signed(s[0x5c/4]),0))return blocked(error,capacity,"draw capacity");
            if(age%96>49) {
                const uint32_t *p=w->player.words;const BlazeSprite *ps=blaze_level_sprite(w->level,p[0]);
                int delta=blaze_signed(p[6]+ps->visible_height/2+s[11]-s[6]);
                s[9]=0;s[10]=(uint32_t)((delta>0)-(delta<0));move(s);
            }
            if(age%96==0)s[11]=blaze_random_next(w->random)*64/32767-32;
            if(age>500) {s[9]=(uint32_t)-8;s[10]=0;move(s);}
            else {
                if(age%96>31 && age%96<50 && (!(age&15) || (age&15)==4)) {
                    static const unsigned directions[]={184,152,216,168,200};unsigned i;
                    unsigned jitter=blaze_random_next(w->random)*16/32767;
                    for(i=0;i<5;++i)if(!blaze_actor_spawn(w,s+5,3,jitter+directions[i],0))
                        return blocked(error,capacity,"enemy projectile");
                }
                if(w->level->stage==1 && age%96<32 && age%8==0) {
                    unsigned direction=blaze_random_next(w->random)*256/32767;
                    if(!offset_spawn(w,s,32,-32,6,direction))return blocked(error,capacity,"carrier homing shot");
                }
                if(w->level->stage==2 && age%96<32) {
                    if(age%32==0 && !offset_spawn(w,s,8,-32,23,210))return blocked(error,capacity,"carrier trail");
                    if(age%28==0 && !offset_spawn(w,s,8,-32,23,224))return blocked(error,capacity,"carrier trail");
                }
                if(w->level->stage==3 && age%96<32 && age%32==0) {
                    unsigned i,h=0;for(i=0;i<256;++i)if(s==w->actors[i]) {h=i+1;break;}
                    if(!blaze_actor_attach(w,h,-32,0,18,192,0))return blocked(error,capacity,"carrier beam");
                }
                if(blaze_signed(s[0x50/4])<0 && s[0x68/4] && !blaze_actor_spawn(w,s+5,11,3,0))
                    return blocked(error,capacity,"weapon drop");
            }
        }
    } else if(type==18 || type==29 || type==30) {
        unsigned parent=s[0x34/4];uint32_t *p;
        if(!parent || parent>256)return blocked(error,capacity,"invalid boss weapon parent");p=w->actors[parent-1];
        if(type==18) {
            s[4]=p[4];
            if(age<32) {
                const BlazeSprite *a=blaze_level_sprite(w->level,s[0]);
                if(!age)blaze_sound(w,BLAZE_SUU);
                place(w,s,p[5]+s[0x38/4]+(uint32_t)(blaze_signed(p[7]-p[5])/2-a->width/2),p[6]+s[0x3c/4]+(uint32_t)(blaze_signed(p[8]-p[6])/2-a->visible_height/2));
                if(!draw(w,s,(unsigned)age&1,blaze_random_next(w->random)*256/32767+256,-1,1))return blocked(error,capacity,"draw capacity");
            } else {
                unsigned direction=s[11]&255;if(age==32)blaze_sound(w,BLAZE_BOSU);
                resize_actor(w,s,0x14f,-2);s[9]=(uint32_t)(w->trig[direction]/10);s[10]=(uint32_t)(-w->trig[direction+64]/10);move(s);
                if(!draw(w,s,(unsigned)age&1,256,-1,1))return blocked(error,capacity,"draw capacity");
                blaze_player_contact(w,s,64);
            }
        } else if(type==29) {
            unsigned j;follow(w,s,0);
            {const BlazeSprite *a=blaze_level_sprite(w->level,s[0]);place(w,s,s[5]+(uint32_t)(blaze_signed(s[7]-s[5])/2-a->width/2),s[6]+(uint32_t)(blaze_signed(s[8]-s[6])/2-a->visible_height/2));}
            if(!age)blaze_sound(w,BLAZE_PUN);if(age>32 && age%3==0)blaze_sound(w,BLAZE_LIG);
            for(j=0;j<32;++j) {
                s[9]=(uint32_t)(-32-(int)(blaze_random_next(w->random)*32/32767));s[10]=0;move(s);
                if(!draw(w,s,0,256,-1,1))return blocked(error,capacity,"draw capacity");
                if(age<33) {BlazeActorDraw *d=&w->actor_draws[w->actor_draw_count-1];const BlazeSprite *a=blaze_level_sprite(w->level,s[0]);
                    d->height_scale=age?age*8:-1;d->y+=((int)a->visible_height-(int)a->visible_height*age*8/256)/2;
                } else if(blaze_player_contact(w,s,4))break;
            }
            if(age==64 || !p[4])s[4]=0;
        } else {
            const uint32_t *player=w->player.words;const BlazeSprite *a=blaze_level_sprite(w->level,s[0]),*b=blaze_level_sprite(w->level,player[0]);
            s[11]=blaze_aim(blaze_signed(player[5]+b->width/2),blaze_signed(player[6]+b->visible_height/2),blaze_signed(s[5]+a->width/2),blaze_signed(s[6]+a->visible_height/2));
            if(age<16)move(s);
            else if(age%32==0) {unsigned i,h=0;for(i=0;i<256;++i)if(s==w->actors[i]) {h=i+1;break;}
                if(!blaze_actor_attach(w,h,0,0,10,s[11],0))return blocked(error,capacity,"boss missile trail");}
            if(!rotated(w,s,(unsigned)age%6,s[11]) || !hit_sparks(w,s))return blocked(error,capacity,"draw capacity");
            if(!p[4])s[4]=0;
        }
    } else if(type==34) {
        unsigned direction=s[11]&255;int divisor=48-age/4;
        if(!age)blaze_sound(w,BLAZE_SHT);
        if(!divisor)return blocked(error,capacity,"type-34 zero speed divisor");
        s[9]=(uint32_t)(w->trig[direction]/divisor);s[10]=(uint32_t)(-w->trig[direction+64]/divisor);move(s);
        if(blaze_player_contact(w,s,8))s[4]=0;
        if(age<64)steer(w,s,3);
        if(!draw(w,s,0,256,-1,1))return blocked(error,capacity,"draw capacity");
    } else if(type==25) {
        unsigned direction=s[11]&255;
        if(age==32)blaze_sound(w,BLAZE_LIG);if(age==48)s[4]=0;
        if(age<32) {resize_actor(w,s,0x91,-2);s[9]=(uint32_t)(w->trig[direction]/48);s[10]=(uint32_t)(-w->trig[direction+64]/48);move(s);
            if(!rotated(w,s,(unsigned)age&1,s[11]))return blocked(error,capacity,"draw capacity");steer(w,s,2);
        } else if(age%3) {resize_actor(w,s,0x146,-32);if(!draw(w,s,0,256,-1,1))return blocked(error,capacity,"draw capacity");blaze_player_contact(w,s,3);}
    } else if(type==39) {
        if(age==0)blaze_sound(w,BLAZE_PUU);
        s[9]=s[11];s[10]=(uint32_t)(age/4-8);move(s);if(blaze_player_contact(w,s,8))s[4]=0;
        if(!draw(w,s,0,256,-1,0))return blocked(error,capacity,"draw capacity");
    } else if(type==40) {
        if(age<25) {s[9]=(uint32_t)-12;s[10]=(uint32_t)(vertical_delta(w,s,64)*8);}
        else {s[9]=0;s[10]=(uint32_t)(vertical_delta(w,s,0)*4);
            if(!draw(w,s,0,256,blaze_signed(s[0x5c/4]),0))return blocked(error,capacity,"draw capacity");
            if(age%16==0 && !blaze_actor_spawn(w,s+5,16,1,0))return blocked(error,capacity,"type-40 missile");}
        move(s);if(!draw(w,s,s[3]&1,256,-1,0))return blocked(error,capacity,"draw capacity");
        if(blaze_signed(s[0x50/4])<0 && s[0x68/4] && !blaze_actor_spawn(w,s+5,11,9,0))return blocked(error,capacity,"weapon drop");
        if(blaze_player_contact(w,s,4))s[4]=0;
    } else if(type==23) {
        blaze_sound(w,BLAZE_KIL);blaze_enemy_trail_create(w,s+5,s[11]);s[4]=0;
    } else if(type==24) {
        if(!hit_sparks(w,s) || !draw(w,s,(unsigned)(blaze_signed(s[11])/16)%16,256,-1,0))return blocked(error,capacity,"draw capacity");
        if(age<32) {s[9]=(uint32_t)-4;s[10]=0;move(s);}
        move(s);
        if(clamp_actor(w,s) || age%32==0) {
            unsigned angle=blaze_random_next(w->random)*256/32767;
            s[9]=(uint32_t)(w->trig[angle&255]/35);s[10]=(uint32_t)(-w->trig[(angle&255)+64]/35);
        }
        if(age>31 && age%64==0) {
            if(blaze_random_next(w->random)*2/32767==0) {
                unsigned i;for(i=0;i<4;++i) {
                    unsigned angle=blaze_random_next(w->random)*256/32767;
                    if(!blaze_actor_spawn(w,s+5,33,angle,0))return blocked(error,capacity,"type-24 shot");
                }
            } else {
                unsigned i,h=0;for(i=0;i<256;++i)if(s==w->actors[i]) {h=i+1;break;}
                if(!blaze_actor_attach(w,h,0,0,10,s[11],0))return blocked(error,capacity,"type-24 attachment");
            }
        }
        if(age%64>32)steer(w,s,3);
        if(blaze_signed(s[0x50/4])<0 && s[0x68/4] && !blaze_actor_spawn(w,s+5,11,7,0))return blocked(error,capacity,"weapon drop");
    } else if(type==36) {
        uint32_t rect[4];unsigned i;memcpy(rect,s+5,16);
        if(age<16) {
            if(!(age&1) && !caption(w,6,0,blaze_signed(s[6])-w->camera_y,0,2,UINT32_MAX))return blocked(error,capacity,"draw capacity");
            if(!age)blaze_sound(w,BLAZE_PI);
        } else {
            if(age==16)blaze_sound(w,BLAZE_LIG);
            if(age<32)for(i=0;i<(unsigned)(age-16) && i<32;++i) {
                move(s);if(!draw(w,s,0,256,-1,1))return blocked(error,capacity,"draw capacity");
                if(blaze_player_contact(w,s,6))break;
            }
            if(age>=32 && age<48)for(i=0;i<32;++i) {
                BlazeActorDraw *d;move(s);if(!draw(w,s,0,256,-1,1))return blocked(error,capacity,"draw capacity");
                d=&w->actor_draws[w->actor_draw_count-1];d->height_scale=(48-age)*16;
                {const BlazeSprite *a=blaze_level_sprite(w->level,s[0]);d->y+=((int)a->visible_height-(int)a->visible_height*d->height_scale/256)/2;}
            }
            memcpy(s+5,rect,16);
        }
    } else if(type==31) {
        unsigned direction,i,h=0;
        if(age<=192) {
            const uint32_t *p=w->player.words;const BlazeSprite *ps=blaze_level_sprite(w->level,p[0]),*as=blaze_level_sprite(w->level,s[0]);
            s[11]=blaze_turn(w->trig,s[11],blaze_signed(p[5]+ps->width/2-128),blaze_signed(p[6]+ps->visible_height/2-64),
                blaze_signed(s[5]+as->width/2),blaze_signed(s[6]+as->visible_height/2),16);
            direction=s[11]&255;
        } else direction=80;
        s[9]=(uint32_t)(w->trig[direction]/38);s[10]=(uint32_t)(-w->trig[direction+64]/38);move(s);
        if(age%64==32 && !blaze_actor_spawn(w,s+5,35,0,0))return blocked(error,capacity,"type-31 projectile");
        if(age%64==0) {
            for(i=0;i<256;++i)if(s==w->actors[i]) {h=i+1;break;}
            if(!blaze_actor_attach(w,h,16,24,10,76,0))return blocked(error,capacity,"type-31 attachment");
        }
        if(!draw(w,s,s[3]&1,256,blaze_signed(s[0x5c/4]),0))return blocked(error,capacity,"draw capacity");
        if(blaze_signed(s[0x50/4])<0 && s[0x68/4] && !blaze_actor_spawn(w,s+5,11,6,0))return blocked(error,capacity,"weapon drop");
    } else if(type==10 || type==42) {
        uint32_t angle=s[11],rect[4];unsigned i;const uint32_t *parent=type==10?follow(w,s,0):s;
        memcpy(rect,s+5,16);
        if(!parent)return blocked(error,capacity,"invalid effect parent");
        if(type==10) {const BlazeSprite *sp=blaze_level_sprite(w->level,s[0]);
            uint32_t x=s[5]+(uint32_t)(blaze_signed(s[7]-s[5])/2-sp->width/2);
            uint32_t y=s[6]+(uint32_t)(blaze_signed(s[8]-s[6])/2-sp->visible_height/2);
            place(w,s,x,y);
        }
        if(age==16)blaze_sound(w,BLAZE_LIG);
        for(i=0;i<32;++i) {
            unsigned direction=s[11]&255,frame;
            s[9]=(uint32_t)(w->trig[direction]/10);s[10]=(uint32_t)(-w->trig[direction+64]/10);
            s[11]+=(w->random->table[(i+s[0x48/4])&255]&31)-16;move(s);
            frame=blaze_random_next(w->random)*3/32767+(age<16?0:3);
            if(!rotated(w,s,frame,s[11]))return blocked(error,capacity,"draw capacity");
            if(age>=16 && blaze_player_contact(w,s,3))break;
        }
        s[11]=angle;if(type==42)memcpy(s+5,rect,16);else if(!parent[4])s[4]=0;
    } else if(type==33) {
        const uint32_t *p=w->player.words;
        const BlazeSprite *a=blaze_level_sprite(w->level,s[0]),*b=blaze_level_sprite(w->level,p[0]);
        unsigned direction;
        if(age==32)blaze_sound(w,BLAZE_PUN);
        if(age<24) {direction=s[11]&255;s[9]=(uint32_t)(w->trig[direction]/40);s[10]=(uint32_t)(-w->trig[direction+64]/40);move(s);}
        if(age>32) {
            if(blaze_player_contact(w,s,20))s[4]=0;
            direction=s[11]&255;s[9]=(uint32_t)(w->trig[direction]/16);s[10]=(uint32_t)(-w->trig[direction+64]/16);move(s);steer(w,s,2);
        }
        if(age==24)s[11]=blaze_aim(blaze_signed(p[5]+b->width/2),blaze_signed(p[6]+b->visible_height/2),blaze_signed(s[5]+a->width/2),blaze_signed(s[6]+a->visible_height/2));
        if(!draw(w,s,(unsigned)age&1,256,-1,1))return blocked(error,capacity,"draw capacity");
    } else if(type==35) {
        if(s[3]==16)blaze_sound(w,BLAZE_SHT);
        if(s[3]>16) {move(s);if(!draw(w,s,0,256,-1,0))return blocked(error,capacity,"draw capacity");}
        else if(age&1) {
            const BlazeSprite *old=blaze_level_sprite(w->level,s[0]),*next=blaze_level_sprite(w->level,0xa3);
            s[5]+=(uint32_t)(((int)old->width-next->width)/2);s[6]+=(uint32_t)(((int)old->visible_height-next->visible_height)/2);
            s[0]=0xa3;s[7]=s[5]+next->width;s[8]=s[6]+next->visible_height;s[12]=(16-s[3])*4;
            if(!draw(w,s,0,(20-(int)s[3])*64,-1,0))return blocked(error,capacity,"draw capacity");
            if(blaze_player_contact(w,s,18))s[4]=0;
        }
    } else if(type==44) {
        if(!draw(w,s,s[3]%3,256,blaze_signed(s[0x5c/4]),0))return blocked(error,capacity,"draw capacity");
        move(s);if(blaze_player_contact(w,s,10))s[4]=0;
        if(blaze_signed(s[0x50/4])<0) {
            unsigned angle;for(angle=0;angle<256;angle+=32)
                if(!blaze_actor_spawn(w,s+5,43,angle,0))return blocked(error,capacity,"type-44 death ring");
        }
    } else if(type==19) {
        unsigned phase=(unsigned)age%64;const uint32_t *p=w->player.words;
        const BlazeSprite *as=blaze_level_sprite(w->level,s[0]),*ps=blaze_level_sprite(w->level,p[0]);
        if(phase<16) {
            uint32_t dx,dy,distance;unsigned direction;
            steer(w,s,6);direction=(s[11]+(s[0x58/4]?64:192))&255;
            s[9]=(uint32_t)(w->trig[direction]/25);s[10]=(uint32_t)(-w->trig[direction+64]/25);move(s);
            dx=s[5]+as->width/2-p[5]-ps->width/2;dy=s[6]+as->visible_height/2-p[6]-ps->visible_height/2;
            /* 0x4199a0 returns wrapped squared distance, not a square root. */
            distance=dx*dx+dy*dy;direction=(s[11]+(blaze_signed(distance)<=128?192:0))&255;
            s[9]=(uint32_t)(w->trig[direction]/20);s[10]=(uint32_t)(-w->trig[direction+64]/20);move(s);
        } else steer(w,s,6);
        if(phase>48 && !(phase&1)) {
            unsigned direction=blaze_random_next(w->random)*32/32767-16+s[11];
            if(!blaze_actor_spawn(w,s+5,3,direction,0))return blocked(error,capacity,"type-19 shot");
            s[0x58/4]=blaze_random_next(w->random)*2/32767;
        }
        /* Original clamp uses horizontal camera bounds and world Y 0..320. */
        if(blaze_signed(s[5])<w->camera_x) {s[5]=(uint32_t)w->camera_x;s[7]=s[5]+as->width;}
        if(blaze_signed(s[7])>=w->camera_x+320) {s[7]=(uint32_t)(w->camera_x+320);s[5]=s[7]-as->width;}
        if(blaze_signed(s[6])<0) {s[6]=0;s[8]=as->visible_height;}
        if(blaze_signed(s[8])>=320) {s[8]=320;s[6]=320-as->visible_height;}
        if(!draw(w,s,(unsigned)(blaze_signed(s[11])/16)%16,256,-1,0))return blocked(error,capacity,"draw capacity");
        if(blaze_signed(s[0x50/4])<0 && s[0x68/4] && !blaze_actor_spawn(w,s+5,11,8,0))return blocked(error,capacity,"weapon drop");
    } else if(type==21) {
        unsigned parent=s[0x34/4],i;const uint32_t *p;
        if(!parent || parent>256)return blocked(error,capacity,"invalid attachment parent");
        p=w->actors[parent-1];
        for(i=0;i<4;++i)s[5+i]=p[5+i]+s[(i&1)?0x3c/4:0x38/4];
        if(!draw(w,s,s[3]&1,256,blaze_signed(p[0x5c/4]),0))return blocked(error,capacity,"draw capacity");
        if(age>=0 && age<32)w->actor_draws[w->actor_draw_count-1].wave_age=(unsigned)age+1;
        if(!p[4])s[4]=0;
    } else if(type==2) {
        unsigned direction=s[11]&255;
        s[9]=(uint32_t)(w->trig[direction]/48);s[10]=(uint32_t)(-w->trig[direction+64]/48);move(s);
        if(!rotated(w,s,s[3]&3,s[11]) || !hit_sparks(w,s))return blocked(error,capacity,"draw capacity");
        steer(w,s,3);
        if(blaze_random_next(w->random)*32/32767==0 && !blaze_actor_spawn(w,s+5,3,s[11],0))
            return blocked(error,capacity,"enemy projectile");
        if(blaze_signed(s[0x50/4])<0 && s[0x68/4] && !blaze_actor_spawn(w,s+5,11,1,0))
            return blocked(error,capacity,"weapon drop");
    } else if(type==8) {
        if(!draw(w,s,s[11],256,blaze_signed(s[0x5c/4]),0))return blocked(error,capacity,"draw capacity");
        bank(w,s);
        if(age<32) {s[9]=(uint32_t)-1;s[10]=0;move(s);}
        else {
            if(age>=128) {s[9]=(uint32_t)-6;s[10]=(uint32_t)vertical_target(w,s);move(s);}
            else if(!s[0x48/4]) {
                if(age%48<32) {s[9]=(uint32_t)-1;s[10]=(uint32_t)(vertical_target(w,s)*2);move(s);}
                else if(!(age&3) && !offset_spawn(w,s,32,16,16,UINT32_MAX))return blocked(error,capacity,"enemy missile");
                if(age%48==0 && !parent_effect(w,s))return blocked(error,capacity,"attached beam");
            } else {
                s[9]=(uint32_t)-1;s[10]=(uint32_t)(vertical_target(w,s)*(w->tick%18<9?1:5));move(s);
                if(w->tick%3==0 && w->tick%18<9 && !blaze_actor_spawn(w,s+5,3,192,0))
                    return blocked(error,capacity,"enemy projectile");
                if(!(w->tick&31) && !offset_spawn(w,s,32,16,16,UINT32_MAX))return blocked(error,capacity,"enemy missile");
            }
            if(blaze_signed(s[0x50/4])<0 && s[0x68/4] && !blaze_actor_spawn(w,s+5,11,4,0))
                return blocked(error,capacity,"weapon drop");
        }
    } else if(type==3) {
        if(age==1)blaze_sound(w,BLAZE_SHT);
        move(s);
        if(!rotated(w,s,s[3]&1,s[11]))return blocked(error,capacity,"draw capacity");
        if(blaze_player_contact(w,s,10))s[4]=0;
    } else if(type==16) {
        if(age/2<33)s[9]=(uint32_t)(blaze_signed(s[11]*(uint32_t)age)/2)-s[11]*4;
        if(age<5) {
            const uint32_t *p=w->player.words;
            const BlazeSprite *ps=blaze_level_sprite(w->level,p[0]),*as=blaze_level_sprite(w->level,s[0]);
            int dy=blaze_signed(p[6]+ps->visible_height/2-s[6]-as->visible_height/2)/32;
            if(dy>8)dy=8;
            if(dy<-8)dy=-8;
            s[10]=(uint32_t)dy;
        }
        move(s);
        if(!draw(w,s,s[3]&1,256,blaze_signed(s[0x5c/4]),0))return blocked(error,capacity,"draw capacity");
        if(blaze_player_contact(w,s,15))s[4]=0;
    } else if(type==37) {
        if(age==0)blaze_sound(w,BLAZE_PUU);
        if(age==16)blaze_sound(w,BLAZE_UEE);
        unsigned parent=s[0x34/4],i;const uint32_t *p;const BlazeSprite *sprite;
        if(!parent || parent>256)return blocked(error,capacity,"invalid beam parent");
        p=w->actors[parent-1];sprite=blaze_level_sprite(w->level,s[0]);
        s[5]=p[5]+s[0x38/4]+(uint32_t)(blaze_signed(p[7]-p[5])/2-sprite->width/2);
        s[6]=p[6]+s[0x3c/4]+(uint32_t)(blaze_signed(p[8]-p[6])/2-sprite->visible_height/2);
        s[7]=s[5]+sprite->width;s[8]=s[6]+sprite->visible_height;
        if(age<16) {
            if(!draw(w,s,((unsigned)age&3)+1,256,-1,1))return blocked(error,capacity,"draw capacity");
        } else if(age<32) {
            unsigned segments=(unsigned)age*2-16;
            for(i=0;i<segments && i<32;++i) {
                move(s);
                if(!draw(w,s,0,256,-1,1))return blocked(error,capacity,"draw capacity");
                if(blaze_player_contact(w,s,2))break;
            }
        } else if(age<48) {
            for(i=0;i<32;++i) {
                move(s);
                if(!draw(w,s,0,256,-1,1))return blocked(error,capacity,"draw capacity");
                w->actor_draws[w->actor_draw_count-1].height_scale=(48-age)*16;
                w->actor_draws[w->actor_draw_count-1].y+=(sprite->visible_height-(int)((int64_t)sprite->visible_height*(48-age)*16/256))/2;
            }
        }
        if(age>=16 && !p[4])s[4]=0;
    } else if(type==0) {
        if(age<65) {
            move(s);
            if(!draw(w,s,0,256,blaze_signed(s[0x5c/4]),0))return blocked(error,capacity,"draw capacity");
        } else {
            if(!draw(w,s,((unsigned)age/3)&7,256,blaze_signed(s[0x5c/4]),0))return blocked(error,capacity,"draw capacity");
            if(!(age&1)) {
                unsigned direction=blaze_random_next(w->random)*256/32767;
                if(!blaze_actor_spawn(w,s+5,20,direction,0))return blocked(error,capacity,"type-20 projectile");
            }
            move(s);
        }
        if(age==64) {s[9]=4;s[10]=0;}
        steer(w,s,3);
        if(blaze_player_contact(w,s,10))s[4]=0;
        if(blaze_signed(s[0x50/4])<0 && s[0x68/4])
            if(!blaze_actor_spawn(w,s+5,11,5,0))return blocked(error,capacity,"weapon drop");
    } else if(type==20) {
        if(age==1)blaze_sound(w,BLAZE_PUN);
        move(s);
        if(!draw(w,s,s[3]&1,256,-1,1))return blocked(error,capacity,"draw capacity");
        if(blaze_player_contact(w,s,13))s[4]=0;
    } else if(type==1) {
        unsigned direction=s[11]&255;
        s[9]=(uint32_t)(w->trig[direction]/48-1);s[10]=(uint32_t)(-w->trig[direction+64]/48);
        s[11]+=s[0x48/4];move(s);
        if(!draw(w,s,s[3]&1,256,blaze_signed(s[0x5c/4]),0))return blocked(error,capacity,"draw capacity");
        if(blaze_player_contact(w,s,10))s[4]=0;
        if(blaze_signed(s[0x50/4])<0 && s[0x68/4])
            if(!blaze_actor_spawn(w,s+5,11,2,0))return blocked(error,capacity,"weapon drop");
        steer(w,s,4);
        if(blaze_random_next(w->random)*32/32767==0)
            if(!(w->level->stage==3?offset_spawn(w,s,0,32,16,UINT32_MAX):blaze_actor_spawn(w,s+5,43,s[11],0)))return blocked(error,capacity,"enemy projectile");
    } else if(type==43) {
        if(age==1)blaze_sound(w,BLAZE_SHT);
        if(age<32) {
            int divisor=(32-age)*2;unsigned direction=s[11]&255;
            s[9]=(uint32_t)(w->trig[direction]/divisor);s[10]=(uint32_t)(-w->trig[direction+64]/divisor);
        }
        move(s);
        if(!rotated(w,s,s[3]&1,(s[3]&31)<<3))return blocked(error,capacity,"rotated draw capacity");
        if(blaze_player_contact(w,s,18))s[4]=0;
    } else if(type==22) {
        if(age/3<17) {
            const BlazeSprite *player=blaze_level_sprite(w->level,w->player.words[0]);
            const BlazeSprite *actor=blaze_level_sprite(w->level,s[0]);
            target_y=blaze_signed(w->player.words[6])+player->visible_height/2;
            actor_y=blaze_signed(s[6])+actor->visible_height/2;
            s[9]=(uint32_t)(-age/3);s[10]=(uint32_t)(((target_y>actor_y)-(target_y<actor_y))*2);
        }
        move(s);
        if(!(w->level->stage==3?rotated(w,s,(unsigned)age&7,s[11]):draw(w,s,w->level->stage==2?(unsigned)age&15:s[3]&1,256,blaze_signed(s[0x5c/4]),0)))return blocked(error,capacity,"draw capacity");
        if(w->level->stage==3)steer(w,s,8);
        if(w->level->stage>=2 && blaze_random_next(w->random)*8/32767==0) {
            unsigned angle=w->level->stage==3?0:blaze_random_next(w->random)*256/32767;
            if(!blaze_actor_spawn(w,s+5,3,w->level->stage==3?s[11]:angle,0))return blocked(error,capacity,"type-22 shot");
        }
        if(blaze_signed(s[0x50/4])<0 && s[0x68/4])
            if(!blaze_actor_spawn(w,s+5,11,10,0))return blocked(error,capacity,"weapon drop");
        if(blaze_player_contact(w,s,10))s[4]=0;
    } else if(type==26) {
        if(!draw(w,s,(unsigned)age,blaze_signed(s[11]),-1,1))return blocked(error,capacity,"draw capacity");
        if(age>2)s[4]=0;
    } else if(type==27 || type==41) {
        if(!age)blaze_sound(w,BLAZE_GOO);
        if(!age && w->loadout->mode==4)
            if(!blaze_actor_spawn(w,s+5,46,s[11],0))return blocked(error,capacity,"score label");
        if(age==17)s[4]=0;
        if(type==27 && !draw(w,s,(unsigned)age/3,256,-1,1))return blocked(error,capacity,"draw capacity");
        s[9]=(uint32_t)-16;s[10]=0;move(s);
        if(type==41 && (age&1) && age<16) {
            uint32_t rect[4];unsigned scale=blaze_random_next(w->random)*64/32767+256;
            int dy=(int)(blaze_random_next(w->random)*48/32767)-24;
            int dx=(int)(blaze_random_next(w->random)*48/32767)-24;
            rect[0]=s[5]+(uint32_t)dx;rect[2]=s[7]+(uint32_t)dx;
            rect[1]=s[6]+(uint32_t)dy;rect[3]=s[8]+(uint32_t)dy;
            if(!blaze_actor_spawn(w,rect,26,scale,0))return blocked(error,capacity,"smoke");
        }
    } else if(type==11) {
        unsigned weapon=s[11],enabled=0,i;
        if(w->loadout->mode==1 || w->loadout->mode==4)s[4]=0;
        else {
            for(i=1;i<11;++i)enabled+=w->loadout->weapons[i].enabled!=0;
            if(!enabled)return blocked(error,capacity,"pickup without any enabled weapon");
            if(weapon>11)return blocked(error,capacity,"invalid pickup weapon");
            /* Deliberate bounds fix: inclusive rand()==32767 produces 11.
               Original 0x40d9f7 then reads 0xd924 (starfield X), beyond the
               eleven weapon records. Reject that roll; preserve every valid
               roll and its RNG order without inventing a twelfth weapon. */
            while(weapon==11 || !w->loadout->weapons[weapon].enabled) {
                weapon=blaze_random_next(w->random)*10/32767+1;s[11]=weapon;
            }
            s[11]=weapon;s[9]=0;s[10]=(uint32_t)(age/4-5);move(s);
            if(!draw(w,s,weapon,256,-1,0))return blocked(error,capacity,"draw capacity");
            if(blaze_signed(s[6])>320)s[4]=0;
            if(!label(w,UINT32_MAX-1,(int)weapon,blaze_signed(s[5])-w->camera_x+32,
                blaze_signed(s[6])-w->camera_y+16))return blocked(error,capacity,"draw capacity");
            if(blaze_player_contact(w,s,0)) {blaze_sound(w,BLAZE_GET);blaze_player_weapon(&w->player,w->loadout,weapon);s[4]=0;}
        }
    } else if(type==46 && w->loadout->mode==4) {
        const BlazeSprite *sprite=blaze_level_sprite(w->level,s[0]);
        if(!label(w,UINT32_MAX,blaze_signed(s[11]),blaze_signed(s[5])+sprite->width/2-w->camera_x,
            blaze_signed(s[6])+sprite->visible_height/2-w->camera_y))return blocked(error,capacity,"draw capacity");
    }
    if((s[0x44/4]&0x10) && (blaze_signed(s[5])>=w->camera_x+320 || blaze_signed(s[7])<=w->camera_x ||
        blaze_signed(s[6])>=w->camera_y+240 || blaze_signed(s[8])<=w->camera_y))s[4]=0;
    if(blaze_signed(s[0x60/4])>0)--s[0x60/4];
    if(blaze_signed(s[0x58/4])<100 && blaze_signed(s[0x50/4])<0) {
        s[0x58/4]=100;
        if(s[0x44/4]&0x800) {
            w->player.words[0x58/4]+=s[0x60/4];
            if(s[0x44/4]&4) {
                unsigned i,h=0;for(i=0;i<256;++i)if(s==w->actors[i]) {h=i+1;break;}
                if(!blaze_actor_attach(w,h,0,0,14,s[0x60/4],0))return blocked(error,capacity,"boss death");
            } else if(!blaze_actor_spawn(w,s+5,blaze_random_next(w->random)*2/32767==0?27:41,s[0x60/4],0))
                return blocked(error,capacity,"death explosion");
        }
        if(!(s[0x44/4]&0x200))s[4]=0;
    }
    if(blaze_signed(--s[3])<1) {if(s[0x44/4]&0x800)++w->player.words[0x94/4];s[4]=0;}
    ++s[0x54/4];return 1;
}
