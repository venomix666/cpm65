/****************************************************/
/*      SPECI-SPICE  by prof. dr. Samir Ribiæ       */
/*      University of Sarajevo, Bosnia Hercegovina  */
/*                     2016                         */
/****************************************************/

#define NULL (void *) 0
#define TRUE 1
#define FALSE 0
#define TINYINT unsigned char
#define BOOL unsigned char
#define NAMELEN 8

#define TOTMODELPARS 8
#include <math.h>
#include <string.h>
#include <ctype.h>

enum Sourcekind {AC,DC};
#define SCREENWID 51
typedef  struct dev {
    unsigned char type;
    unsigned char name[NAMELEN];
    union x {
        struct devparst {
            TINYINT port[6];
            float mainparam[8];
            TINYINT modelindex,refdev1,refdev2;
        } devpars;
        struct modelparst {
            TINYINT modelkind;
            TINYINT parindex[TOTMODELPARS];
            float parvalue[TOTMODELPARS];
        } modelpars;
        struct printvarst {
            char str[SCREENWID];
        } printvars;
        struct subcktportst {
            TINYINT modelindex;
            TINYINT port1[SCREENWID];
        } subcktports;
    } d;
} device;

typedef  struct  {
    unsigned char type;
    TINYINT extravarsDC,extravarsAC,extravarsTran,nonlinear;
    void  *initdevice;
    void  *updatedevice;
    char * parsetempl;
} devicetemplate;

TINYINT totaldevices,nlcount,startsubcktnode;
float temperature,Vt,timestep,currenttime,frequency,omega;
device * devices;
BOOL subcktmode;
char * circuittitle,* inputentry;
enum {DCMODE,ACMODE,TRANMODE} analyse;

#include "platform.inc"
#ifdef LEVEL3
#define DEVICES 15
#define LEVEL2
#define LEVEL1
#else
#ifdef LEVEL2
#define DEVICES 15
#define LEVEL1
#else
#ifdef LEVEL1
#define DEVICES 9
#else
#define DEVICES 3
#endif
#endif
#endif

#ifdef LEVEL3
FILE * inputfile;
#endif

char  membuf[MEMLEN];
/**********************************/
/*   STANDARD LIBRARY FUNCTIONS   */
/**********************************/

float min(float a, float b) {
    return (a<b?a:b);
}
BOOL bothzero(float * a,float * b) {
    return (*a==0.0 && *b==0.0);
}

BOOL lessthanzero(float * a) {
    return *a <0.0;
}

BOOL lessorequalthanzero(float * a) {
    return *a <=0.0;
}

BOOL greaterthanzero(float * a) {
    return *a >0.0;
}

void setzero(float * x) {
    *x=0.0;
}

void ftoa2 (float x,TINYINT prec, char * str) {
    /* converts a floating point number to an ascii string */
    /* x is stored into str, which should be at least 30 chars long */
    static int ie, i, ndig,k;
    static float y;
    ndig = ( prec<0) ? 7 : (prec > 22 ? 23 : prec+1);

    /* print in e format unless last arg is 'f' */
    ie = 0;
    /* if x negative, write minus and reverse */
    if ( lessthanzero(&x))  {
        *str++ = '-';
        x = -x;
    }

    /* put x in range 1 <= x < 10 */
    if (greaterthanzero(&x)) while (x < 1.0)  {
            x *= 10.0;
            ie--;
        }
    while (x >= 10.0)  {
        x = x/10.0;
        ie++;
    }

    /* round. x is between 1 and 10 and ndig will be printed to
       right of decimal point so rounding is ... */
    for (y = i = 1; i < ndig; i++)
        y = y/10.;
    x += y/2.0;
    if (x >= 10.0) {
        x = 1.0;    /* repair rounding disasters */
        ie++;
    }
    for (i=0; i < ndig; i++) {
        y = trunc(x);
        k = y;

        *str++ = k + '0';
        if (i == 0) { /* where is decimal point */
            *str++ = '.';
        }
        x -= y;
        x *= 10.0;
    }

    *str++ = 'E';
    if (ie < 0)
    {
        ie = -ie;
        *str++ = '-';
    }
    else
        *str++ = '+';
    *str++ = ie/10 + '0';
    *str++ = ie%10 + '0';
    *str = '\0';
    return;
}


void err(char * s) {
    printstr("Error in ");
    puts(s);
}



void printfp (float f) {
    static char s[12];
    ftoa2(f,4, s);
    printstr(s);
}



void uitoa(unsigned int value, char* string) {
    static unsigned char index, i;

    index = 6;
    i = 0;

    do {
        string[--index] = '0' + (value % 10);
        value /= 10;
    } while (value != 0);

    do {
        string[i++] = string[index++];
    } while ( index < 6 );

    string[i] = 0; /* string terminator */
}

void printint (TINYINT n) {
    static char s[6];
    uitoa(n,s);
    printstr(s);
}

BOOL  testmemory(unsigned int request) {
    if (request>MEMLEN-(inputentry-membuf)-128) {
        puts("memory");
        return FALSE;
    }
    else
        return TRUE;
}

#define PI 3.141592654
#define TWOPI 6.283185307
#define PIHALF 1.570796327
#define LN2 0.6931471806
#define LN10 2.302585093
#define PI180 0.01745329252
#ifdef LEVEL1
float  radian(float degree) {
    return degree*PI180;
}

float  degree(float rad1) {
    return rad1/PI180;
}
float Atan2(float y, float x) {
    static float absx, absy, val;
    if (bothzero(&x,&y)) {
        return 0.0;
    }
    absy = lessthanzero(&y) ? -y : y;
    absx = lessthanzero(&x) ? -x : x;
    if (absy - absx == absy) {
        /* x negligible compared to y */
        return lessthanzero(&y) ? -PIHALF : PIHALF;
    }
    if (absx - absy == absx) {
        /* y negligible compared to x */
        setzero(&val);
    }
    else
        val = atan(y/x);
    if (greaterthanzero(&x)) {
        /* first or fourth quadrant; already correct */
        return val;
    }
    if (lessthanzero(&y)) {
        /* third quadrant */
        return val - PI;
    }
    return val + PI;
}
#endif
/**********************************/
/*   COMPLEX NUMBERS FUNCTIONS    */
/**********************************/
float * RealSystem;
float * OpSolution;
TINYINT totalcolumns,totalnodes;

#ifdef LEVEL1
typedef struct {
    float re,im;
} complex;
complex * ComplexSystem;  /* Main solution matrix */
complex * AcSolution;


enum {REAL,COMPLEX} calcmode;


void polartorect(float magnitude, float phase, float * valuere, float * valueim) {
    *valuere=magnitude*cos(phase);
    *valueim=magnitude*sin(phase);
}

void complexAdd (complex * result,complex * add1,complex * add2) {
    result->re = add1->re+add2->re;
    result->im = add1->im+add2->im;
}

void complexSub (complex * result,complex * sub1,complex * sub2) {
    result->re = sub1->re-sub2->re;
    result->im = sub1->im-sub2->im;
}

void complexMul (complex * result,complex * mul1,complex * mul2) {
    result->re = mul1->re*mul2->re - mul1->im*mul2->im;
    result->im = mul1->re*mul2->im + mul1->im*mul2->re;
}

void complexDiv (complex * result,complex * div1,complex * div2) {
    static float denom;
    if (bothzero(&(div2->re),&(div2->im))) {
        result->re=1e38;
        result->im=1e38;
        return;
    }
    if (bothzero(&(div1->im),&(div2->im))) {
        result->re=div1->re/div2->re;
        setzero(&(result->im));
        return;
    }
    if (div2->re==0.0) {
        result->re=div1->im/div2->im;
        result->im=-div1->re/div2->im;
        return;
    }
    if (div2->im==0.0) {
        result->re=div1->re/div2->re;
        result->im=div1->im/div2->re;
        return;
    }
    denom=div2->re/div2->im+div2->im/div2->re;
    result->re=(div1->re/div2->im+div1->im/div2->re)/denom;
    result->im=(div1->im/div2->im-div1->re/div2->re)/denom;
}
#endif

/**********************************/
/*   PARAMETER PARSING FUNCTIONS  */
/**********************************/

char * getname(char * s,char * buffer,int length) {
    static TINYINT i,j;
    /* skip white space */
    for (i=0; isspace(s[i]); i++);
    for (j=0; isalnum(s[i]); i++,j++) {
        if (j<length) {
            buffer[j]=s[i];
        }
    }
    if (j<length)
        buffer[j]=0;
    return s+i;

}

char * getunsignedint(char * s,TINYINT * value) {
    static int i;
    static TINYINT val;
    /* skip white space */
    for (i=0; isspace(s[i]); i++);
    if (!isdigit(s[i])) return NULL;
    for (val = 0; isdigit(s[i]); i++) {
        val=val*10+(s[i]-'0');
    }
    *value=val;
    return s+i;
}


char * getfloat(char * s,float * val) {
    static float value, fraction;
    static char iexp;
    static char sign,signexp;
    /*Skip leading blanks*/
    while (*s==' ') s++;

    /*Get the sign*/
    if (*s == '-')    {
        sign=1;
        s++;
    }
    else    {
        sign=0;
        if (*s == '+') s++;
    }
    if (!isdigit(*s)) return NULL;

    /*Get the integer part*/
    for (value=0.0; isdigit(*s); s++)    {
        value=10.0*value+(*s-'0');
    }
    /*Get the fraction */
    if (*s == '.')    {
        s++;
        for (fraction=0.1; isdigit(*s); s++)  {
            value+=(*s-'0')*fraction;
            fraction*=0.1;
        }
    }
    iexp=0;
    /*Finally, the exponent (not very efficient, but enough for now*/
    if (*s=='E') {
        s++;
        if (*s=='-') {
            signexp=-1;
            s++;
        }
        else {
            signexp=1;
        }
        for (iexp = 0; isdigit(*s); s++) {
            iexp=10*iexp+(*s-'0') ;
        }
        if (signexp==-1) {
            iexp=-iexp;
        }
    }
    /* SI prefixes SPICE mode */
    switch (*s) {
    case 'F':
        iexp-=15;
        break;
    case 'P':
        iexp-=12;
        break;
    case 'N':
        iexp-=9;
        break;
    case 'U':
        iexp-=6;
        break;
    case 'M':
        iexp+=(s[1]=='E' && s[2]=='G')? 6 :-3;
        break;
    case 'K':
        iexp+=3;
        break;
    case 'G':
        iexp+=9;
        break;
    case 'T':
        iexp+=12;
        break;
    }

    while(iexp!=0)  {
        if(iexp<0)   {
            value*=0.1;
            iexp++;
        }
        else   {
            value*=10.0;
            iexp--;
        }
    }

    if(sign) {
        value*=-1.0;
    }
    /* Ignore unit name */
    for (; isalpha(*s); s++);
    *val = value;
    return(s);
}
char * expect(char * s,char c) {
    for (; isspace(*s); s++);
    if (*s != c)
        s=NULL;
    else
        s++;
    return s;
}



int  finddevice(char * name) {
    register TINYINT i;
    for (i=0; i<totaldevices; i++) {
        if (!strncmp(devices[i].name,name,NAMELEN)) {
            return i;
        }
    }
    return -1;
}

char * reftodevice (char * inputline,device * self, TINYINT reqtype,TINYINT refdevind,char * subcktname) {
    static char refdev[NAMELEN];
    static char * s;
    static int pos;
    s=inputline;
    s=getname(s,refdev,NAMELEN);
    strncat(refdev,subcktname,NAMELEN-strlen(refdev));
    if (s) {
        pos=finddevice(refdev);
        if (pos>=0) {
            if (refdevind==1) {
                self->d.devpars.refdev1=pos;
            }
            if (refdevind==2) {
                self->d.devpars.refdev2=pos;
            }
            if (devices[pos].type !=reqtype) {
                s=NULL;
            }
            return s;
        }
        else s=NULL;
    }
    return(s);
}


/******************************************/
/*   VOLTAGE AND CURRENT SOURCE FUNCTIONS */
/******************************************/

char * sourcekindnames[6]= {"AC","DC","EXP","PULSE","SFFM","SIN"};
TINYINT sourceparcount[6]= {2,1,6,7,5,6};
char * Sources (char * inputline,device * self) {
    static char sourcetype[NAMELEN];
    static char * s;
    static TINYINT numparams,i;
    static int pos;
    s=inputline;
    for (; isspace(*s); s++);
    if (isdigit(*s) || *s=='-')
        numparams=pos=1;
    else {
        pos=-1;
        s=getname(s,sourcetype,NAMELEN);
        for (i=0; i<6; i++) {
            if (!strncmp(sourcetype,sourcekindnames[i],NAMELEN)) {
                pos=i;
                numparams=sourceparcount[pos];
                break;
            }
        }
        if (pos==-1) {
            s=NULL;
        }
    }
    self->d.devpars.modelindex=pos;
    if (pos>1) {
        s=expect(s,'(');
    }
    for (i=0; i<numparams; i++) {
        if (s) {
            for (; isspace(*s); s++);
            if (isdigit(*s) || *s=='-')
                s=getfloat(s,&(self->d.devpars.mainparam[i]));
            else
                break;
        }
    }
    if (s && pos >1) {
        s=expect(s,')');
    }
    return s;
}
#ifdef LEVEL1

