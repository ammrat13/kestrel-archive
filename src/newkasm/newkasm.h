
#define MAXBITS 96
#define AIMMMIN -128
#define AIMMMAX 255
#define CIMMMAX 65535
#define MAXCODE 100000
#define MAXSYM  50000
#define MAXSYMS "50000"
#define MAXMACRO 100000
#define MAXMACROS "10000"
#define MAXARGS 100
#define MAXARGSS "100"
#define MAXLINE 1024
#define MAXEXPANDLINE 1024
#define PRINTERR 1
#define NOPRINTERR 0
#define MACRO 1
#define DEFINE 2
#define LABEL 3
#define stringize(a) #a
#define ZFREE(_a) if (_a) {free(_a) ; (_a) = NULL;}
#define ISSET(c) (c == '0' || c == '1')
#define MEFIELDSTART(_f)  (me->bits[field[fnum._f].start])
typedef struct opcfield {
  int id; char *name; int group; int start; int end; char *deflt;
  char *doc; int *fnpt; } opcfield;
typedef struct code_entry {
  char bits[MAXBITS];
  char *lstring;
  int context;     /* Context of this line of code */
  int  valid;
  int index;      /* I believe that this has to do with loops */
  char *cimm;     /* cimmediate value is a label */
  int target;
  char *eline;
  char *oline;    /* original line */
  int uses;
} mcode_entry;
/* The opc array provides definitions of the opcodes */
typedef struct opc {
  /* Index and name */
  int id; char *name;
  /* opcodes can declare controller part, kestrel part, or a specific field */
  char *cdef; char *kdef; char *fdef;
  int *fpt;
  char *longname, *operation, *doc, *comments;
  int *onpt;
  int (*fcall)();
  int ownline;			/* Must be on own line (fcall) */
  char bits[MAXBITS];  /* compression of the input format removing spaces etc */
  int type;			/* Used by genmanual to categorize */
} opcodes;

typedef struct symbol_entry {
  char *name;
  int context;    /* context of this entry */
  int defcontext; /* context of the defined line (<> c for macros) */
  char *lstring;  /* defined line */
  int type;
  int  id;         /* for instruction labels */
  char *value;     /* for defines */
  int mline;       /* for labels */
  char *fname;	   /* File and line number of macro */
  int lnum;
  int  macrolines; /* Number of lines in macro */
  char **macro;
  char **args;
  int  narg;
} symbol_entry;

/* Default instructions:  force, mova in func field */
#define KESTRELX "w zzwzw x x x x xwxxx xxxx xx x x xxx xxxxxx xxxxxx xxxxxx xxxxxxxx"
#define KESTRELTRUEX "x xxxxx x x x x xxxxx xxxx xx x x xxx xxxxxx xxxxxx xxxxxx xxxxxxxx"
/* Leftdatasel, rightdatasel, immmux */
#define CONTROLX "xx w xxx x xxxxxxxxxxxxxxxx x x x x w w x xx x x xx x x x x x x xx"
#define CONTROLTRUEX "xx x xxx x xxxxxxxxxxxxxxxx x x x x x x x xx x x xx x x x x x x xx"
#define KESNULL NULL
#define CONTNULL NULL
