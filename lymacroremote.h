#ifndef LYMACROREMOTE_H
#define LYMACROREMOTE_H

bool  connectHost(unsigned int* handle, char * ip);
void  closeConnect(unsigned int handle);

bool  readMacro(unsigned int handle, int * macro, double * value, int count);
bool  writeMacro(unsigned int handle, int * macro, double * value, int count);
bool  readNCName(unsigned int handle, char * filename, int count); //count should not more than 64.

#endif    // LYMACROREMOTE_H

