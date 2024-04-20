unsigned int div10(unsigned int a){
    unsigned int res = 1;
    while (res * 10 < a){
        res <<= 1;
    }
    res >>= 1;
    unsigned int b = a - (res * 10);
    while (b > 10){
        b -= 10;
        res += 1;
    }
    return res;
}

unsigned int mul(unsigned int a, unsigned int b){
    unsigned int c = 0;
    unsigned int d = a;
    while (b > 0){
        if (b&1){ c += d; }
        b >>= 1;
        d += d;
    }
    return c;
}

