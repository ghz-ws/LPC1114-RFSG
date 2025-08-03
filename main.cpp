#include "mbed.h"

BufferedSerial uart0(P1_7, P1_6,9600);
SPI spi(P0_9, P0_8, P0_6);    //mosi, miso, sclk
DigitalOut le1(P0_7);
DigitalOut le2(P0_11);
DigitalIn ld1(P0_4);
DigitalIn ld2(P0_5);
DigitalOut sel(P0_0);   //CLK sel
BusOut att0(P1_5,P1_4,P1_2,P1_1,P1_0);
BusOut att1(P1_8,P1_9,P0_1,P0_2,P0_3);

//CS control
void spi_send(uint32_t in);

//PLL control
const uint8_t fpfd=25;
const uint16_t nmod=25000;  //fpfd/spacing(0.001MHz)
uint32_t freq;              //kHz
int8_t ampl;               //means att state
void pllset(uint8_t ch,uint32_t freq,uint8_t att);
uint8_t k;
uint32_t data[6]={0,0,0x4E42,0x4B3,0,0x580005};
char local_buf[2];
uint8_t locked1,locked2;
uint8_t att;
uint8_t buf;

//parser
char char_read();
float char2flac();
void parser();
float pars[4];
uint32_t a,b,c,d;

int main(){
    sel=0;      //0=int, 1=ext
    le1=1;
    le2=1;
    att0=0;
    att1=0;
    spi.format(8,0);   //spi mode setting. 2byte(16bit) transfer, mode 2
    thread_sleep_for(100);

    while (true){
        buf=char_read();
        if(buf=='S'){
            parser();
        }else if(buf=='?'){
            locked1=ld1;
            locked2=ld2;
            printf("%d,%d,%04d,%04d\n\r",locked1,locked2,0,0);
        }
        a=(uint32_t)pars[0];
        b=(uint32_t)pars[1];
        c=(uint32_t)pars[2];
        d=(uint32_t)pars[3];

        if(a==5){
            pllset(1,b,c);
        }else if(a==6){
            pllset(2,b,c);
        }
    }
}

void spi_send(uint32_t in){
    uint8_t buf;
    buf=(in>>24)&0xff;
    spi.write(buf);
    buf=(in>>16)&0xff;
    spi.write(buf);
    buf=(in>>8)&0xff;
    spi.write(buf);
    buf=(in>>0)&0xff;
    spi.write(buf);
}

void pllset(uint8_t ch,uint32_t freq, uint8_t att){
    uint8_t i,j,ndiv,att_buf;
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
        if(ch==1)le1=0;
        else le2=0;
        spi_send(data[5-i]);
        thread_sleep_for(3);
        if(ch==1)le1=1;
        else le2=1;
    }
    thread_sleep_for(20);
    if(ch==1)le1=0;
    else le2=0;
    spi_send(data[0]);
    if(ch==1)le1=1;
    else le2=1;

    //att
    if(ch==1)att0=~att;
    else att1=~att;
}

char char_read(){
    char local_buf[1];          //local buffer
    uart0.read(local_buf,1);    //1-char read
    return local_buf[0];        //return 1-char
}
float char2flac(){
    char temp[1],local_buf[20];          //local buffer
    uint8_t i;
    for(i=0;i<sizeof(local_buf);++i) local_buf[i]='\0'; //init local buf
    i=0;
    while(true){
        temp[0]=char_read();
        if(temp[0]==',') break; //',' is delimiter
        local_buf[i]=temp[0];
        ++i;
    }
    return atof(local_buf);
}
void parser(){
    uint8_t i=0;
    pars[i]=char2flac();
    ++i;
    pars[i]=char2flac();
    ++i;
    pars[i]=char2flac();
    ++i;
    pars[i]=char2flac();
}
