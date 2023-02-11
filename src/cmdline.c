
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "memory.h"

typedef enum {
    CL_TOGGLE,
    CL_NUMBER,
    CL_STRING,
    CL_CB_W,
    CL_CB_WO,
} CLType;

typedef struct _cmd_line_elem {
    CLType type;
    const char* name;
    const char* param;
    const char* help;
    CLFlags flags;
    union {
        int num;
        const char* str;
        bool bval;
        callback_w with;
        callback_wo without;
    } data;
    struct _cmd_line_elem* next;
} _cmd_line_elem;

typedef struct _file_list_elem {
    const char* str;
    struct _file_list_elem* next;
} _file_list_elem;

typedef struct {
    _cmd_line_elem* cfirst;
    _cmd_line_elem* clast;
    _file_list_elem* ffirst;
    _file_list_elem* flast;
    _file_list_elem* crnt;
    const char* intro;
    const char* fname;
    CLFlags flags;
} _cmd_line;

static _cmd_line* cmds = NULL;

void initCmdLine(CLFlags flags, const char* intro)
{
    cmds = _alloc_ds(_cmd_line);

    cmds->cfirst = NULL;
    cmds->clast = NULL;
    cmds->ffirst = NULL;
    cmds->flast = NULL;
    cmds->crnt = NULL;
    cmds->intro = _copy_str(intro);
    cmds->flags = flags;

}

void destroyCmdLine()
{
    if(cmds == NULL)
        return;

    _cmd_line_elem* crntc;
    _cmd_line_elem* nextc;
    if(cmds->cfirst != NULL) {
        for(crntc = cmds->cfirst; crntc != NULL; crntc = nextc) {
            nextc = crntc->next;
            if(crntc->type == CL_STRING)
                _free((void*)crntc->data.str);
            _free((void*)crntc->name);
            _free((void*)crntc->param);
            _free((void*)crntc->help);
            _free(crntc);
        }
    }

    _file_list_elem* crntf;
    _file_list_elem* nextf;
    if(cmds->ffirst != NULL) {
        for(crntf = cmds->ffirst; crntf != NULL; crntf = nextf) {
            nextf = crntf->next;
            _free((void*)crntf->str);
            _free(crntf);
        }
    }
}

static inline _cmd_line_elem* find_param(const char* param)
{
    _cmd_line_elem* crnt;
    for(crnt = cmds->cfirst; crnt != NULL; crnt = crnt->next) {
        size_t len = strlen(crnt->param);
        if(strncmp(param, crnt->param, len) == 0)
            break;
    }

    return crnt;
}

static inline void add_file(const char* fn)
{
    _file_list_elem* fe = _alloc_ds(_file_list_elem);
    fe->str = _copy_str(fn);

    if(cmds->ffirst != NULL)
        cmds->flast->next = fe;
    else
        cmds->ffirst = fe;
    cmds->flast = fe;
}

void parseCmdLine(int argc, char** argv)
{
    int index = 0;
    const char* arg;

    // save the program name
    cmds->fname = _copy_str(argv[index]); // _copy_str() is a macro
    index++;

    while(index < argc) {
        // find the parameter
        _cmd_line_elem* param = find_param(argv[index]);
        if(param == NULL) {
            // if it's not found, add it to the file list
            add_file(argv[index++]);
            continue;
        }
        else {
            // check to see if a space separates the parameter from the arg
            size_t len = strlen(param->param);
            if(param->type == CL_TOGGLE || param->type == CL_CB_WO) {
                // there is no arg, just index to the next one.
                arg = NULL;
                index++;
            }
            else if(strlen(argv[index]) > len) {
                // there is no space
                arg = &argv[index][len];
                index++;
            }
            else {
                // there is a space.
                index++;    // index the arg
                arg = argv[index];
                index++;    // index the next param
            }
        }

        // parse the parameter arguments
        switch(param->type) {
            case CL_TOGGLE:
                param->data.bval = param->data.bval? false: true;
                param->flags |= CL_PRESENT;
                break;
            case CL_CB_WO:
                param->data.without();
                param->flags |= CL_PRESENT;
                break;
            case CL_NUMBER:
                if(arg != NULL)
                    param->data.num = strtol(arg, NULL, 10);
                else {
                    if(index > argc) {
                        fprintf(stderr, "missing argument for numeric command\n");
                        showUseCmdLine();
                    }
                    param->data.num = strtol(argv[index++], NULL, 10);
                }
                param->flags |= CL_PRESENT;
                break;
            case CL_STRING:
                param->data.str = _copy_str(arg);
                param->flags |= CL_PRESENT;
                break;
            case CL_CB_W:
                param->data.with(arg);
                param->flags |= CL_PRESENT;
                break;
            default:
                fprintf(stderr, "internal cmd error: unknown type: %d\n", param->type);
                exit(1);
        }
    }

    // verify that required params are present
    for(_cmd_line_elem* cel = cmds->cfirst; cel != NULL; cel = cel->next) {
        if((cel->flags & CL_REQD) && !(cel->flags & CL_PRESENT)) {
            fprintf(stderr, "cmd line error: required parameter '%s' is not found\n", cel->param);
            showUseCmdLine();
        }
    }

    if(cmds->flags & CL_NO_FL) {
        if(cmds->ffirst != NULL) {
            fprintf(stderr, "cmd line error: extra parameters on command line\n");
            showUseCmdLine();
        }
    }
    else {
        if(cmds->flags & (CL_FL_REQD|CL_FL_ONE)) {
            if(cmds->ffirst == NULL) {
                fprintf(stderr, "cmd line error: required file list is not found\n");
                showUseCmdLine();
            }
        }

        if(cmds->flags & (CL_FL_ONE)) {
            if(cmds->ffirst != cmds->flast) {
                fprintf(stderr, "cmd line error: exactly one file name required\n");
                showUseCmdLine();
            }
        }
    }
}