float  pulsesource(float * params) {
    static float ut,u1,u2,td,tr,tf,pw,per,t,t1,t2;
    u1=params[0];
    u2=params[1];
    td=params[2];
    tr=params[3];
    tf=params[4];
    pw=params[5];
    per=params[6];
    t1=tr+td;
    t2=t1+pw+tf;
    t=currenttime;
    while (t>td+per) {
        t=t-per;
    }
    if (t < td) { /* before pulse */
        ut = u1;
    }
    else if (t < t1) { /* rising edge */
        ut = u1 + (u2 - u1) / tr * (t - td);
    }
    else if (t < td+tr+pw) { /* during full pulse */
        ut = u2;
    }
    else if (t < t2) { /* falling edge */
        ut = u2 + (u1 - u2) / tf * (t - (t2 - tf));
    }
    else { /* after pulse */
        ut = u1;
    }
    return ut;
}

float  sffmsource(float * params) {
    static float voff,vampl,fc,mod,fm;
    voff=params[0];
    vampl=params[1];
    fc=params[2];
    mod=params[3];
    fm=params[4];
    return voff+vampl*sin(TWOPI*fc*currenttime+mod*sin(TWOPI*fm*currenttime));
}

float  expsource(float * params) {
    static float u1,u2,t1,tr,t2,tf,t,ut;
    u1=params[0];
    u2=params[1];
    t1=params[2];
    tr=params[3];
    t2=params[4];
    tf=params[5];
    t=currenttime;
    if (t <= t1) { /* before pulse */
        ut = u1;
    }
    else { /* rising edge */
        ut = u1 + (u2 - u1) * (1.0 - exp (-(t - t1) / tr));
        if (t > t2) {
            ut -= (u2 - u1) * (1.0 - exp (-(t - t2) / tf));
        }
    }
    return ut;
}

float  sinsource(float * params) {
    static float voff,vampl,freq,td,df,phase,ut ;
    voff=params[0];
    vampl=params[1];
    freq=params[2];
    td=params[3];
    df=params[4];
    phase=params[5];
    if (currenttime <= td) { /* before damp */
        ut = voff+vampl*sin(PI180*phase);
    }
    else {
        ut = voff+vampl*sin(TWOPI*freq*(currenttime-td)+phase/360.0)*
             exp(-(currenttime-td)*df);
    }
    return ut;
}
#endif
/******************************************/
/*   MODEL REFERENCE AND SETUP FUNCTIONS  */
/******************************************/


typedef struct modelparamt {
    char * paramname;
    char * parval;
} modelparam;

/* ZCC does not allow initializators of floating point numbers */

#define DIODEPARSLEN 5
#define FETPARSLEN 7
#define BIPOLARPARSLEN 5
#define MOSFETPARSLEN 10

#ifdef LEVEL2
modelparam diodepars[DIODEPARSLEN]= {{"IS","1E-14"},{"ISR","0"},{"N","1"},{"NR","2"},{"TT","0"}
} ;
modelparam FETpars[FETPARSLEN]= {{"IS","1E-14"},{"ISR","0"},{"N","1"},{"NR","2"},{"VTO","-2"},{"BETA","1E-4"},
    {"LAMBDA","0"}
} ;
modelparam Bipolarpars[BIPOLARPARSLEN]= {{"IS","1E-16"},{"NF","1"},{"NR","1"}, {"BF","100"},{"BR","1"}
} ;
modelparam Mosfetpars[MOSFETPARSLEN]= {{"IS","1E-14"},{"N","1"},{"L","1E-4"},{"LD","0"},{"KP","2E-5"},
    {"W","1E-4"},{"LAMBDA","0"},{"GAMMA","0"},{"VTO","0"},{"PHI","6E-1"}
} ;

typedef struct modeltypes_t {
    char * name;
    char devcode;
    modelparam * parametertable;
    int ptablelen;
} modeltypes;


modeltypes mt[8]= {{"ZERO",' ',NULL,0},{"D",9,diodepars,DIODEPARSLEN},
    {"PNP",13,Bipolarpars,BIPOLARPARSLEN},{"NPN",13,Bipolarpars,BIPOLARPARSLEN},
    {"NJF",10,FETpars,FETPARSLEN},{"PJF",10,FETpars,FETPARSLEN},
    {"PMOS",12,Mosfetpars,MOSFETPARSLEN},{"NMOS",12,Mosfetpars,MOSFETPARSLEN}
};
#endif

#ifdef LEVEL2
char * reftomodel (char * inputline,device * self) {
    static char model[NAMELEN];
    static char * s;
    static int pos;
    s=inputline;
    s=getname(s,model,NAMELEN);
    if (s) {
        pos=finddevice(model);
        if (pos>=0) {
            if ((devices[pos].type !='#') ||
                    (mt[devices[pos].d.modelpars.modelkind].devcode != self->type)) {
                s=NULL;
            }
            self->d.devpars.modelindex=pos;
            return s;
        }
    }
    else
        return NULL;
}
#endif

/******************************************/
/*   TEMPERATURE FUNCTIONS                */
/******************************************/



void  AdjustTemperature(float t) {
    temperature=t;
    Vt=1.38064852e-23*t/1.60217662e-19;
}

#ifdef LEVEL2
char *  DoTempCommand(char * inputline) {
    static float t;
    static char * s;
    s=inputline;
    s=getfloat(s,&t);
    if (s) AdjustTemperature(t+273.15);
    return s;
}
#endif

/*******************************************************/
/*   DEVICE INITIALIZATION AND CALCULATION FUNCTIONS   */
/*******************************************************/
/*
#define NODE_1 self->d.devpars.port[0]
#define NODE_2 self->d.devpars.port[1]
#define NODE_3 self->d.devpars.port[2]
#define NODE_4 self->d.devpars.port[3]

#define VSRC_1 self->d.devpars.port[4]
#define VSRC_2 self->d.devpars.port[5]


*/

#define NODE_1 0
#define NODE_2 1
#define NODE_3 2
#define NODE_4 3
#define VSRC_1 4
#define VSRC_2 5

#define NODE_1_TO_NODE_1 00
#define NODE_2_TO_NODE_1 10
#define NODE_3_TO_NODE_1 20
#define NODE_4_TO_NODE_1 30
#define VSRC_1_TO_NODE_1 40
#define VSRC_2_TO_NODE_1 50

#define NODE_1_TO_NODE_2 01
#define NODE_2_TO_NODE_2 11
#define NODE_3_TO_NODE_2 21
#define NODE_4_TO_NODE_2 31
#define VSRC_1_TO_NODE_2 41
#define VSRC_2_TO_NODE_2 51

#define NODE_1_TO_NODE_3 02
#define NODE_2_TO_NODE_3 12
#define NODE_3_TO_NODE_3 22
#define NODE_4_TO_NODE_3 32
#define VSRC_1_TO_NODE_3 42
#define VSRC_2_TO_NODE_3 52

#define NODE_1_TO_NODE_4 03
#define NODE_2_TO_NODE_4 13
#define NODE_3_TO_NODE_4 23
#define NODE_4_TO_NODE_4 33
#define VSRC_1_TO_NODE_4 43
#define VSRC_2_TO_NODE_4 53

#define NODE_1_TO_VSRC_1 04
#define NODE_2_TO_VSRC_1 14
#define NODE_3_TO_VSRC_1 24
#define NODE_4_TO_VSRC_1 34
#define VSRC_1_TO_VSRC_1 44
#define VSRC_2_TO_VSRC_1 54

#define NODE_1_TO_VSRC_2 05
#define NODE_2_TO_VSRC_2 15
#define NODE_3_TO_VSRC_2 25
#define NODE_4_TO_VSRC_2 35
#define VSRC_1_TO_VSRC_2 45
#define VSRC_2_TO_VSRC_2 55

/* To reduce generated code on Z80, parameters of these functions are indices in dev.devparts.ports array */
device * ref;
void  setref(device * rref) {
    ref=rref;
}

void addAComplex(TINYINT rowpluscolumnnode,float realvalue,float imaginarvalue) {
    static TINYINT rownode,columnnode,row,column;
    static int rp;
    rownode=rowpluscolumnnode/10;
    columnnode=rowpluscolumnnode%10;

    row=ref->d.devpars.port[rownode];
    column=ref->d.devpars.port[columnnode];
    if (row>0)
        row-- ;
    else
        return;
    if (column>0)
        column-- ;
    else
        return;
    rp=(totalcolumns+1)*row+column;
#ifdef LEVEL1
    if (calcmode==COMPLEX) {
        ComplexSystem[rp].re+=realvalue;
        ComplexSystem[rp].im+=imaginarvalue;
    }
    else
        RealSystem[rp]+=realvalue;
#else
    RealSystem[rp]+=realvalue;

#endif
}
#ifdef LEVEL1
void addAComplexV(TINYINT rowcolumn,complex * value) {
    addAComplex(rowcolumn,value->re,value->im);
}

void addAComplexMinusV(TINYINT rowcolumn,complex * value) {
    addAComplex(rowcolumn,-value->re,-value->im);
}
#endif
void addAReal(TINYINT rowcolumn,float value) {
    addAComplex(rowcolumn,value,0.0);
}

void addARealV(TINYINT rowcolumn,float * value) {
    addAReal(rowcolumn,*value);
}

void addARealMinusV(TINYINT rowcolumn,float * value) {
    addAReal(rowcolumn,-(*value));
}

void addAImaginary(TINYINT rowcolumn,float value) {
    addAComplex(rowcolumn,0.0,value);
}

void addAImaginaryV(TINYINT rowcolumn,float * value) {
    addAImaginary(rowcolumn,*value);
}

void addAImaginaryMinusV(TINYINT rowcolumn,float * value) {
    addAImaginary(rowcolumn,-(*value));
}

void  addAOne1(TINYINT rowcolumn) {
    addAReal(rowcolumn,1.0);
}

void  addAMinusOne1(TINYINT rowcolumn) {
    addAReal(rowcolumn,-1.0);
}

#define addAOne(X,Y) addAOne1(X)
#define addAMinusOne(X,Y) addAMinusOne1(X)


#define addAZero(X,Y) ;
#define addEZero(X,Y) ;
#define addIZero(X,Y) ;

void addE(TINYINT rownode,float realvalue,float imaginarvalue) {
    static TINYINT row;
    static int index;
    row=ref->d.devpars.port[rownode];
    if (row>0)
        row-- ;
    else
        return;
    index=(totalcolumns+1)*row+totalcolumns;
#ifdef LEVEL1
    if (calcmode==COMPLEX) {
        ComplexSystem[index].re+=realvalue;
        ComplexSystem[index].im+=imaginarvalue;
    }
    else
        RealSystem[index]+=realvalue;
#else
    RealSystem[index]+=realvalue;
#endif
}

#define addI addE
#define addIReal addEReal

void addEReal (TINYINT rownode,float realvalue) {
    addE(rownode,realvalue,0.0);
}

#ifdef LEVEL1
#define savestate(index,value) ref->d.devpars.mainparam[index]=value
void savestate1(TINYINT index, float  * value) {
    savestate(index,*value);
}

float  restorestate(TINYINT index) {
    return ref->d.devpars.mainparam[index];
}

float  realval(TINYINT node1) {
    static TINYINT row;
    row=ref->d.devpars.port[node1];
    if (row !=0)
        return OpSolution[row-1];
    else
        return 0;
}

float previousvoltage(TINYINT nodes,float pol) {
    return pol*(realval(nodes/10)-realval(nodes%10));
}
#endif
#ifdef LEVEL2
float  paramfrommodel(TINYINT paramindex) {
    static TINYINT m,i;
    static device * modeldef;
    static char * parval;
    static modeltypes * thismodel;
    static float result;
    m=ref->d.devpars.modelindex;
    modeldef=devices+m;
    for (i=0; i<TOTMODELPARS; i++) {
        if (modeldef->d.modelpars.parindex[i]==255) {
            break;
        }
        if (modeldef->d.modelpars.parindex[i]==paramindex) {
            return modeldef->d.modelpars.parvalue[i];
        }
    }
    thismodel=mt+devices[m].d.modelpars.modelkind;
    if (paramindex>=thismodel->ptablelen) {
        return 0.0;
    }
    parval=thismodel->parametertable[paramindex].parval;
    getfloat(parval,&result);
    return result;
}
#endif
#ifdef LEVEL1

float dcsources() {
    static float valuere;
    switch (ref->d.devpars.modelindex) {
    case 0:
        setzero(&valuere);
        break;
    case 1:
        valuere=ref->d.devpars.mainparam[0];
        break;
    case 2:
        valuere=expsource(ref->d.devpars.mainparam);
        break;
    case 3:
        valuere=pulsesource(ref->d.devpars.mainparam);
        break;
    case 4:
        valuere=sffmsource(ref->d.devpars.mainparam);
        break;
    case 5:
        valuere=sinsource(ref->d.devpars.mainparam);
        break;
    default:
        setzero(&valuere);
    }
    return valuere;
}
#endif


