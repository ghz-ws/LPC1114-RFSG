#include "mbed.h"
#include "stdio.h"

BufferedSerial uart0(P1_7, P1_6,115200);  //TX, RX
SPI spi(P0_9, P0_8, P0_6);    //mosi, miso, sclk
DigitalOut le1(P0_7);
DigitalOut le2(P0_11);
DigitalIn ld1(P0_4);
DigitalIn ld2(P0_5);

DigitalOut sel(P0_0);   //CLK sel

BusOut att0(P1_5,P1_4,P1_2,P1_1,P1_0);
BusOut att1(P1_8,P1_9,P0_1,P0_2,P0_3);

//reg calc from freq
const uint8_t buf_size=9;
void buf_read(uint8_t num); //uart read func.
char read_buf[buf_size];    //uart read buf
void buf2val();             //buf to vals change func. return to 'freq' and 'pha' global var 
const uint8_t fpfd=25;
const uint16_t nmod=25000;  //fpfd/spacing(0.001MHz)
uint32_t freq;              //kHz
int8_t ampl;               //means att state

//spi write
void spi_send(uint8_t ch,uint32_t in);
void pll_set(uint8_t ch,uint32_t freq);
uint8_t k;
uint32_t data[6]={0,0,0x4E42,0x4B3,0,0x580005};
char local_buf[2];

int main(){
    sel=0;      //0=int, 1=ext
    le1=1;
    le2=1;
    att0=0;
    att1=0;
    spi.format(8,0);   //spi mode setting. 2byte(16bit) transfer, mode 0
    while (true){
        for(k=1;k<=2;++k){
            buf_read(buf_size);//uart buf read
            buf2val();
            pll_set(k,freq);
            if(ampl>=31)ampl=31;
            if(ampl<0)ampl=0;
            if(k==1)att0=~ampl;
            else att1=~ampl;
        }
        //lock detect
        if(ld1==1)local_buf[0]='1';
        else local_buf[0]='0';
        thread_sleep_for(1);
        if(ld2==1)local_buf[1]='1';
        else local_buf[1]='0';
        uart0.write(local_buf,2);
    }
}

//uart char number read func.
void buf_read(uint8_t num){
    char local_buf[1];
    uint8_t i;
    for (i=0;i<num;++i){
        uart0.read(local_buf,1);
        read_buf[i]=local_buf[0];
    }
}

//buf to val change func.
void buf2val(){
    uint8_t i,j;
    uint32_t pow10;
    freq=0;
    ampl=0;
    for(i=0;i<7;++i){
        pow10=1;
        for(j=0;j<6-i;++j){
            pow10=10*pow10;
        }
        freq=freq+(read_buf[i]-48)*pow10;
    }
    for(i=0;i<2;++i){
        pow10=1;
        for(j=0;j<1-i;++j){
            pow10=10*pow10;
        }
        ampl=ampl+(read_buf[i+7]-48)*pow10;
    }
}

void spi_send(uint8_t ch,uint32_t in){
    uint8_t buf;
    if(ch==1)le1=0;
    else le2=0;
    buf=(in>>24)&0xff;
    spi.write(buf);
    buf=(in>>16)&0xff;
    spi.write(buf);
    buf=(in>>8)&0xff;
    spi.write(buf);
    buf=(in>>0)&0xff;
    spi.write(buf);
    if(ch==1)le1=1;
    else le2=1;
}

void pll_set(uint8_t ch,uint32_t freq){
    uint8_t i,j;
    uint8_t ndiv;
    uint16_t nint,nmodulus,nfrac_i,r,a,b;
    float freq_f,temp,nfrac;
    if(freq>4400000)freq=4400000;
    if(freq<35)freq=35;
    i=0;
    ndiv=2;
    freq_f=(float)freq;
    freq_f=freq_f/1000; //change MHz unit
    temp=4400/freq_f;   //for nint calc.
    while(1){
        temp=temp/2;
        i++;
        if(temp<2)break;
    }
    for(j=0;j<i-1;++j){
        ndiv=ndiv*2;
    }
    nint=freq*ndiv/fpfd/1000;
    temp=freq_f*ndiv/fpfd;  //for nfrac calc
    nfrac=nmod*(temp-nint);
    nfrac_i=(uint16_t)round(nfrac);

    //calc gcd by euclid algorithm
    a=nmod;
    b=nfrac_i;
    r=a%b;
    while(r!=0){
        a=b;
        b=r;
        r=a%b;
    }
    
    //calc nfrac and nmodulus
    nfrac_i=nfrac_i/b;
    nmodulus=nmod/b;
    if(nmodulus==1)nmodulus=2;
    
    //calc reg.
    data[0]=0;
    data[1]=0;
    data[4]=0;
    data[0]=(nint<<15)+(nfrac_i<<3)+0;
    data[1]=(1<<27)+(1<<15)+(nmodulus<<3)+1;
    data[4]=(1<<23)+(i<<20)+(200<<12)+(1<<5)+(3<<3)+4;
    
    //spi send
    for(i=0;i<5;++i){
        spi_send(ch,data[5-i]);
        thread_sleep_for(3);
    }
    thread_sleep_for(20);
    spi_send(ch,data[0]);
}