void showUseCmdLine()
{
    fprintf(stderr, "\nuse: %s <parameters> %s\n", cmds->fname, cmds->flags & CL_NO_FL? "": "<file(s)>");
    fprintf(stderr, "%s\n", cmds->intro);

    for(_cmd_line_elem* cel = cmds->cfirst; cel != NULL; cel = cel->next) {
        switch(cel->type) {
            case CL_TOGGLE:
            case CL_CB_WO:
                fprintf(stderr, "    %s ----- %s %s\n", cel->param, cel->help, cel->flags & CL_REQD? "(reqd)": "");
                break;
            case CL_NUMBER:
                fprintf(stderr, "    %s <num> %s %s\n", cel->param, cel->help, cel->flags & CL_REQD? "(reqd)": "");
                break;
            case CL_STRING:
            case CL_CB_W:
                fprintf(stderr, "    %s <str> %s %s\n", cel->param, cel->help, cel->flags & CL_REQD? "(reqd)": "");
                break;
            default:
                fprintf(stderr, "internal cmd error: unknown type: %d\n", cel->type);
                exit(1);
        }
    }
    fprintf(stderr, "\n\n");
    exit(1);
}

static inline _cmd_line_elem* create_cel(CLType type, const char* param, const char* name, const char* help)
{
    _cmd_line_elem* cel = _alloc_ds(_cmd_line_elem);

    cel->param = _copy_str(param);
    cel->help = _copy_str(help);
    if(name == NULL)
        cel->name = _copy_str("");
    else
        cel->name = _copy_str(name);
    cel->type = type;

    return cel;
}

static inline void add_cel(_cmd_line_elem* cel)
{
    if(cmds->cfirst != NULL)
        cmds->clast->next = cel;
    else
        cmds->cfirst = cel;
    cmds->clast = cel;
}

void addNumParam(const char* param, const char* name, const char* help, int def, CLFlags flags)
{
    _cmd_line_elem* cel = create_cel(CL_NUMBER, param, name, help);
    cel->data.num = def;
    cel->flags = flags;

    add_cel(cel);
}

void addStrParam(const char* param, const char* name, const char* help, const char* def, CLFlags flags)
{
    _cmd_line_elem* cel = create_cel(CL_STRING, param, name, help);
    cel->data.str = _copy_str(def);
    cel->flags = flags;

    add_cel(cel);
}

void addTogParam(const char* param, const char* name, const char* help, bool def, CLFlags flags)
{
    _cmd_line_elem* cel = create_cel(CL_TOGGLE, param, name, help);
    cel->data.bval = def;
    cel->flags = flags;

    add_cel(cel);
}

void addCBwParam(const char* param, const char* help, callback_w def, CLFlags flags)
{
    _cmd_line_elem* cel = create_cel(CL_CB_W, param, NULL, help);
    cel->data.with = def;
    cel->flags = flags;

    add_cel(cel);
}

void addCBwoParam(const char* param, const char* help, callback_wo def, CLFlags flags)
{
    _cmd_line_elem* cel = create_cel(CL_CB_WO, param, NULL, help);
    cel->data.without = def;
    cel->flags = flags;

    add_cel(cel);
}

static inline _cmd_line_elem* find_name(const char* name)
{
    _cmd_line_elem* crnt;
    for(crnt = cmds->cfirst; crnt != NULL; crnt = crnt->next) {
        if(!strcmp(name, crnt->name))
            break;
    }

    return crnt;
}

int getNumParam(const char* name)
{
    _cmd_line_elem* crnt = find_name(name);

    if(crnt != NULL)
        return crnt->data.num;
    else {
        fprintf(stderr, "internal cmd error: unknown name: %s\n", name);
        exit(1);
    }
    return 0; // happy compiler
}

const char* getStrParam(const char* name)
{
    _cmd_line_elem* crnt = find_name(name);

    if(crnt != NULL)
        return crnt->data.str;
    else {
        fprintf(stderr, "internal cmd error: unknown name: %s\n", name);
        exit(1);
    }
    return NULL; // happy compiler
}

bool getTogParam(const char* name)
{
    _cmd_line_elem* crnt = find_name(name);

    if(crnt != NULL)
        return crnt->data.bval;
    else {
        fprintf(stderr, "internal cmd error: unknown name: %s\n", name);
        exit(1);
    }
    return false; // happy compiler
}

void resetCLFileList()
{
    cmds->crnt = cmds->ffirst;
}

const char* iterateCLFileList()
{
    _file_list_elem* crnt = cmds->crnt;
    if(crnt != NULL)
        cmds->crnt = cmds->crnt->next;

    if(crnt != NULL)
        return crnt->str;
    else
        return NULL;
}