union dvt {
#ifdef LEVEL1
    struct ct {
        float cap,y,v,g,i;
    } c;
#endif
#ifdef LEVEL2
    struct dt {
        float Is,Isr,N,Nr,nvt,nrvt,ef,er,Id,Uprev,Ucrit,gd,Ieq,Vd;
    } d;
#endif
#ifdef LEVEL1
    struct et {
        float g;
    } e;
    struct ft {
        float g;
        TINYINT r;
    } f;
    struct gt {
        float gain;
        TINYINT rr;
    } gg;
    struct ht {
        float g;
    } h;
#endif
    struct it {
        float valuere,valueim;
    } ii;
#ifdef LEVEL2
    struct jt {
        float Is,n,UgsCrit,UgdCrit,UgsPrev,UgdPrev,
              Ugd,Ugs,Uds,ggs,Igs,ggd,gds,Igd, Ugst,Ugdt,Ids,gm,b,beta,Vt0,l,
              pol,IeqG,IeqS,IeqD,c,p,Vtn,twoc,csquared,twobc,pUds;
    } j;
    struct kt {
        float l1,l2,k,a,y1,y2,y3,m,r12,v1,v2,Iold,oa;
        TINYINT refdev,*destports,*srcports;
    } kk;
#endif
#ifdef LEVEL1
    struct lt {
        float l,y,r,v,Iold;
    } ll;
#endif
#ifdef LEVEL2
    struct mt {
        float Isd,Iss,n,l,Kp,W,Ll,Ld,Vto,Phi,pol,Leff,beta,Ugd,Ugs,Ubd,Uds,
              Ubs,gtiny,Ibs,gbs,Ibd,gbd,arg,MOSdir,Upn,Sphi,Sarg,Uon,Utst,Ga,
              gm,gds,gmb,Vds,Udsat,Ids,IeqBD,IeqBS,IeqDS,b,SourceControl,
              DrainControl,UbsCrit,UbdCrit,UbsPrev,UbdPrev,Vtn,UtstSquaredHalf,Vdsgain
              ;
    } mm;
    struct qt {
        float Gcd,Ged,af,ar,Is,Nf,Nr,Bf,Br,Ied,Icd,afGed,arGcd,afm1Ged,
              arm1Gcd,pol,Ube,Ubc,UbeCrit,UbcCrit,UbePrev,UbcPrev,VtNf,VtNr;
    } q;
#endif
    struct rt {
        float r1,g1;
    } res;
#ifdef LEVEL2
    struct tt {
        float Tau,z,prevj1,prevj2,prev12,prev34,ot;
        complex explg,expminlg,cothlg,cschlg,expsum,expdiff,chelp,y11,y21;
    } t;
#endif
    struct vt {
        float valuere,valueim;
    } vs;

} dv;
#ifdef LEVEL1
void updatecapacitor() {
    savestate(1,previousvoltage(NODE_1_TO_NODE_2,1.0));
}
void initcapacitor() {
    dv.c.cap=ref->d.devpars.mainparam[0];
    switch (analyse) {
    case DCMODE:
        break;
    case TRANMODE:
        dv.c.v=restorestate(1);
        dv.c.g=dv.c.cap/timestep;   /* Euler method */
        dv.c.i=-dv.c.cap*dv.c.v/timestep;
        addARealV (NODE_1_TO_NODE_1,&dv.c.g);
        addARealV (NODE_2_TO_NODE_2, &dv.c.g);
        addARealMinusV (NODE_1_TO_NODE_2, &dv.c.g);
        addARealMinusV (NODE_2_TO_NODE_1, &dv.c.g);
        addIReal (NODE_1 , -dv.c.i);
        addIReal (NODE_2 , dv.c.i);
        break;

    case ACMODE:
        dv.c.y=omega*dv.c.cap;
        addAImaginaryV (NODE_1_TO_NODE_1,&dv.c.y);
        addAImaginaryV (NODE_2_TO_NODE_2, &dv.c.y);
        addAImaginaryMinusV (NODE_1_TO_NODE_2,&dv.c.y);
        addAImaginaryMinusV (NODE_2_TO_NODE_1, &dv.c.y);
        break;
    }
}
#endif

#ifdef LEVEL2
/* Compute critical voltage of pn-junction. */
float pnCriticalVoltage (float Iss, float Ute) {
    return Ute * log (Ute / sqrt(2.0) / Iss);
}

/* The function limits the forward pn-voltage for each DC iteration in
   order to avoid numerical overflows and thereby improve the
   convergence. */
float pnVoltage (float Ud, float Uold,float Ut, float Ucrit) {
    static float arg;
    if (Ud > Ucrit && fabs (Ud - Uold) > 2.0 * Ut) {
        if (greaterthanzero(&Uold)) {
            arg = (Ud - Uold) / Ut;
            if (greaterthanzero(&arg)) {
                Ud = Uold + Ut * (2.0 + log (arg - 2.0));
            }
            else {
                Ud = Uold - Ut * (2.0 + log (2.0 - arg));
            }
        }
        else Ud = lessthanzero(&Uold) ? Ut * log (Ud / Ut) : Ucrit;
    }
    else {
        if (lessthanzero(&Ud)) {
            arg = greaterthanzero(&Uold) ? -1.0 - Uold : 2.0 * Uold - 1.0;
            if (Ud < arg) Ud = arg;
        }
    }
    return Ud;
}



void  pnJunctionBIP (float Upn, float Iss, float Ute,float * I, float * g) {
    static float a,e;

    if (Upn < -3.0 * Ute) {
        a = 3.0 * Ute / (Upn * 2.718281828459);
        a = a*a*a;
        *I = -Iss * (1.0 + a);
        *g = Iss * 3.0 * a / Upn;
    }
    else {
        e = exp (min(Upn / Ute,40));
        *I = Iss * (e - 1.0);
        *g = Iss * e / Ute;
    }

}

#endif
#ifdef LEVEL2
void initdiode() {
    if (analyse!=ACMODE) {
        dv.d.Vd=previousvoltage(NODE_1_TO_NODE_2,1.0);
        dv.d.Is=paramfrommodel(0);
        dv.d.Isr=paramfrommodel(1);
        dv.d.N=paramfrommodel(2);
        dv.d.Nr=paramfrommodel(3);
        dv.d.nvt=dv.d.N*Vt;
        dv.d.nrvt=dv.d.Nr*Vt;
        dv.d.Uprev = restorestate(1);
        dv.d.Ucrit = pnCriticalVoltage (dv.d.Is, dv.d.nvt);
        dv.d.Vd = pnVoltage (dv.d.Vd, dv.d.Uprev, dv.d.nvt, dv.d.Ucrit);
        dv.d.Uprev = dv.d.Vd;
        savestate1(1,&dv.d.Uprev);
        dv.d.ef=exp(dv.d.Vd/dv.d.nvt);
        dv.d.er=exp(dv.d.Vd/dv.d.nrvt);
        dv.d.Id=dv.d.Is*(dv.d.ef-1.0)+dv.d.Isr*(dv.d.er-1.0);
        dv.d.gd=dv.d.Is*dv.d.ef/dv.d.nvt+dv.d.Isr*dv.d.er/dv.d.nrvt;
        dv.d.Ieq=dv.d.Id-(dv.d.gd)*dv.d.Vd;
        addIReal (NODE_2, dv.d.Ieq);
        addIReal (NODE_1, -dv.d.Ieq);
    }
    else {
        dv.d.gd=restorestate(0);
    }
    addARealV (NODE_2_TO_NODE_2, &dv.d.gd);
    addARealV (NODE_1_TO_NODE_1, &dv.d.gd);
    addARealMinusV (NODE_2_TO_NODE_1, &dv.d.gd);
    addARealMinusV (NODE_1_TO_NODE_2, &dv.d.gd);
    if (analyse==DCMODE) {
        savestate1(0,&dv.d.gd);
    }
}
#endif

#ifdef LEVEL1
void initvcvs() {
    dv.e.g=ref->d.devpars.mainparam[0];
    addARealV    (VSRC_1_TO_NODE_3, &dv.e.g);
    addAMinusOne (VSRC_1_TO_NODE_1, -1.0);
    addAOne (VSRC_1_TO_NODE_2, 1.0 );
    addARealMinusV    (VSRC_1_TO_NODE_4, &dv.e.g);
    addAZero    (NODE_3_TO_VSRC_1, 0);
    addAOne  (NODE_1_TO_VSRC_1 , 1.0);
    addAMinusOne (NODE_2_TO_VSRC_1 , -1.0);
    addAZero  (NODE_4_TO_VSRC_1,0);
    addAZero    (VSRC_1_TO_VSRC_1, 0);
    addEZero (VSRC_1, 0.0);
}

void initcccs() {
    dv.f.r=ref->d.devpars.refdev1;
    dv.f.g=ref->d.devpars.mainparam[0];
    ref->d.devpars.port[5]=devices[dv.f.r].d.devpars.port[4];
    addAOne  (NODE_1_TO_VSRC_1, 1.0);
    addAMinusOne  (NODE_2_TO_VSRC_1, -1.0);
    addAOne  (VSRC_1_TO_VSRC_1, 1.0);
    addARealMinusV  (VSRC_1_TO_VSRC_2, &dv.f.g);
    addEZero  (VSRC_1, 0.0);
}

void initccvs() {
    dv.gg.rr=ref->d.devpars.refdev1;
    dv.gg.gain=ref->d.devpars.mainparam[0];

    ref->d.devpars.port[5]=devices[dv.gg.rr].d.devpars.port[4];

    addAOne (VSRC_1_TO_NODE_1, 1.0);
    addAOne (NODE_1_TO_VSRC_1, 1.0);
    addAMinusOne (VSRC_1_TO_NODE_2, -1.0);
    addAMinusOne (NODE_2_TO_VSRC_1, -1.0);
    addARealMinusV (VSRC_1_TO_VSRC_2, &dv.gg.gain);
    addEZero (VSRC_1, 0.0);
}

void initvccs() {
    dv.h.g=ref->d.devpars.mainparam[0];
    addAMinusOne (VSRC_1_TO_NODE_3, -1.0);
    addAOne (VSRC_1_TO_NODE_4, 1.0);
    addAOne (NODE_1_TO_VSRC_1, 1.0);
    addAMinusOne (NODE_2_TO_VSRC_1, -1.0);
    addAReal (VSRC_1_TO_VSRC_1, 1.0/dv.h.g);
}
#endif



void initcurrentsource() {
#ifdef LEVEL1
    switch (analyse) {
    case DCMODE:
    case TRANMODE:
        dv.ii.valuere=dcsources();
        setzero(&dv.ii.valueim);
        break;
    case ACMODE:
        if (ref->d.devpars.modelindex==0) {
            polartorect(ref->d.devpars.mainparam[0],radian(ref->d.devpars.mainparam[1]),&dv.ii.valuere,&dv.ii.valueim);
        }
        else {
            setzero(&dv.ii.valuere);
            dv.ii.valueim=dv.ii.valuere;
        }
        break;
    }
    addI(NODE_1,-dv.ii.valuere,-dv.ii.valueim);
    addI(NODE_2,dv.ii.valuere,dv.ii.valueim);
#else
    addI(NODE_1,-ref->d.devpars.mainparam[0],0.0);
    addI(NODE_2,ref->d.devpars.mainparam[0],0.0);
#endif
}
#ifdef LEVEL2

