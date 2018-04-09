#ifndef FLARM_CODEC

#define FLARM_CODEC

#define FLARM_KEY1 { 0xe43276df, 0xdca83759, 0x9802b8ac, 0x4675a56b, \
                     0xfc78ea65, 0x804b90ea, 0xb76542cd, 0x329dfa32 }
#define FLARM_KEY2 0x045d9f3b
#define FLARM_KEY3 0x87b562f4

typedef struct {
    /********************/
    unsigned int addr:24;
    unsigned int magic:8;
    /********************/
    int vs:10;
    unsigned int _unk0:3;
    unsigned int stealth:1;
    unsigned int no_track:1;
    unsigned int _unk1:1;
    unsigned int gps:12;
    unsigned int type:4;
    /********************/
    unsigned int lat:19;
    unsigned int alt:13;
    /********************/
    unsigned int lon:20;
    unsigned int _unk2:10;
    unsigned int vsmult:2;
    /********************/
    int8_t ns[4];
    int8_t ew[4];
    /********************/
} flarm_packet;

#endif
