void* memset(void* const dest, const int c, unsigned long long count) {
    unsigned char* p = (unsigned char*)dest;
    const unsigned char value = (unsigned char)c;
    while (count-- != 0) {
       *p++ = value;
    }

    return dest;
}

void* memcpy(void* const dest, const void* src, unsigned long long count) {
    unsigned char* destination = (unsigned char*)dest;
    const unsigned char* source = (const unsigned char*)src;
    while (count-- != 0) {
      *destination++ = *source++;
    }

    return dest;
}

void __chkstk(void) {
   
}

int _fltused = 0x9875;