void initJFET() {
    if (analyse !=ACMODE) {
        dv.j.Is=paramfrommodel(0);
        dv.j.n=paramfrommodel(2);
        dv.j.Vt0=paramfrommodel(4);
        dv.j.beta=paramfrommodel(5);
        dv.j.l=paramfrommodel(6);

        dv.j.Vtn=Vt*dv.j.n;
        dv.j.pol=1.0;
        if (devices[ref->d.devpars.modelindex].d.modelpars.modelkind==5) {
            dv.j.pol=-1.0;
        }
        /* pol=-1 for PJF */
        dv.j.Ugd = previousvoltage(NODE_2_TO_NODE_1, dv.j.pol);
        dv.j.Ugs = previousvoltage(NODE_2_TO_NODE_3, dv.j.pol);
        dv.j.UgdCrit = dv.j.UgsCrit = pnCriticalVoltage (dv.j.Is, dv.j.Vtn);
        dv.j.UgsPrev=restorestate(4);
        dv.j.UgdPrev=restorestate(5);

        dv.j.UgsPrev = dv.j.Ugs = pnVoltage (dv.j.Ugs, dv.j.UgsPrev, dv.j.Vtn, dv.j.UgsCrit);
        dv.j.UgdPrev = dv.j.Ugd = pnVoltage (dv.j.Ugd, dv.j.UgdPrev, dv.j.Vtn, dv.j.UgdCrit);
        savestate1(4,&dv.j.UgsPrev);
        savestate1(5,&dv.j.UgdPrev);
        pnJunctionBIP(dv.j.Ugs,dv.j.Is,dv.j.Vtn,&dv.j.Igs,&dv.j.ggs);
        pnJunctionBIP(dv.j.Ugd,dv.j.Is,dv.j.Vtn,&dv.j.Igd,&dv.j.ggd);
        dv.j.Uds = dv.j.Ugs - dv.j.Ugd;

        if (lessthanzero(&dv.j.Uds)) {
            dv.j.c = dv.j.Ugdt = dv.j.Ugd - dv.j.Vt0;
            dv.j.p = - 1.0;
        }
        else {
            dv.j.c = dv.j.Ugst = dv.j.Ugs - dv.j.Vt0;
            dv.j.p = 1.0;
        }
        dv.j.twoc=2.0*dv.j.c;
        dv.j.csquared=dv.j.c*dv.j.c;
        dv.j.pUds=dv.j.p*dv.j.Uds;
        if (lessorequalthanzero(&dv.j.c)) {
            setzero(&dv.j.Ids);
            setzero(&dv.j.gm);
            setzero(&dv.j.gds);
        }
        else {
            dv.j.b = dv.j.beta * (1.0 + dv.j.pUds*dv.j.l);
            dv.j.twobc=dv.j.twoc*dv.j.b;
            if (dv.j.c <= dv.j.pUds) {
                dv.j.Ids = dv.j.p*dv.j.b * dv.j.csquared;
                dv.j.gm  = dv.j.p* dv.j.twobc;
                dv.j.gds = dv.j.l * dv.j.beta * dv.j.csquared;
                if (lessthanzero(&dv.j.Uds)) {
                    dv.j.gds+=  dv.j.twobc;
                }
            }
            else {
                dv.j.Ids = dv.j.b * dv.j.Uds * (dv.j.twoc - dv.j.pUds);
                dv.j.gm  = dv.j.b * 2.0 * dv.j.Uds;
                dv.j.gds = dv.j.twobc ;
                if (dv.j.Uds >= 0.0) {
                    dv.j.gds -= dv.j.gm;
                }
                dv.j.gds+= dv.j.p*dv.j.l * dv.j.beta * dv.j.Uds * (dv.j.twoc - dv.j.pUds);
            }
        }
        dv.j.IeqG = dv.j.Igs - dv.j.ggs * dv.j.Ugs;
        dv.j.IeqD = dv.j.Igd - dv.j.ggd * dv.j.Ugd;
        dv.j.IeqS = dv.j.Ids - dv.j.gm * dv.j.Ugs - dv.j.gds * dv.j.Uds;

        addIReal (NODE_2, (-dv.j.IeqG - dv.j.IeqD) * dv.j.pol);
        addIReal (NODE_1, (dv.j.IeqD - dv.j.IeqS) * dv.j.pol);
        addIReal (NODE_3, (dv.j.IeqG + dv.j.IeqS) * dv.j.pol);
    }
    else {
        dv.j.ggs=restorestate(0);
        dv.j.ggd=restorestate(1);
        dv.j.gds=restorestate(2);
        dv.j.gm=restorestate(3);
    }
    addAReal (NODE_2_TO_NODE_2, dv.j.ggs + dv.j.ggd);
    addARealMinusV (NODE_2_TO_NODE_1, &dv.j.ggd);
    addARealMinusV (NODE_2_TO_NODE_3, &dv.j.ggs);
    addAReal (NODE_1_TO_NODE_2, -dv.j.ggd + dv.j.gm);
    addAReal (NODE_1_TO_NODE_1, dv.j.gds + dv.j.ggd);
    addAReal (NODE_1_TO_NODE_3, -dv.j.gm - dv.j.gds);
    addAReal (NODE_3_TO_NODE_2, -dv.j.ggs - dv.j.gm);
    addARealMinusV (NODE_3_TO_NODE_1, &dv.j.gds);
    addAReal (NODE_3_TO_NODE_3, dv.j.ggs + dv.j.gds + dv.j.gm);
    if (analyse==DCMODE) {
        savestate1(0,&dv.j.ggs);
        savestate1(1, &dv.j.ggd);
        savestate1(2,  &dv.j.gds);
        savestate1(3,  &dv.j.gm);
    }
}
#endif
#ifdef LEVEL2
void updateinductivecouple() {
    savestate(2,realval(VSRC_2));
    savestate(3,realval(VSRC_1));
}

void initinductivecouple() {
    dv.kk.refdev=ref->d.devpars.refdev1;
    dv.kk.k = ref->d.devpars.mainparam[0];
    dv.kk.destports=ref->d.devpars.port;
    dv.kk.srcports=devices[dv.kk.refdev].d.devpars.port;
    dv.kk.destports[0]=dv.kk.srcports[0];
    dv.kk.destports[3]=dv.kk.srcports[1];
    dv.kk.destports[4]=dv.kk.srcports[4];

    dv.kk.l1 = devices[dv.kk.refdev].d.devpars.mainparam[0];
    dv.kk.refdev=ref->d.devpars.refdev2;
    dv.kk.srcports=devices[dv.kk.refdev].d.devpars.port;
    dv.kk.destports[1]=dv.kk.srcports[0];
    dv.kk.destports[2]=dv.kk.srcports[1];

    dv.kk.l2 = devices[dv.kk.refdev].d.devpars.mainparam[0];
    dv.kk.destports[5]=dv.kk.srcports[4];
    dv.kk.m=dv.kk.k*sqrt(dv.kk.l1*dv.kk.l2);

    switch (analyse) {
    case DCMODE:
        savestate1(1,&dv.kk.m);
        break;
    case TRANMODE:
        savestate1(1,&dv.kk.m);
        dv.kk.Iold=restorestate(2);
        dv.kk.r12=dv.kk.m/timestep;    /* Euler */
        dv.kk.v1=-dv.kk.r12*dv.kk.Iold;
        dv.kk.Iold=restorestate(3);
        dv.kk.v2=-dv.kk.r12*dv.kk.Iold;

        /* r12 and r21 are the same when we have separate self inductivities */
        addARealMinusV (VSRC_1_TO_VSRC_2, &dv.kk.r12);
        addARealMinusV (VSRC_2_TO_VSRC_1, &dv.kk.r12);
        addEReal (VSRC_1, dv.kk.v1);
        addEReal (VSRC_2, dv.kk.v2);

        break;
    case ACMODE:

        dv.kk.a =1.0 - dv.kk.k * dv.kk.k;
        dv.kk.oa=omega*dv.kk.a;
        dv.kk.y1 = (dv.kk.a-1.0)/(dv.kk.oa * dv.kk.l1 );
        dv.kk.y2 = (dv.kk.a-1.0)/(dv.kk.oa * dv.kk.l2 );
        dv.kk.y3 = -dv.kk.k / (dv.kk.oa * sqrt (dv.kk.l1 * dv.kk.l2) );

        addAImaginaryV (NODE_1_TO_NODE_1, &dv.kk.y1);
        addAImaginaryV (NODE_4_TO_NODE_4, &dv.kk.y1);
        addAImaginaryMinusV (NODE_1_TO_NODE_4, &dv.kk.y1);
        addAImaginaryMinusV (NODE_4_TO_NODE_1, &dv.kk.y1);

        addAImaginaryV (NODE_2_TO_NODE_2, &dv.kk.y2);
        addAImaginaryV (NODE_3_TO_NODE_3, &dv.kk.y2);
        addAImaginaryMinusV (NODE_2_TO_NODE_3, &dv.kk.y2);
        addAImaginaryMinusV (NODE_3_TO_NODE_2, &dv.kk.y2);

        addAImaginaryV (NODE_1_TO_NODE_3, &dv.kk.y3);
        addAImaginaryV (NODE_3_TO_NODE_1, &dv.kk.y3);
        addAImaginaryV (NODE_2_TO_NODE_4, &dv.kk.y3);
        addAImaginaryV (NODE_4_TO_NODE_2, &dv.kk.y3);
        addAImaginaryMinusV (NODE_1_TO_NODE_2, &dv.kk.y3);
        addAImaginaryMinusV (NODE_2_TO_NODE_1, &dv.kk.y3);
        addAImaginaryMinusV (NODE_3_TO_NODE_4, &dv.kk.y3);
        addAImaginaryMinusV (NODE_4_TO_NODE_3, &dv.kk.y3);

        break;
    }
}
#endif

#ifdef LEVEL1
void updateinductor() {
    savestate(1,realval(VSRC_1));
}

void initinductor() {

    dv.ll.l=ref->d.devpars.mainparam[0];
    switch (analyse) {
    case DCMODE:
    case TRANMODE:
        addAOne (VSRC_1_TO_NODE_1, 1.0);
        addAOne (NODE_1_TO_VSRC_1, 1.0);
        addAMinusOne (VSRC_1_TO_NODE_2, -1.0);
        addAMinusOne (NODE_2_TO_VSRC_1, -1.0);
        addAZero (VSRC_1_TO_VSRC_1, 0);
        addEZero (VSRC_1, 0.0);
        if (analyse==TRANMODE) {
            dv.ll.Iold=restorestate(1);
            dv.ll.r=dv.ll.l/timestep;    /* Euler */
            dv.ll.v=-dv.ll.r*dv.ll.Iold;
            savestate1(3,&dv.ll.v);
            addARealMinusV (VSRC_1_TO_VSRC_1, &dv.ll.r);
            addEReal (VSRC_1, dv.ll.v);
        }
        break;
    case ACMODE:
        dv.ll.y=-1.0/(omega*dv.ll.l);
        addAImaginaryV (NODE_1_TO_NODE_1, &dv.ll.y);
        addAImaginaryV (NODE_2_TO_NODE_2, &dv.ll.y);
        addAImaginaryMinusV (NODE_1_TO_NODE_2, &dv.ll.y);
        addAImaginaryMinusV (NODE_2_TO_NODE_1, &dv.ll.y);
        break;
    }
}
#endif

