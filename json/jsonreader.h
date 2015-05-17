#ifndef __JSON__READER__H
#define __JSON__READER__H

#define nullptr NULL
#define INVALID_POS 0xFFFFFFFF

#include "jsonstring.h"

typedef struct JsonReader
{
    char *buff ;
    size_t total_sz;
    size_t curr_pos ;
	char errbuff[1024] ;	
}JsonReader;
#endif
