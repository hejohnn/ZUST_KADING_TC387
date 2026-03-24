#ifndef _BASE64_H  
#define _BASE64_H  
  
#include <stdlib.h>  
#include <string.h>  
#include <stdio.h>  
#include <stdint.h>
  
void base64_encode(uint8_t *input, char *output, uint32_t len);
  
#endif  