#ifdef LEVEL2
void initMOSFET() {
    if (analyse !=ACMODE) {
        /* fetch device model parameters */
        dv.mm.Isd = paramfrommodel( 0);
        dv.mm.Iss = paramfrommodel( 0);
        dv.mm.n   = paramfrommodel( 1);
        dv.mm.l   = paramfrommodel( 6);
        dv.mm.Kp = paramfrommodel( 4);
        dv.mm.W   = paramfrommodel( 5);
        dv.mm.Ll  = paramfrommodel( 2);
        dv.mm.Ld = paramfrommodel( 3);
        dv.mm.Ga = paramfrommodel( 7);
        dv.mm.Vto = paramfrommodel( 8);
        dv.mm.Phi = paramfrommodel( 9);
        dv.mm.pol=1.0;
        if (devices[ref->d.devpars.modelindex].d.modelpars.modelkind==6) {
            dv.mm.pol=-1.0; /* pol=-1 for PMOS */
        }
        dv.mm.Leff = dv.mm.Ll - 2.0 * dv.mm.Ld;
        if ( lessorequalthanzero(&dv.mm.Leff)) {
            dv.mm.Leff = dv.mm.Ll;
        }
        dv.mm.beta = dv.mm.Kp * dv.mm.W / dv.mm.Leff;
        dv.mm.Ugd =  previousvoltage (NODE_2_TO_NODE_1, dv.mm.pol);
        dv.mm.Ugs =  previousvoltage (NODE_2_TO_NODE_3,dv.mm.pol);
        dv.mm.Ubs =  previousvoltage (NODE_4_TO_NODE_3,dv.mm.pol);
        dv.mm.Ubd =  previousvoltage (NODE_4_TO_NODE_1, dv.mm.pol);
        dv.mm.Uds = dv.mm.Ugs - dv.mm.Ugd;
        dv.mm.Vtn=Vt*dv.mm.n;
        dv.mm.UbsCrit = pnCriticalVoltage (dv.mm.Iss, dv.mm.Vtn);
        dv.mm.UbdCrit = pnCriticalVoltage (dv.mm.Isd, dv.mm.Vtn);
        if (bothzero(&dv.mm.Ubs,&dv.mm.Ubd)) {
            setzero(&dv.mm.UbsPrev);
            setzero(&dv.mm.UbdPrev);
        }
        else {
            dv.mm.UbsPrev=restorestate(6);
            dv.mm.UbdPrev=restorestate(7);
        }
        dv.mm.UbsPrev = dv.mm.Ubs = pnVoltage (dv.mm.Ubs, dv.mm.UbsPrev, dv.mm.Vtn, dv.mm.UbsCrit);
        dv.mm.UbdPrev = dv.mm.Ubd = pnVoltage (dv.mm.Ubd, dv.mm.UbdPrev, dv.mm.Vtn, dv.mm.UbdCrit);
        savestate1(6,&dv.mm.UbsPrev);
        savestate1(7,&dv.mm.UbdPrev);

        /* parasitic bulk-source diode*/
        dv.mm.gtiny = dv.mm.Iss;
        pnJunctionBIP (dv.mm.Ubs, dv.mm.Iss, dv.mm.Vtn, &dv.mm.Ibs, &dv.mm.gbs);
        dv.mm.Ibs += dv.mm.gtiny * dv.mm.Ubs;
        dv.mm.gbs += dv.mm.gtiny;

        /* parasitic bulk-drain diode*/
        dv.mm.gtiny = dv.mm.Isd;
        pnJunctionBIP (dv.mm.Ubd, dv.mm.Isd, dv.mm.Vtn, &dv.mm.Ibd, &dv.mm.gbd);
        dv.mm.Ibd += dv.mm.gtiny * dv.mm.Ubd;
        dv.mm.gbd += dv.mm.gtiny;

        /* differentiate inverse and forward mode*/
        dv.mm.MOSdir = (lessthanzero(&dv.mm.Uds)) ? -1.0 : 1.0;

        /* first calculate sqrt (Upn - Phi)*/
        dv.mm.Upn = (greaterthanzero(&dv.mm.MOSdir)) ? dv.mm.Ubs : dv.mm.Ubd;
        dv.mm.Sphi = sqrt (dv.mm.Phi);
        if (lessorequalthanzero(&dv.mm.Upn)) {
            /* take equation as is*/
            dv.mm.Sarg = sqrt (dv.mm.Phi - dv.mm.Upn);
        }
        else {
            /* taylor series of "sqrt (x - 1)" -> continual at Ubs/Ubd = 0*/
            dv.mm.Sarg = dv.mm.Sphi - dv.mm.Upn / dv.mm.Sphi / 2.0;
            dv.mm.Sarg = (lessthanzero(&dv.mm.Sarg)?0.0:dv.mm.Sarg);
        }

        /* calculate bias-dependent threshold voltage*/
        dv.mm.Uon = dv.mm.Vto * dv.mm.pol + dv.mm.Ga * (dv.mm.Sarg - dv.mm.Sphi);
        dv.mm.Utst = (greaterthanzero(&dv.mm.MOSdir) ? dv.mm.Ugs : dv.mm.Ugd) - dv.mm.Uon;
        /* no infinite backgate transconductance (if non-zero Ga)*/
        dv.mm.arg = (dv.mm.Sarg != 0.0) ? (dv.mm.Ga / dv.mm.Sarg / 2.0) : 0;

        /* cutoff region*/
        if (lessorequalthanzero(&dv.mm.Utst)) {
            setzero(&dv.mm.Ids);
            setzero(&dv.mm.gm);
            setzero(&dv.mm.gds);
            setzero(&dv.mm.gmb);
        }
        else {
            dv.mm.Vds = dv.mm.Uds * dv.mm.MOSdir;
            dv.mm.b   = dv.mm.beta * (1.0 + dv.mm.l * dv.mm.Vds);
            /* saturation region*/
            if (dv.mm.Utst <= dv.mm.Vds) {
                dv.mm.UtstSquaredHalf=dv.mm.Utst * dv.mm.Utst / 2.0;
                dv.mm.Ids = dv.mm.b * dv.mm.UtstSquaredHalf;
                dv.mm.gm  = dv.mm.b * dv.mm.Utst;
                dv.mm.gds = dv.mm.l * dv.mm.beta * dv.mm.UtstSquaredHalf;
            }
            /* linear region*/
            else {
                dv.mm.Vdsgain=dv.mm.Vds * (dv.mm.Utst - dv.mm.Vds / 2.0);
                dv.mm.Ids = dv.mm.b * dv.mm.Vdsgain;
                dv.mm.gm  = dv.mm.b * dv.mm.Vds;
                dv.mm.gds = dv.mm.b * (dv.mm.Utst - dv.mm.Vds) + dv.mm.l * dv.mm.beta * dv.mm.Vdsgain;
            }
            dv.mm.gmb = dv.mm.gm * dv.mm.arg;
        }
        dv.mm.Udsat = dv.mm.pol * (lessthanzero(&dv.mm.Utst)?0.0:dv.mm.Utst);
        dv.mm.Ids = dv.mm.MOSdir * dv.mm.Ids;
        dv.mm.Uon = dv.mm.pol * dv.mm.Uon;
        /* compute autonomic current sources*/
        dv.mm.IeqBD = dv.mm.Ibd - dv.mm.gbd * dv.mm.Ubd;
        dv.mm.IeqBS = dv.mm.Ibs - dv.mm.gbs * dv.mm.Ubs;
        dv.mm.IeqDS = dv.mm.Ids - dv.mm.gds * dv.mm.Uds;
        if (greaterthanzero(&dv.mm.MOSdir)) {
            dv.mm.IeqDS -= dv.mm.gm * dv.mm.Ugs + dv.mm.gmb * dv.mm.Ubs;
        } else {
            dv.mm.IeqDS -= dv.mm.gm * dv.mm.Ugd + dv.mm.gmb * dv.mm.Ubd;
        }

        addIZero (NODE_2, 0.0);
        addIReal (NODE_1, (dv.mm.IeqBD - dv.mm.IeqDS) * dv.mm.pol);
        addIReal (NODE_3, (dv.mm.IeqBS + dv.mm.IeqDS) * dv.mm.pol);
        addIReal (NODE_4, (-dv.mm.IeqBD - dv.mm.IeqBS) * dv.mm.pol);
    }
    else {
        dv.mm.gbs=restorestate(0);
        dv.mm.gbd=restorestate(1);
        dv.mm.gds=restorestate(2);
        dv.mm.gm=restorestate(3);
        dv.mm.gmb=restorestate(4);
        dv.mm.MOSdir=restorestate(5);
    }
    /* exchange controlling nodes if necessary*/
    dv.mm.SourceControl = (greaterthanzero(&dv.mm.MOSdir)) ? (dv.mm.gm + dv.mm.gmb) : 0.0;
    dv.mm.DrainControl  = (lessthanzero(&dv.mm.MOSdir)) ? (dv.mm.gm + dv.mm.gmb) : 0.0;

    /* apply admittance matrix elements*/
    addAZero (NODE_2_TO_NODE_2, 0.0);
    addAZero (NODE_2_TO_NODE_1, 0.0);
    addAZero (NODE_2_TO_NODE_3, 0.0);
    addAZero (NODE_2_TO_NODE_4, 0.0);
    addARealV (NODE_1_TO_NODE_2, &dv.mm.gm);
    addAReal (NODE_1_TO_NODE_1, dv.mm.gds + dv.mm.gbd - dv.mm.DrainControl);
    addAReal (NODE_1_TO_NODE_3, -dv.mm.gds - dv.mm.SourceControl);
    addAReal (NODE_1_TO_NODE_4, dv.mm.gmb - dv.mm.gbd);
    addARealMinusV (NODE_3_TO_NODE_2, &dv.mm.gm);
    addAReal (NODE_3_TO_NODE_1, -dv.mm.gds + dv.mm.DrainControl);
    addAReal (NODE_3_TO_NODE_3, dv.mm.gbs + dv.mm.gds + dv.mm.SourceControl);
    addAReal (NODE_3_TO_NODE_4, -dv.mm.gbs - dv.mm.gmb);
    addAZero (NODE_4_TO_NODE_2, 0.0);
    addARealMinusV (NODE_4_TO_NODE_1, &dv.mm.gbd);
    addARealMinusV (NODE_4_TO_NODE_3, &dv.mm.gbs);
    addAReal (NODE_4_TO_NODE_4, dv.mm.gbs + dv.mm.gbd);
    if (analyse==DCMODE) {
        savestate1(0,&dv.mm.gbs);
        savestate1(1,&dv.mm.gbd);
        savestate1(2,&dv.mm.gds);
        savestate1(3,&dv.mm.gm);
        savestate1(4,&dv.mm.gmb);
        savestate1(5,&dv.mm.MOSdir);
    }
}
#endif
#ifdef LEVEL2
void initBJT() {
    if (analyse!=ACMODE) {
        dv.q.Is=paramfrommodel(0);
        dv.q.Nf=paramfrommodel(1);
        dv.q.Nr=paramfrommodel(2);
        dv.q.Bf=paramfrommodel(3);
        dv.q.Br=paramfrommodel(4);
        dv.q.VtNf=Vt*dv.q.Nf;
        dv.q.VtNr=Vt*dv.q.Nr;
        dv.q.af=dv.q.Bf/(1.0+dv.q.Bf);
        dv.q.ar=dv.q.Br/(1.0+dv.q.Br);
        dv.q.pol=1.0;
        if (devices[ref->d.devpars.modelindex].d.modelpars.modelkind==2) {
            dv.q.pol=-1.0;
        }
        dv.q.Ube =  previousvoltage (NODE_2_TO_NODE_3,dv.q.pol);
        dv.q.Ubc =  previousvoltage (NODE_2_TO_NODE_1, dv.q.pol);

        dv.q.UbeCrit = pnCriticalVoltage (dv.q.Is, dv.q.VtNf);
        dv.q.UbcCrit = pnCriticalVoltage (dv.q.Is, dv.q.VtNr);
        if (bothzero(&dv.q.Ube,&dv.q.Ubc)) {
            dv.q.UbePrev=1.0;
            dv.q.UbcPrev=1.0;
        }
        else
        {
            dv.q.UbePrev=restorestate(6);
            dv.q.UbcPrev=restorestate(7);
        }
        dv.q.UbePrev = dv.q.Ube = pnVoltage (dv.q.Ube, dv.q.UbePrev, dv.q.VtNf, dv.q.UbeCrit);
        dv.q.UbcPrev = dv.q.Ubc = pnVoltage (dv.q.Ubc, dv.q.UbcPrev, dv.q.VtNr, dv.q.UbcCrit);
        savestate1(6,&dv.q.UbePrev);
        savestate1(7,&dv.q.UbcPrev);

        pnJunctionBIP (dv.q.Ube, dv.q.Is, dv.q.VtNf, &dv.q.Ied, &dv.q.Ged);
        pnJunctionBIP (dv.q.Ubc, dv.q.Is, dv.q.VtNr, &dv.q.Icd, &dv.q.Gcd);
        dv.q.afGed=dv.q.af*dv.q.Ged;
        dv.q.arGcd=dv.q.ar*dv.q.Gcd;
        dv.q.afm1Ged=dv.q.afGed-dv.q.Ged;
        dv.q.arm1Gcd=dv.q.arGcd-dv.q.Gcd;
        addIReal (NODE_1, (dv.q.Icd - dv.q.Gcd*dv.q.Ubc  - dv.q.af* dv.q.Ied +dv.q.afGed*dv.q.Ube)*dv.q.pol);
        addIReal (NODE_2, ((dv.q.ar-1.0)* dv.q.Icd  + (dv.q.af-1.0)* dv.q.Ied
                           - dv.q.afm1Ged*dv.q.Ube - dv.q.arm1Gcd*dv.q.Ubc )*dv.q.pol);
        addIReal (NODE_3, (dv.q.Ied - dv.q.Ged*dv.q.Ube - dv.q.ar* dv.q.Icd + dv.q.arGcd*dv.q.Ubc)*dv.q.pol);
    }
    else {
        dv.q.Ged=restorestate(0);
        dv.q.Gcd=restorestate(1);
        dv.q.afGed=restorestate(2);
        dv.q.afm1Ged=restorestate(3);
        dv.q.arGcd=restorestate(4);
        dv.q.arm1Gcd=restorestate(5);
    }

    addARealV (NODE_1_TO_NODE_1, &dv.q.Gcd);
    addAReal (NODE_1_TO_NODE_2, dv.q.afGed- dv.q.Gcd );
    addARealMinusV (NODE_1_TO_NODE_3, &dv.q.afGed);
    addARealV (NODE_2_TO_NODE_1,  &dv.q.arm1Gcd);
    addAReal (NODE_2_TO_NODE_2, -dv.q.afm1Ged -dv.q.arm1Gcd);
    addARealV (NODE_1_TO_NODE_3, &dv.q.afm1Ged);
    addARealMinusV (NODE_3_TO_NODE_1, &dv.q.arGcd);
    addAReal (NODE_3_TO_NODE_2, dv.q.arGcd - dv.q.Ged);
    addARealV (NODE_3_TO_NODE_3,&dv.q.Ged);
    if (analyse==DCMODE) {
        savestate1(0,&dv.q.Ged);
        savestate1(1,&dv.q.Gcd);
        savestate1(2,&dv.q.afGed);
        savestate1(3,&dv.q.afm1Ged);
        savestate1(4,&dv.q.arGcd);
        savestate1(5,&dv.q.arm1Gcd);
    }
}
#endif

void initResistor() {
    dv.res.r1=ref->d.devpars.mainparam[0];
    dv.res.g1 = 1.0 / dv.res.r1;
    addARealV (NODE_1_TO_NODE_1, &dv.res.g1);
    addARealV (NODE_2_TO_NODE_2, &dv.res.g1);
    addARealMinusV (NODE_1_TO_NODE_2, &dv.res.g1);
    addARealMinusV (NODE_2_TO_NODE_1, &dv.res.g1);
}

