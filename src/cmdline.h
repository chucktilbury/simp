
#ifndef _CMDLINE_H_
#define _CMDLINE_H_

#include <stdbool.h>

//TODO: Add groups for mutual exclusion and mutual requirement.
typedef enum {
    CL_NONE = 0,        // no flags
    CL_REQD = 0x01,     // parameter required
    CL_FL_REQD = 0x02,  // file list required
    CL_FL_ONE = 0x04,   // limit file list to one file
    CL_NO_FL = 0x08,    // file list is a syntax error
    CL_PRESENT = 0x10,  // set by the parser to check for required params
} CLFlags;

typedef void (*callback_w)(const char*);
typedef void (*callback_wo)();

void initCmdLine(CLFlags flags, const char* intro);
void destroyCmdLine();
void parseCmdLine(int argc, char** argv);
void showUseCmdLine();

void addNumParam(const char* param, const char* name, const char* help, int def, CLFlags flags);
void addStrParam(const char* param, const char* name, const char* help, const char* def, CLFlags flags);
void addTogParam(const char* param, const char* name, const char* help, bool def, CLFlags flags);
void addCBwParam(const char* param, const char* help, callback_w def, CLFlags flags);
void addCBwoParam(const char* param, const char* help, callback_wo def, CLFlags flags);

int getNumParam(const char* name);
const char* getStrParam(const char* name);
bool getTogParam(const char* name);

void resetCLFileList();
const char* iterateCLFileList();

#endif
