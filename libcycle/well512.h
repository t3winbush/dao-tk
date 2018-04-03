#include <vector>
#include <stdint.h>

#ifndef _WELL512_
#define _WELL512_


class WELLFiveTwelve
{
    int W;
    int R;
    int P;
    int M1;
    int M2;
    int M3;
    long double FACT;

    std::vector< uint32_t > state;
    int state_i;


public:
    //WELLFiveTwelve();
    WELLFiveTwelve(int scen, int W=32, int R=16, int P=0, int M1=13, int M2=9, int M3=5);

    void assignStates(int scenario);

    int V0();
    int VM1();
    int VM2();
    int VM3();
    int VRm1();
    int VRm2();
    int newV0();
    int newV1();
    int newVRm1();
    uint32_t MAT0POS(int t, uint32_t v);
    uint32_t MAT0NEG(int t, uint32_t v);
    uint32_t MAT3NEG(int t, uint32_t v);
    uint32_t MAT4NEG(int t, uint32_t b, uint32_t v);
    double getVariate();

};




#endif