#ifdef LEVEL2
void initTransmissionLine() {
    dv.t.Tau=ref->d.devpars.mainparam[0];
    dv.t.z=ref->d.devpars.mainparam[1];
    switch (analyse) {
    case DCMODE:
        addAOne (NODE_1_TO_VSRC_1, +1);
        addAOne (NODE_3_TO_VSRC_1, -1);
        addAOne (VSRC_1_TO_NODE_1, +1);
        addAOne (VSRC_1_TO_NODE_3, -1);

        addAOne (NODE_2_TO_VSRC_2, +1);
        addAOne (NODE_4_TO_VSRC_2, -1);
        addAOne (VSRC_2_TO_NODE_2, +1);
        addAOne (VSRC_2_TO_NODE_4, -1);

        break;
    case TRANMODE:
        addAOne (NODE_1_TO_VSRC_1, +1);
        addAOne(NODE_3_TO_VSRC_2, +1);
        addAMinusOne (NODE_2_TO_VSRC_1, -1);
        addAMinusOne (NODE_4_TO_VSRC_2, -1);

        addAOne (VSRC_1_TO_NODE_1, +1);
        addAOne (VSRC_2_TO_NODE_3, +1);
        addAMinusOne (VSRC_1_TO_NODE_2, -1);
        addAMinusOne (VSRC_2_TO_NODE_4, -1);
        dv.t.prevj1=restorestate(4);
        dv.t.prevj2=restorestate(5);
        dv.t.prev12=restorestate(6);
        dv.t.prev34=restorestate(7);
        if (currenttime==0.0)
            dv.t.prevj1=dv.t.prevj2=dv.t.prev12=dv.t.prev34=0.0;
        addEReal (VSRC_1,  dv.t.prev34 + dv.t.z * dv.t.prevj2);
        addEReal (VSRC_2,  dv.t.prev12 + dv.t.z * dv.t.prevj1);
        addAReal (VSRC_1_TO_VSRC_1, -dv.t.z);
        addAReal (VSRC_2_TO_VSRC_2, -dv.t.z);
        break;
    case ACMODE:
        dv.t.ot=omega*dv.t.Tau;
        dv.t.explg.re=cos(dv.t.ot);
        dv.t.explg.im=sin(dv.t.ot);
        dv.t.expminlg.re=dv.t.explg.re;
        dv.t.expminlg.im=-dv.t.explg.im;
        complexAdd(&dv.t.expsum,&dv.t.explg,&dv.t.expminlg);
        complexSub(&dv.t.expdiff,&dv.t.explg,&dv.t.expminlg);
        complexDiv(&dv.t.cothlg,&dv.t.expsum,&dv.t.expdiff);
        dv.t.chelp.re=2.0;
        dv.t.chelp.im=0.0;
        complexDiv(&dv.t.cschlg,&dv.t.chelp,&dv.t.expdiff);
        dv.t.chelp.re=dv.t.z;
        complexDiv(&dv.t.y11,&dv.t.cothlg,&dv.t.chelp);
        dv.t.chelp.re=-dv.t.z;
        complexDiv(&dv.t.y21,&dv.t.cschlg,&dv.t.chelp);
        addAComplexV (NODE_1_TO_NODE_1, &dv.t.y11);
        addAComplexV (NODE_3_TO_NODE_3, &dv.t.y11);
        addAComplexV (NODE_4_TO_NODE_4, &dv.t.y11);
        addAComplexV (NODE_2_TO_NODE_2, &dv.t.y11);
        addAComplexMinusV (NODE_1_TO_NODE_2, &dv.t.y11);
        addAComplexMinusV (NODE_2_TO_NODE_1, &dv.t.y11);
        addAComplexMinusV (NODE_3_TO_NODE_4, &dv.t.y11);
        addAComplexMinusV (NODE_4_TO_NODE_3, &dv.t.y11);
        addAComplexV (NODE_1_TO_NODE_3, &dv.t.y21);
        addAComplexV (NODE_3_TO_NODE_1, &dv.t.y21);
        addAComplexV (NODE_4_TO_NODE_2, &dv.t.y21);
        addAComplexV (NODE_2_TO_NODE_4, &dv.t.y21);
        addAComplexMinusV (NODE_1_TO_NODE_4, &dv.t.y21);
        addAComplexMinusV (NODE_4_TO_NODE_1, &dv.t.y21);
        addAComplexMinusV (NODE_3_TO_NODE_2, &dv.t.y21);
        addAComplexMinusV (NODE_2_TO_NODE_3, &dv.t.y21);
        break;
    }
}

void updatetransmissionline() {
    savestate(4,realval(VSRC_1));
    savestate(5,realval(VSRC_2));
    savestate(6,previousvoltage(NODE_1_TO_NODE_2,1.0));
    savestate(7,previousvoltage(NODE_3_TO_NODE_4,1.0));
}
#endif

/* Voltage sources {"AC","DC","EXP","PULSE","SFFM","SIN"};*/

void initVoltageSource() {
#ifdef LEVEL1
    switch (analyse) {
    case DCMODE:
    case TRANMODE:
        dv.vs.valuere=dcsources();
        setzero(&dv.vs.valueim);
        break;
    case ACMODE:
        if (ref->d.devpars.modelindex==0) {
            polartorect(ref->d.devpars.mainparam[0],radian(ref->d.devpars.mainparam[1]),&dv.vs.valuere,&dv.vs.valueim);
        }
        else {
            setzero(&dv.vs.valueim);
            dv.vs.valuere=dv.vs.valueim;
        }
        break;
    }
#endif
    addAOne (VSRC_1_TO_NODE_1, 1.0);
    addAOne (NODE_1_TO_VSRC_1, 1.0);
    addAMinusOne (VSRC_1_TO_NODE_2, -1.0);
    addAMinusOne (NODE_2_TO_VSRC_1, -1.0);
    addAZero (VSRC_1_TO_VSRC_1, 0.0);
#ifdef LEVEL1
    addE (VSRC_1, dv.vs.valuere,dv.vs.valueim);
#else
    addE (VSRC_1, ref->d.devpars.mainparam[0],0.0);

#endif
}

void empty() {
}


devicetemplate devicefunctions[DEVICES]= {
    { 'R',0,0,0,0,initResistor,empty,"n12P" }, /* Resistor */
    { 'I',0,0,0,0,initcurrentsource,empty,"n12S"  },  /* Independent current source */
    { 'V',1,1,1,0,initVoltageSource,empty,"n12S" }, /* Independent voltage source */
#ifdef LEVEL1
    { 'L',1,0,1,0,initinductor,updateinductor,"n12P" }, /* Inductivity */
    { 'C',0,0,0,0,initcapacitor,updatecapacitor,"n12P" }, /* Capacity */
    { 'E',1,1,1,0,initvcvs,empty,"n1234P" }, /* Voltage controlled voltage source */
    { 'F',1,1,1,0,initcccs,empty,"n12VP" }, /* Current controlled current source */
    { 'G',1,1,1,0,initvccs,empty,"n1234P" }, /* Voltage controlled current source */
    { 'H',1,1,1,0,initccvs,empty,"n12VP" }, /* Current controlled voltage source */
#endif
#ifdef LEVEL2
    { 'D',0,0,0,1,initdiode,empty,"n12M" }, /* Diode */
    { 'J',0,0,0,1,initJFET,empty,"n123M" },/* Junction FET */
    { 'K',0,0,0,0,initinductivecouple,updateinductivecouple,"nLlP" }, /* Inductive couple */
    { 'M',0,0,0,1,initMOSFET,empty,"n1234M" }, /* MOSFET */
    { 'Q',0,0,0,1,initBJT,empty,"n123M" }, /* Bipolar transistor */
    { 'T',0,0,2,0,initTransmissionLine,updatetransmissionline,"n1234T" }, /* Transmission line */
#endif
};


void  PutToDevice(int pos) {
    static char * clearer,* source, * dest;
    static int count;
    count=sizeof(device);
    if (pos<0) {
        if (!testmemory(count)) {
            return;
        }
        totaldevices++;
        inputentry=(char *) (devices+totaldevices+1);
        clearer=(char *) (devices+totaldevices);
        while (count--) {
            *clearer = 0;
            ++clearer;
        }
    }
    else {
        source=(char *) (devices+totaldevices);
        dest=(char *) (devices+pos);
        while (count--) {
            *dest++ = *source++;
        }
    }
}

#ifdef LEVEL2
TINYINT remapnode(TINYINT n,char* subcktname) {
    static int posmodel,poscaller,i,modelnode,callernode;
    poscaller=finddevice(subcktname);
    if (n==0) return n;
    if (poscaller<0) return n;
    posmodel=devices[poscaller].d.subcktports.modelindex;
    if (posmodel<0) return n;
    for (i=0; i<SCREENWID; i++) {
        modelnode=devices[posmodel].d.subcktports.port1[i];
        if (modelnode==255) {
            return n+startsubcktnode;
        }
        if (modelnode==n) {
            callernode=devices[poscaller].d.subcktports.port1[i];
            if (callernode==255) {
                return n;
            }
            else {
                return callernode;
            }
        }
    }
}
#endif

char * uniparser (device * self, char * inputline,char * template, char * subcktname) {
    static char * m,* s,tdpar[4];
    static TINYINT n;
    static float v;
    s=inputline;
    for (m=template; (*m != 0) && (s != NULL) ; m++) {
        switch(*m) {
        case 'n' :
            s=getname(s,self->name,NAMELEN);
            strncat(self->name,subcktname,NAMELEN-strlen(self->name));
            break;
        case '1' :
        case '2' :
        case '3' :
        case '4' :
            s=getunsignedint(s,&n);
#ifdef LEVEL2
            if (*subcktname) {
                n=remapnode(n,subcktname);
            }
#endif
            self->d.devpars.port[*m-'1']=n;
            break;
        case 'P' :
            s=getfloat(s,&(self->d.devpars.mainparam[0]));
            break;
        case 'p' :
            s=getfloat(s,&(self->d.devpars.mainparam[1]));
            break;
        case 'S' :
            s=Sources(s,self);
            break;
#ifdef LEVEL2

        case 'M' :
            s=reftomodel(s,self);
            break;
        case 'L' :
            s=reftodevice(s,self,3,1,subcktname); /* connect mutual inductor to ports 0 and 3 */
            break;
        case 'l' :
            s=reftodevice(s,self,3,2,subcktname);  /* connect mutual inductor to ports 1 and 2 */
            break;
#endif
        case 'V' :
            s=reftodevice(s,self,2,1,subcktname);  /* connect voltage source to ports 2 and 3 */
            break;
#ifdef LEVEL2
        case 'T' :
            for (n=0; n<2; n++) {
                s=getname(s,tdpar,NAMELEN);
                s=expect(s,'=');
                if (s) s=getfloat(s,&v);
                if (s) {
                    if (!strncmp(tdpar,"Z0",4)) {
                        self->d.devpars.mainparam[1]=v;
                    }
                    if (!strncmp(tdpar,"TD",4)) {
                        self->d.devpars.mainparam[0]=v;
                    }
                }
                else break;
            }
            break;
#endif
        }
    }
    return(s);
}

char *  DoDevice(char * s,char * subcktname) {
    static char devicecode;
    static TINYINT i;
    static int pos;
    static device * currentdevice;
    static char * s1;
    static char name[NAMELEN];
    devicetemplate *  devtempl;
    s1=s;
    getname(s,name,NAMELEN);
    strncat(name,subcktname,NAMELEN-strlen(name));
    devicecode=*s;
    for (i=0; i<DEVICES; i++)
        if (devicefunctions[i].type==devicecode) {
            devtempl=devicefunctions+i;
            pos=finddevice(name);
            currentdevice=devices+totaldevices;
            currentdevice->type=i;
            s=uniparser (currentdevice,s,devtempl->parsetempl,subcktname);
            if (s) {
                if (subcktmode) {
                    currentdevice->type='"';
                    strncpy(currentdevice->name," ",NAMELEN);
                    strncpy(currentdevice->d.printvars.str,s1,SCREENWID);
                    PutToDevice(-1);
                }
                else {
                    PutToDevice(pos);
                }
            }
            return (s);
        }
    return NULL;
}

/**********************************/
/*   EQUATION ALLOCATION FUNCITONS*/
/**********************************/

TINYINT FindMaxNodeIndex() {
    static TINYINT i,j;
    static TINYINT maxnode;
    static device * devicesi;
    maxnode=0;
    for (i=0; i<totaldevices; i++) {
        devicesi=devices+i;
        for (j=0; j<4; j++) {
            if (devicesi->d.devpars.port[j]>maxnode && devicesi->type<DEVICES) {
                maxnode=devicesi->d.devpars.port[j];
            }
        }
    }
    return(maxnode);
}

void CalcTotalVariables () {
    static TINYINT curvsvar,tp,addvars,i;
    static device * devicesi;
    static devicetemplate * devicefunctionstp;

    totalnodes=curvsvar=FindMaxNodeIndex();
    for (i=0; i<totaldevices; i++) {
        devicesi=devices+i;
        tp=devicesi->type;
        if (tp<DEVICES) {
            devicefunctionstp=devicefunctions+tp;
#ifdef LEVEL1
            switch (analyse) {
            case DCMODE:
                addvars=devicefunctionstp->extravarsDC;
                break;
            case ACMODE:
                addvars=devicefunctionstp->extravarsAC;
                break;
            case TRANMODE:
                addvars=devicefunctionstp->extravarsTran;
                break;
            }
#else
            addvars=devicefunctionstp->extravarsDC;
#endif
            if (addvars==2) {
                devicesi->d.devpars.port[5]= ++curvsvar;
                addvars--;
            }
            if (addvars==1) {
                devicesi->d.devpars.port[4]= ++curvsvar;
            }
        }
    }
    totalcolumns=curvsvar;
}
unsigned int matsize() {
    return totalcolumns*(totalcolumns+1);
}
BOOL SolveRealSystem(int N,float * sol) {
    static float t;
    static int i,j,k,M;
    static BOOL changed;
    static float * RealSystemiM,* RealSystemjM,* soli;
    M=N+1;

    for(i=0; i<N; i++)  {
        RealSystemiM=RealSystem+i*M;
        if (RealSystemiM[i]==0.0) {
            if (i<totalnodes) {
                RealSystemiM[i]=1.0;
            }
            else
            for(j=0; j<N; j++) {
                RealSystemjM=RealSystem+j*M;
                if (RealSystemjM[i]!=0.0 && RealSystemiM[j]!=0.0) {
                    for(k=0; k<M; k++)  {
                        t=RealSystemiM[k];
                        RealSystemiM[k] =RealSystemjM[k];
                        RealSystemjM[k]=t;
                    }
                    break;
                }
                if ( breakkey()) return FALSE;
            }
            if (j==N) err("matrix");
        }
    }
    for(i=0; i<N; i++)  {
        RealSystemiM=RealSystem+i*M;
        for(j=0; j<N; j++) {
            RealSystemjM=RealSystem+j*M;
            if(i!=j) {
                t=RealSystemjM[i]/RealSystemiM[i];
                for(k=0; k<M; k++) {
                    RealSystemjM[k]-=RealSystemiM[k]*t;
                }
            }
            if ( breakkey()) return FALSE;
        }
    }
    changed=FALSE;
    soli=sol;
    for(i=0; i<N; i++,soli++)  {
        RealSystemiM=RealSystem+i*M;
        t=*soli;
        *soli=RealSystemiM[N]/RealSystemiM[i];
        if (fabs(*soli-t)>1.0e-7) {
            changed=TRUE;
        }
    }
    if (nlcount==0) {
        changed=FALSE;
    }
    return changed;
}
#ifdef LEVEL1
void SolveComplexSystem(int N,complex * sol) {
    static complex t,p,* CompSysiM,* CompSysjM, * CompSysiMpk,* CompSysjMpk;
    static int i,j,k,M;
    M=N+1;
    for(i=0; i<N; i++)  {
        CompSysiM=ComplexSystem+i*M;
        if (bothzero(&(CompSysiM[i].re),&(CompSysiM[i].im))) {
            if (i<totalnodes) {
                CompSysiM[i].re=1.0;
            }
            for(j=0; j<N; j++) {
                CompSysjM=ComplexSystem+j*M;
                if (!bothzero( &(CompSysjM[i].re),&(CompSysjM[i].im) ) &&
                        (!bothzero( &(CompSysiM[j].re),&(CompSysiM[j].im) ))) {
                    for(k=0; k<M; k++)  {
                        CompSysiMpk=CompSysiM+k;
                        CompSysjMpk=CompSysjM+k;
                        t.re=CompSysiMpk->re;
                        CompSysiMpk->re =CompSysjMpk->re;
                        CompSysjMpk->re=t.re;
                        t.im=CompSysiMpk->im;
                        CompSysiMpk->im =CompSysjMpk->im;
                        CompSysjMpk->im=t.im;
                    }
                    break;
                }
                if ( breakkey()) return;
            }
        }
    }
    for(i=0; i<N; i++)  {
        CompSysiM=ComplexSystem+i*M;
        for(j=0; j<N; j++) {
            CompSysjM=ComplexSystem+j*M;
            if(i!=j) {
                complexDiv(&t,CompSysjM+i,CompSysiM+i);
                for(k=0; k<M; k++)  {
                    complexMul(&p,CompSysiM+k,&t);
                    complexSub(CompSysjM+k,CompSysjM+k,&p);
                }
            }
            if ( breakkey()) return;
        }
    }
    for(i=0; i<N; i++) {
        CompSysiM=ComplexSystem+i*M;
        complexDiv(sol+i,CompSysiM+N,CompSysiM+i);
    }
    return;
}
#endif
void SolveOP() {
    static TINYINT tp;
    static int i,j;
    static void * fn;
    CalcTotalVariables();
#ifdef LEVEL1
    calcmode=REAL;
#endif
    OpSolution=(float *) (devices+totaldevices+1);
    RealSystem=OpSolution+totalcolumns;
    for (i=0; i<totalcolumns; i++) {
        setzero(OpSolution+i);
    }
    for (j=0; j<50; j++) {
        for (i=0; i<matsize(); i++) {
            setzero(RealSystem+i);
        }
        nlcount=0;
        for (i=0; i<totaldevices; i++) {
            if ((tp=devices[i].type)<DEVICES)  {
                setref(devices+i);
                nlcount+=devicefunctions[tp].nonlinear;
                fn=devicefunctions[tp].initdevice;
                FN (devices+i);
            }
        }
        if ( breakkey()) return;
        if (!SolveRealSystem(totalcolumns,OpSolution)) break;
    }
}

#ifdef LEVEL1
void PrintAllNodeVoltages(BOOL header,int of,int to) {
    static int i,j,n,pos,p;
    static char name[20],*format;
    static BOOL doprint;
    static complex * AcSolutioni;
    CalcTotalVariables();
    n=FindMaxNodeIndex();
    pos=finddevice(".PRINT");
    if (pos>=0) {
        format=devices[pos].d.printvars.str;
    }
    for (i=0; i<totalcolumns; i++) {

        for (p=of; p<=to; p++) {
            sp();
            if (i<n) {
                switch (p) {
                case 1:
                    strncpy(name," V(",12);
                    break;
                case 2:
                    strncpy(name,"VM(",12);
                    break;
                case 3:
                    strncpy(name,"VP(",12);
                    break;
                }
                uitoa(i+1,name+3);
                strncpy(&name[strlen(name)],")",12);
            }
            else {
                switch (p) {
                case 1:
                    strncpy(name," I(",12);
                    break;
                case 2:
                    strncpy(name,"IM(",12);
                    break;
                case 3:
                    strncpy(name,"IP(",12);
                    break;
                }
                for (j=0; j<totaldevices; j++) {
                    if (devices[j].d.devpars.port[4]==i+1) {
                        strncpy(name+3,devices[j].name,12);
                        break;
                    }
                }
                strncpy(&name[strlen(name)],")",12);
            }
            doprint=FALSE;
            if (pos>=0) {
                if (strstr(format,name)) {
                    doprint=TRUE;
                }
            }
            else {
                doprint=TRUE;
            }
            for (j=strlen(name); j<10; j++) {
                name[j]=' ';
            }
            name[10]=0;
            if (doprint) {
                if (header) {
                    printstr(name);
                }
                else {
                    AcSolutioni=AcSolution+i;
                    switch (p) {
                    case 1:
                        printfp(OpSolution[i]);
                        break;
                    case 2:
                        printfp(sqrt(AcSolutioni->re*AcSolutioni->re+AcSolutioni->im*AcSolutioni->im));
                        break;
                    case 3:
                        printfp(degree(Atan2(AcSolutioni->im,AcSolutioni->re)));
                        break;
                    }
                }
            }
        }
    }
}
#else
void PrintAllNodeVoltages(BOOL header,int of,int to) {
    static int i;
    if (!header) {
        for (i=0; i<totalcolumns; i++) {
            printfp(OpSolution[i]);
            sp();

        }
    }
}
#endif
/**********************************/
/*   DOT COMMAND FUNCITONS        */
/**********************************/

#ifdef LEVEL2
char * DoModelCommand(char * inputline) {
    static char * s;
    static char modeltype[NAMELEN];
    static int modelref,mtpos,parcnt,pos;
    static TINYINT i;
    static device * currentdevice, * devicesi;
    static float v;
    modeltypes * foundmodel;
    s=inputline;

    getname(s,modeltype,NAMELEN);
    pos=finddevice(modeltype);
    currentdevice=devices+totaldevices;

    currentdevice->type='#';
    currentdevice->d.modelpars.modelkind=0;
    s=getname(s,currentdevice->name,NAMELEN);
    for (i=0; i<TOTMODELPARS; i++) {
        currentdevice->d.modelpars.parindex[i]=255;
    }
    if (s) {
        s=getname(s,modeltype,NAMELEN);
    }
    mtpos=-1;
    for (i=0; i<8; i++) {
        if (!strncmp(mt[i].name,modeltype,NAMELEN)) {
            mtpos=i;
            currentdevice->d.modelpars.modelkind=mtpos;
            break;
        }
    }
    if (mtpos==-1) {
        s=NULL;
    }
    if (s) {
        foundmodel=mt+mtpos;
        modelref=totaldevices;
        s=expect(s,'(');
        parcnt=0;
        while (s) {
            for (; isspace(*s); s++);
            if (*s == ')') {
                break;
            }
            s=getname(s,modeltype,NAMELEN);
            s=expect(s,'=');
            if (s) s=getfloat(s,&v);
            if (parcnt<TOTMODELPARS) {
                for (i=0; i<foundmodel->ptablelen; i++) {
                    if (!strncmp(foundmodel->parametertable[i].paramname,modeltype,NAMELEN)) {
                        currentdevice->d.modelpars.parindex[parcnt]=i;
                        currentdevice->d.modelpars.parvalue[parcnt]=v;
                        parcnt++;
                        break;
                    }
                }
            }
        }
        if (s && pos>=0) {
            for(i=0; i<totaldevices; i++) {
                devicesi=devices+i;
                if (devicesi->d.devpars.modelindex==pos &&
                        devicesi->type != foundmodel->devcode &&
                        devicesi->type != '#') {
                    s=NULL;
                }
            }
        }
        if (s) {
            PutToDevice(pos);
        }
    }
    return (s);
}
#endif

#ifdef LEVEL1
char * DoDCCommand(char * inputline) {
    static char varingdev[NAMELEN];
    static char * s;
    static float startval,endval,incrval,baseval;
    static unsigned int memoryreq;
    static device * vardev;
    static int pos;
    s=inputline;
    s=getname(s,varingdev,NAMELEN);
    pos=finddevice(varingdev);
    if (pos<0) {
        return NULL;
    }
    vardev=devices+pos;
    if (vardev->type !=1 && vardev->type !=2) {
        return NULL;
    }
    if (s) {
        s=getfloat(s,&startval);
    }
    if (s) {
        s=getfloat(s,&endval);
    }
    if (s) {
        s=getfloat(s,&incrval);
    }
    if (incrval==0.0) {
        return NULL;
    }
    if (!s) {
        return NULL;
    }
    analyse=DCMODE;
    calcmode=REAL;
    CalcTotalVariables();
    memoryreq=sizeof(float)*(matsize()); /* N*(N+1) matrix and solution*/
    if (!testmemory(memoryreq)) {
        return NULL;
    }
    baseval=vardev->d.devpars.mainparam[0];
    PrintAllNodeVoltages(TRUE,1,1);
    nl();

    while (startval <= endval) {
        vardev->d.devpars.mainparam[0]=startval;
        startval+=incrval;
        SolveOP();
        if ( breakkey()) return NULL;
        PrintAllNodeVoltages(FALSE,1,1);
        nl();
    }
    vardev->d.devpars.mainparam[0]=baseval;
    return (s);
}
#endif

#ifdef LEVEL1
char *  DoACCommand(char * inputline) {
    static char variation[NAMELEN];
    static char * s;
    static float startfreq,endfreq,freqstep,x,nn,numpoints;
    static int memoryreq,tp,mode;
    static void * fn;
    static TINYINT i,n;
    s=inputline;
    s=getname(s,variation,NAMELEN);
    if (!strncmp(variation,"LIN",NAMELEN)) {
        mode=1;
    }
    else if (!strncmp(variation,"DEC",NAMELEN)) {
        mode=2;
    }
    else if (!strncmp(variation,"OCT",NAMELEN)) {
        mode=3;
    }
    else {
        return NULL;
    }
    if (s) {
        s=getfloat(s,&numpoints);
    }
    if (!numpoints) {
        return NULL;
    }
    if (s) {
        s=getfloat(s,&startfreq);
    }
    if (s) {
        s=getfloat(s,&endfreq);
    }
    if (startfreq>endfreq) {
        return NULL;
    }
    if (numpoints>1.0) {
        freqstep=(endfreq-startfreq)/(numpoints-1.0);
    }
    else {
        freqstep=(endfreq-startfreq)+1.0;
    }
    analyse=ACMODE;
    calcmode=COMPLEX;
    CalcTotalVariables();
    memoryreq=sizeof(complex)*(matsize());
    if (!testmemory(memoryreq)) {
        return NULL;
    }
    analyse=DCMODE;
    SolveOP();  /* Bias point*/
    analyse=ACMODE;
    calcmode=COMPLEX;
    n=FindMaxNodeIndex();
    AcSolution=(complex *) (devices+totaldevices+1);
    ComplexSystem=AcSolution+totalcolumns;
    printstr("FREQUENCY ");
    PrintAllNodeVoltages(TRUE,2,3);
    nl();
    for (setzero(&nn); frequency<endfreq; nn+=1.0) {
        switch (mode) {
        case 1:
            frequency=startfreq+freqstep*nn;
            break;
        case 2:
            x=log(startfreq)/LN10;
            frequency=exp((x+nn/numpoints)*LN10);
            break;
        case 3:
            x=log(startfreq)/LN2;
            frequency=exp((x+nn/numpoints)*LN2);
            break;
        }
        omega=TWOPI*frequency;
        printfp(frequency);
        for (i=0; i<totalcolumns; i++) {
            setzero(&(AcSolution[i].re));
            setzero(&(AcSolution[i].im));
        }
        for (i=0; i<matsize(); i++) {
            setzero(&(ComplexSystem[i].re));
            setzero(&(ComplexSystem[i].im));
        }
        for (i=0; i<totaldevices; i++) {
            if ((tp=devices[i].type)<DEVICES) {
                setref(devices+i);
                fn=devicefunctions[tp].initdevice;
                FN (devices+i);
            }
        }
        SolveComplexSystem(totalcolumns,AcSolution);
        if ( breakkey()) return NULL;
        PrintAllNodeVoltages(FALSE,2,3);
        nl();
    }
    return (s);
}
#endif

char *  DoOPCommand(char * inputline) {
    static unsigned int memoryreq;
    analyse=DCMODE;
    CalcTotalVariables();
    memoryreq=sizeof(float)*(matsize()); /* N*(N+1) matrix and solution*/
    if (!testmemory(memoryreq))
        return NULL;
    SolveOP();
    PrintAllNodeVoltages(TRUE,1,1);
    nl();
    PrintAllNodeVoltages(FALSE,1,1);
    nl();
    return inputline;
}

#ifdef LEVEL1
void RememberStatesDynDev() {
    static void * fn;
    static TINYINT i,tp;
    static device * devicei;
    for (i=0; i<totaldevices; i++) {
        devicei=devices+i;
        if ((tp=devicei->type)<DEVICES) {
            setref(devicei);
            fn=devicefunctions[tp].updatedevice;
            FN (devicei);
        }
    }
}
#endif
#ifdef LEVEL1
char * DoTranCommand(char * inputline) {
    static float tstep,tstop,tstart,printtime,dt;
    static unsigned int memoryreq,i;
    static TINYINT variables;
    static char * s;
    s=inputline;
    if (s)
        s=getfloat(s,&tstep);
    if (tstep==0.0)
        return NULL;
    if (s)
        s=getfloat(s,&tstop);
    if (s) {
        if (!getfloat(s,&tstart)) {
            setzero(&tstart);
        }
    }
    if (tstop<tstart) {
        return NULL;
    }
    timestep=min((tstop-tstart)/50.0,tstep);
    for (i=0; i<totaldevices; i++) {
        if (devices[i].type==13) {
            timestep=devices[i].d.devpars.mainparam[0];
        }
    }
    dt=timestep;
    analyse=DCMODE;
    calcmode=REAL;
    CalcTotalVariables();
    memoryreq=sizeof(float)*(matsize()); /* N*(N+1) matrix and solution*/
    if (!testmemory(memoryreq)) {
        return NULL;
    }
    SolveOP();
    RememberStatesDynDev();
    analyse=TRANMODE;
    printtime=tstart;
    printstr("TIME      ");
    PrintAllNodeVoltages(TRUE,1,1);
    nl();
    for (currenttime=0; currenttime<tstop; currenttime+=timestep) {
        if (currenttime>printtime) {
            timestep-=(currenttime-printtime);
            currenttime=printtime;
        }
        SolveOP();
        if ( breakkey()) return NULL;
        RememberStatesDynDev();
        if (currenttime==printtime) {
            printfp(currenttime);
            PrintAllNodeVoltages(FALSE,1,1);
            timestep=dt;
            printtime+=tstep;
            nl();
        }
    }

    return (s);
}
#endif

#ifdef LEVEL1
char *  DoClearCommand(char * inputline) {
    static char * s;
    static char devname[NAMELEN];
    static int pos;
    s=inputline;
    s=getname(s,devname,NAMELEN);
    pos=finddevice(devname);
    if (pos>=0) {
        totaldevices=pos;
        inputentry=(char *) (devices+totaldevices+1);
    }
    return (s);
}
#endif

#ifdef LEVEL1
char *  DoPrintCommand(char * inputline) {
    static char * s,*devname;
    static int pos;
    static device * currentdevice;
    s=inputline;
    devname=".PRINT";
    pos=finddevice(devname);
    currentdevice=devices+totaldevices;
    currentdevice->type='%';
    strncpy(currentdevice->name,devname,6);
    strncpy(currentdevice->d.printvars.str,inputline,SCREENWID);
    PutToDevice(pos);
    return (s);
}
#endif

/*
        struct subcktportst {
            TINYINT port[SCREENWID];
        } subcktports;
*/
#ifdef LEVEL2
char * SubcktParams (char * s,device * self) {
    static TINYINT i,n;
    static char * s1;
    for (i=0; i<SCREENWID; i++) {
        self->d.subcktports.port1[i]=255;
    }
    for (i=0; i<SCREENWID; i++) {
        s1=getunsignedint(s,&n);
        if (!s1) {
            return s;
        }
        else {
            self->d.subcktports.port1[i]=n;
            s=s1;
        }
    }
    return s;
}
#endif
#ifdef LEVEL2
char *  DoSubcktCommand(char * inputline) {
    static char * s;
    static device * currentdevice;
    s=inputline;
    currentdevice=devices+totaldevices;
    s=getname(s,currentdevice->name,NAMELEN);
    if (!s) {
        return NULL;
    }
    currentdevice->type='.';
    if(SubcktParams(s,currentdevice)) {
        PutToDevice(-1);
        subcktmode=TRUE;
        return (inputline);
    }
    else
        return NULL;
}
#endif
#ifdef LEVEL2
char * DoEndsCommand () {
    static device * currentdevice;
    currentdevice=devices+totaldevices;
    currentdevice->type='!';
    strncpy(currentdevice->name,".ENDS",6);
    PutToDevice(-1);
    subcktmode=FALSE;
    return currentdevice->name;
}
#endif
#ifdef LEVEL2
char *  DoListCommand() {
    static TINYINT i,j,k;
    static char * m;
    static device * currdev;
    puts(circuittitle);

    currdev=devices;
    for (i=0; i<totaldevices; i++,currdev++) {
        switch(currdev->type) {
        case ' ':
            break;
        case '%':
            printstr(currdev->name);
            printstr(currdev->d.printvars.str);
            break;
        case '"':
            printstr(currdev->d.printvars.str);
            break;
        case '!':
            printstr(".ENDS");
            break;
        case '.':
            printstr(".SUBCKT ");
        case 16:
            printstr(currdev->name);
            sp();
            for (j=0; j<SCREENWID; j++) {
                k=currdev->d.subcktports.port1[j];
                if (k==255) {
                    break;
                }
                printint(k);
                sp();
            }
            if (currdev->type==16) {
               printstr(devices[currdev->d.subcktports.modelindex].name);
            }
            break;
        case '#':
            printstr(".MODEL ");
            printstr(currdev->name);
            sp();
            k=currdev->d.modelpars.modelkind;
            printstr(mt[k].name);
            sp();
            putchar('(');

            for (j=0; j<TOTMODELPARS; j++) {
                if(currdev->d.modelpars.parindex[j]<mt[k].ptablelen) {
                    printstr(mt[k].
                             parametertable[currdev->d.modelpars.parindex[j]].paramname);
                    putchar('=');
                    printfp(currdev->d.modelpars.parvalue[j]);
                    sp();
                }
            }
            putchar(')');
            break;
        default:
            for (m=devicefunctions[currdev->type].parsetempl; (*m != 0)  ; m++) {
                switch(*m) {
                case 'n' :
                    printstr(currdev->name);
                    break;
                case '1' :
                case '2' :
                case '3' :
                case '4' :
                    printint(currdev->d.devpars.port[*m -'1']);
                    break;
                case 'P' :
                    printfp(currdev->d.devpars.mainparam[0]);
                    break;
                case 'p' :
                    printfp(currdev->d.devpars.mainparam[1]);
                    break;
                case 'S' :
                    k=currdev->d.devpars.modelindex;
                    printstr(sourcekindnames[k]);
                    sp();
                    if (k>1) putchar('(');
                    for (j=0; j<sourceparcount[k]; j++) {
                        printfp(currdev->d.devpars.mainparam[j]);
                        sp();
                    }
                    if (currdev->d.devpars.modelindex>1) putchar(')');
                    break;
                case 'M' :
                    printstr(devices[currdev->d.devpars.modelindex].name);
                    break;
                case 'L' :
                    printstr(devices[currdev->d.devpars.refdev1].name);
                    break;
                case 'l' :
                    printstr(devices[currdev->d.devpars.refdev2].name);
                    break;
                case 'V' :
                    printstr(devices[currdev->d.devpars.refdev1].name);
                    break;
                case 'T' :
                    printstr("Z0=");
                    printfp(currdev->d.devpars.mainparam[1]);
                    sp();
                    printstr("TD=");
                    printfp(currdev->d.devpars.mainparam[0]);
                    sp();
                    break;
                }
                sp();
            }
        }
        nl();
    }
}
#endif
#ifdef LEVEL2
BOOL DoExpandSubcircuit(char * inputline) {
    static char name [NAMELEN];
    static char * s;
    static device * currentdevice, *expandeddevice;
    static int pos,possubckt;
    s=inputline;
    currentdevice=devices+totaldevices;
    s=getname(s,currentdevice->name,NAMELEN);
    if (!s) return FALSE;
    s=SubcktParams(s,currentdevice);
    s=getname(s,name,NAMELEN);
    if (!s) return FALSE;
    pos=finddevice(currentdevice->name);
    possubckt=finddevice(name);
    if (possubckt<0) return FALSE;
    currentdevice->type=16;
    currentdevice->d.subcktports.modelindex=possubckt;
    PutToDevice(pos);
    startsubcktnode=FindMaxNodeIndex();
    for(expandeddevice=devices+possubckt+1; expandeddevice->type=='"'; expandeddevice++) {
         DoDevice(expandeddevice->d.printvars.str,currentdevice->name);
    }
    return TRUE;
}
#endif
void MainInterpreter() {
    static char * s,* upcase;
    for (;;) {
#ifdef LEVEL3
        if (!fgets(inputentry,128,inputfile))
#endif
        gets(inputentry);
#ifdef GETSNONL
        nl();
#endif
        upcase=inputentry;
        for(; *upcase; upcase++) {
            if (*upcase>='a' && *upcase<='z') {
                *upcase-=32;
            }
        }
#ifdef LEVEL2
        if (*inputentry=='X') {
            if (!DoExpandSubcircuit(inputentry)) {
                err("subcircuit");
            }
        }
        else
#endif
            if (isalpha(*inputentry)) {
                if (!DoDevice(inputentry,"")) {
                    err("device");
                }
            }
            else if (*inputentry=='.') {
                s=NULL;
                if (!strncmp(inputentry,".OP",3)) s=DoOPCommand(inputentry+3);
#ifdef LEVEL1
                else if (!strncmp(inputentry,".DC",3)) s=DoDCCommand(inputentry+3);
                else if (!strncmp(inputentry,".AC",3)) s=DoACCommand(inputentry+3);
                else if (!strncmp(inputentry,".TRAN",5)) s=DoTranCommand(inputentry+5);
                else if (!strncmp(inputentry,".PRINT",6)) s=DoPrintCommand(inputentry+6);
                else if (!strncmp(inputentry,".CLEAR",6)) s=DoClearCommand(inputentry+6);
#endif
#ifdef LEVEL2
                if (!strncmp(inputentry,".MODEL",6)) s=DoModelCommand(inputentry+6);
                else if (!strncmp(inputentry,".TEMP",5)) s=DoTempCommand(inputentry+5);
                else if (!strncmp(inputentry,".LIST",5)) s=DoListCommand();
                else if (!strncmp(inputentry,".SUBCKT",7)) s=DoSubcktCommand(inputentry+7);
                else if (!strncmp(inputentry,".ENDS",5)) s=DoEndsCommand();
#endif
                else if (!strncmp(inputentry,".END",4)) return;
                if (!s)  {
                    err("command");
                }
            }
            else if ((*inputentry==0)||(*inputentry=='*')) {
                continue;
            }
            else {
                err("symbol");
            }
    }
}

#ifdef LEVEL3
void redirect(char * filename) {
   inputfile=fopen(filename, "r");
}
#endif

void main (int argc,char * argv[]) {
    cls();
    puts ("SPECI-SPICE CIRCUIT SIMULATOR");
#ifdef LEVEL3
    if (argc==2) {
        redirect(argv[1]);
    }
#endif
    totaldevices=0;
    subcktmode=FALSE;
    startsubcktnode=0;
    AdjustTemperature(300.0);
    circuittitle=membuf;
#ifdef LEVEL3
    if (!fgets(circuittitle,128,inputfile))
#endif
    gets(circuittitle);
#ifdef GETSNONL
        nl();
#endif
    devices=(device *) (membuf+strlen(circuittitle)+2);
    inputentry=(char *) (devices+1);
    MainInterpreter();
#ifdef LEVEL3
    if (inputfile)
       fclose(inputfile);
#endif
 

}
