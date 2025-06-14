/************************************************************************\
 *                                                                      *
 *   NAME:                                                              *
 *       newkasm.c                                                      *
 *                                                                      *
 *   PURPOSE:                                                           *
 *       Kasm assembler for 96-bit kasm, modifiable to 64-bit code      *
 *                                                                      *
 *   FUNCTIONS:                                                         *
 *                                                                      *
 *   HISTORY:                                                           *
 *       Richard Hughey July 1998                                       *
 *                                                                      *
\************************************************************************/

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
//#include <malloc.h>
#include "newkasm.h"
#include "codetable.h"

#ifndef KESTRELLIB
#define KESTRELLIB "/projects/kestrel/lib/kasm"
#endif

#define DEBUG 1
#define ANNOTATE 1
#define NOANNOTATE 0
#define SYMSYMBOL(_c) ((isalpha((int)(_c)) || ((_c) == '_')))

#define ERRORX0(_s,_l)          fprintf(stderr,"%s: " _s,(_l))
#define ERRORX1(_s,_l,_a)       fprintf(stderr,"%s: " _s,(_l),(_a))
#define ERRORX2(_s,_l,_a,_b)    fprintf(stderr,"%s: " _s,(_l),(_a),(_b))
#define ERRORX3(_s,_l,_a,_b,_c) fprintf(stderr,"%s: " _s,(_l),(_a),(_b),(_c))
#define ERRORX4(_s,_l,_a,_b,_c,_d) fprintf(stderr,"%s: " _s,(_l),(_a),(_b),(_c),(_d))
#define ERROR0(_s)          ERRORX0(_s, me->lstring)
#define ERROR1(_s,_a)       ERRORX1(_s, me->lstring, _a)
#define ERROR2(_s,_a,_b)    ERRORX2(_s, me->lstring, _a, _b)
#define ERROR3(_s,_a,_b,_c) ERRORX3(_s, me->lstring, _a, _b, _c)
#define ERROR4(_s,_a,_b,_c,_d) ERRORX4(_s, me->lstring, _a, _b, _c,_d)


mcode_entry mcode[MAXCODE];
symbol_entry symtab[MAXSYM];
int context_parent[MAXMACRO];

int numsym = 0;
int numcode = 0;
int curcontext = 0;
int numcontexts = 1;
int conddepth = 0;		/* Depth of begincond/endcond pairs */
int verbose=0;
int dofile (char *file, char *rlstring);
void parseline (char *line, mcode_entry *me);
int fieldis (int fnum, mcode_entry *me, char *bits);

/* Intercepted automatically, but defined in opc.def for */
/* documentation. */
int KMacroDef (mcode_entry *me) {return 1;}
int KMacroEnd (mcode_entry *me) {return 1;}

/* flip( n, m ) flips instruction n, bit m */
int
flip( int n, int m )
{
  if( mcode[n].bits[m] == '1' || mcode[n].bits[m] == 'w' ){
    mcode[n].bits[m] = '0';
    return 0;
  }
  else if(mcode[n].bits[m] == '0' || mcode[n].bits[m] == 'z'){
    mcode[n].bits[m] = '1';
    return 1;
  } else {
    mcode[n].bits[m] = 'w';
    return 1;
  }
}
void
resolve_bits()
{
  int i, j, err;
  char *c;
  mcode_entry *me;
  
  for (j = 0; j < numcode ; j++){
    me = &(mcode[j]);
    err= 0;
    for (i = MAXBITS-1 ; i>= 0; i--) {
      c = &me->bits[i];
      if (*c == 'r') {
	if (! err) {
	  ERROR0("Missing required fields in bit conversion.\n");
	  err = 1;
	}
      } else if (*c == '1' || *c == 'w') {
	*c = '1';
      } else
	*c = '0';
      }
  }
}
     
#define not_next(_m,_i,_s) \
     {if (_m((_i+1))) { \
       ERRORX0(_s, mcode[_i].lstring);\
     } else { \
       j = mcode[_i].target;\
       if (_m(j)) {\
	 ERRORX2 (_s "\tSee jump target %s %s\n", mcode[_i].lstring,\
		  mcode[_i].cimm, mcode[j].lstring);\
						      }}}
#define not_next_two(_m,_i,_s) {\
  if (_m(_i+1) || _m(_i+2)) { \
    ERRORX0(_s, mcode[_i].lstring); \
  } else {\
    j = mcode[_i].target;\
    if (_m(j) || _m(j+1) || _m(mcode[j].target)) { \
	 ERRORX2 (_s "\tSee jump target %s %s\n", mcode[_i].lstring, \
		  mcode[_i].cimm, mcode[j].lstring); \
    } else { \
      j = mcode[_i+1].target; \
      if (_m(j)) { \
	 ERRORX2 (_s "\tSee jump target %s %s\n", mcode[_i].lstring, \
		  mcode[_i+1].cimm, mcode[j].lstring); \
      }}}}
#define is_wor(_i) (((_i)<numcode) && ((_i) > 0) &&\
		    (fieldis(fnum.br_w_or,&mcode[_i],"1") || \
		     fieldis(fnum.bs_load,&mcode[_i],"1") || \
		     fieldis(fnum.bs_sleft,&mcode[_i],"1")))
#define is_scrwarray(_i) (((_i)<numcode) && ((_i) > 0) &&\
			  (fieldis (fnum.scr_store, &mcode[_i], "1") &&\
			   fieldis (fnum.scr_sel, &mcode[_i], "00")))
#define is_scrwinternal(_i) (((_i) < numcode) && ((_i) > 0) && \
			     (fieldis (fnum.scr_store, &mcode[_i], "1") &&\
			      ! fieldis (fnum.scr_sel, &mcode[_i], "00")))
#define is_scrread(_i) (((_i) < numcode) && ((_i) > 0) && \
			((! fieldis (fnum.cnt_wr, &mcode[_i], "00")) ||\
			 (fieldis (fnum.right_d_sel, &mcode[_i], "0") && \
			  fieldis (fnum.left_d_sel, &mcode[_i], "0") && \
			  fieldis (fnum.data_read, &mcode[_i], "1"))))
void
check_code ()
{
  /* j is used by macros */
  int i, j;

  for (i=0 ; i < numcode ; i++) {
    /* Look for arrtoscr or arrtoq, which do a late write to the
       scratch register */
    if (is_scrwarray(i)) {
      not_next(is_scrwinternal, i,
     "Error:  controller write to scratch following\n\tarray write to scratch requires one intervening instruction.\n");
      not_next_two (is_scrread, i, 
      "Error:  controller read from scratch following\n\tarray write to scratch requires 2 intervening instructions.\n");
    }
    if (is_scrwinternal(i)) {
      not_next (is_scrread, i, 
      "Error:  controller read from scratch following\n\tcontroller write to scratch requires one intervening instruction.\n");
    }
  }
  /* Look for wor use too soon after changing bit-shifter/mask */
  for (i = 0 ; i < numcode ; i++) {
    if (! fieldis (fnum.bit, &mcode[i], "0000")) {
      not_next_two (is_wor, i, 
     "Error:  controller use of WOR requires stable\n\tbitshifter for 2 intervening instructions.\n");
    }
  }
}
void
correct_board_errors()
{
  int i;
  int bit;
  char tmp;

  for( i=0; i<numcode; i++ ){   /* for each instr, do these things... */

    /* exchanging bits 90 and 93 to compensate for board wiring error */
    /* comment out for new board */
    tmp = mcode[i].bits[90];
	mcode[i].bits[90] = mcode[i].bits[93];
    mcode[i].bits[93] = tmp;

    /* subtracting one from the cimm field for beginloop instructions */
    /* to compensate for board error (or unexpected behavior) */
    if( mcode[i].bits[4]=='1' && mcode[i].bits[5]=='0' && mcode[i].bits[6]
	=='0' && mcode[i].bits[7]=='0' ) /* if this is a beginloop instr... */
      for(bit=21; flip(i, bit); bit++); /* subtract one from the cimm field. */

  }
}
void
init_mcode ()
{
  int i;
  
  for (i = 0 ; i < MAXCODE ; i++) {
    mcode[i].valid = 0;
    mcode[i].cimm = NULL;
    mcode[i].lstring = NULL;
  }
}

mcode_entry *
init_code_line (char *lstring, int context, int cdepth)
{
  mcode_entry *me;
  char *c, *b;

  me = &(mcode[numcode]);
  ZFREE (me->lstring);
  me->lstring = strdup (lstring);
  me->valid = 0;
  me->context = context;
  ZFREE(me->eline);
  ZFREE(me->oline);
  me->valid = 1;
  ZFREE (me->cimm);
  me->target = -100;
  me->index = numcode;

  if (numcode >= MAXCODE-1) {
    ERROR1 ("Internal error -- more than %d lines of code\n",
	     MAXCODE);
    exit (1);
  }

  for (b=me->bits + MAXBITS-1, c = KESTRELX ; *c ; c++)
    if (*c != ' ')
      *b-- = *c;
  for (c = CONTROLX ; *c ; c++)
    if (*c != ' ')
      *b-- = *c;
  if (b != me->bits-1) {
    ERROR0 ("Internal error initializing code line to default\n");
    exit(1);
  }
  return me;
}
void
set_eline (mcode_entry *me, char *el, int v)
{
  if (v) {
    ZFREE(me->eline);
    me->eline = strdup (el);
  }
}
void
set_oline (mcode_entry *me, char *el, int v)
{
  if (v) {
    ZFREE(me->oline);
    me->oline = strdup (el);
  }
}
void
commit_code_line (mcode_entry *me)
{
  if (me != &(mcode[numcode])) {
    ERROR0 ("Internal error:  wrong code line passed to commit\n");
    exit(1);
  } else
    numcode++;
}
char *
tok_or_null (char *istring, mcode_entry *me)
{
  static char string[MAXEXPANDLINE];
  char *temp;          /* temp/indexing ptr. */ 
  char *return_value;  /* pointer to beginning of token to be returned*/
  static char *next_tok;  /* pointer to beginning of next token for next call*/
  static char c;   /* saved char that is replaced with a */

  if (istring) {
    strcpy (string, istring);
    next_tok = string;
    c = 0;
  }
  if (next_tok && c) {
    *next_tok = c;
  }
  c = 0;
  while (next_tok && *next_tok && strchr (" ,\t", *next_tok))
    next_tok++;
  if (! next_tok || ! *next_tok || *next_tok == '\n') {
    next_tok = NULL;
    return NULL;
  }
  temp = next_tok;
  return_value = next_tok;
  if (*temp == ':') {
    ERROR0 ("Colon must appear immediately after identifier\n");
    next_tok = NULL;
    return NULL;
  } 
  /* Single-character tokens */
  if (*temp == '+' || *temp == '(' || *temp == ')') {
    c = *(temp+1);
    *(temp+1) = '\0';
    next_tok = temp+1;
  } 
    /* multi-character tokens */
  else {
    while (*temp && ! (strchr (" ,\t\n():+", *temp)))
      temp++;
    if (*temp == ':')
      temp++;
    c = *temp;
    next_tok = temp;
    *temp = '\0';
  } 
  if (! return_value || ! *return_value)
    return NULL;
  else
    return return_value;
}
int 
lookup_sym (char *name, int context)
{
  int i = 0, rv, p;
  char *t;
  int foundi, foundc=0;

  rv = -1;
  foundi = -1;

  /* Symbols only match if same or global context (no nesting) */
  /* keep following pointers until we get out.  could be better done
     on entering data */
  for (t = name ; i >= 0 && t ; ) {
    foundi = -1; foundc = 0;
    for (i = numsym-1 ; i >= 0 ; i--) {
      if (i != rv && (strcasecmp (t, symtab[i].name) == 0)) {
	/* Local or any context match */
	if (symtab[i].context == context || context == -1)
	  break;
	p = context;
	/* Check parent for closer contexts */
	while ((p = context_parent[p]) &&
	       symtab[i].context != p  &&
	       p != foundc);
	if (symtab[i].context == p) {
	  foundc = p;
	  foundi = i;
	}
      }
    }
    if (i >= 0) {
      t = (symtab[i].type == DEFINE ? symtab[i].value : NULL);
      context = symtab[i].defcontext;
      rv = i;
    } else if (foundi >= 0) {
      i = foundi;
      t = (symtab[i].type == DEFINE ? symtab[i].value : NULL);
      context = symtab[i].defcontext;
      rv = i;
    }
  }
  return rv;
}
/* Look for a previous definition of the symbol */
void
check_sym (char *name, int context, mcode_entry *me)
{
  int i;
  for (i = numsym-1 ; i>=0 ; i--) {
    if (strcasecmp (name, symtab[i].name)==0) {
      if (symtab[i].context == context) {
	ERROR1 ("Symbol %s redefined in same context\n", name);
	ERRORX0 ("This is the previous definition\n",symtab[i].lstring);
      } else if (symtab[i].context == 0) {
	ERROR1 ("Warning:  Symbol %s hides global definition\n", name);
	ERRORX0 ("This is the previous definition\n",symtab[i].lstring);
      }
    }
  }
}
void
expand_line (char *to, char *from, mcode_entry *me)
{
  char *s;
  char tmp[MAXLINE];
  int sval;
  while (*from && ((*from == ' ') || (*from == '\t')))
    from++;
  while (*from && *from != '\n' && *from != ';') {
    if (*from != '$')
      *to++ = *from++;
    else {
      from++;
      for (s = tmp; SYMSYMBOL(*from) ; *s++ = *from++);
      *s = '\0';
      if (*from == '$') /* inline expansion */
	from++;

     sval = lookup_sym (tmp, me->context);
      if (sval < 0)
        ERROR1("Undefined symbol (%s) ignored.\n", tmp);
      else {
        if (symtab[sval].type == LABEL || symtab[sval].type ==  MACRO) {
          /* this is the case in a backwards jump, the label is
             already set, but labels don't use the value, so use name.*/
          for (s = symtab[sval].name ; *s && *s != ';' ; *to++ = *s++);
        } else if (symtab[sval].type == DEFINE) {
          /* hope it is a define! */
          for (s = symtab[sval].value ; *s && *s != ';' ; *to++ = *s++);
        } else {
	  ERROR2 ("Undefined symbol type for %s (%d)\n",
		  tmp, symtab[sval].type);
	}
      }
    }
  }
  *to = '\0';
  set_eline (me, to, verbose);
  set_oline (me, from, verbose);
}
int
add_define (char *name, int context, int defcontext, char *def, mcode_entry *me)
{
  char *s;

  if (!name || !*name) {
    ERROR0("Define missing name (case-insensitive letters and underscores)\n");
    return -1;
  }
  if (!def || !*def) {
    ERROR1 ("Define of %s missing definition\n", name);
    return -1;
  }
  for (s = name ; *s ; s++) {
    if (! SYMSYMBOL(*s)) {
      ERROR1 ("Symbol names (%s) only include letters and underscores\n",
	       name);
      return -1;
    }
  }
  check_sym (name, context, me);
  for ( ;  *def && strchr (" \t", *def) ; def++) {;}
  symtab[numsym].name = strdup (name);
  symtab[numsym].context = context;
  symtab[numsym].defcontext = defcontext;
  symtab[numsym].type = DEFINE;
  symtab[numsym].value = strdup (def);
  symtab[numsym].lstring = strdup (me->lstring);
  numsym++;
  if (numsym >= MAXSYM) {
    ERROR1 ("Internal error -- More than %d symbols\n", MAXSYM);
    exit(1);
  }
  return numsym-1;
}
int
add_label (char *name, int context, mcode_entry *me, int mline)
{
  int tlen = strlen(name);
  if (name[tlen-1] == ':')
    name[tlen-1] = '\0';
  
  if (! name || ! *name) {
    ERROR0("Null label ignored\n");
    return -1;
  }
  check_sym (name, context, me);
  symtab[numsym].name = strdup (name);
  symtab[numsym].context = context;
  symtab[numsym].defcontext = context;
  symtab[numsym].type = LABEL;
  symtab[numsym].value = NULL;
  symtab[numsym].lstring = strdup (me->lstring);
  symtab[numsym].mline = mline;
  numsym++;
  return numsym-1;
}
int
add_macro (char *name, int context, mcode_entry *me)
{
  check_sym (name, context, me);
  symtab[numsym].name = strdup (name);
  symtab[numsym].context = context;
  symtab[numsym].defcontext = context;
  symtab[numsym].type =  MACRO;
  symtab[numsym].lstring = strdup (me->lstring);
  symtab[numsym].macro = (char **) malloc (sizeof (char *) * MAXMACRO);
  symtab[numsym].args = (char **) malloc (sizeof (char *) * MAXARGS);
  numsym++;
  return (numsym-1);
}

/* 
 * return value appears to be the index of the opcode.
 * if a match is not found, negative one is returned.
 */
int
lookup_opcode (char *opcode)
{
  int i = 0;
  for (i = 0 ; i < numopc && strcasecmp (opcode, opc[i].name) ; i++);
  if (i == numopc)
    i = -1;

  return i;
}
int
lookup_field (char *f)
{
  int i = 0;
  for (i = 0 ; i < numfield && strcasecmp (f, field[i].name) ; i++);
  if (i == numfield)
    i = -1;

  return i;
}

int
define_macro (char *line, FILE *file, char *fname, int lnum,
	      mcode_entry *me)
{
  int i = 0, tl, nm;
  int white;
  char *t;
  int found = 0;
  char *lcopy;
  
  lcopy = strdup(line);
  t = strtok (lcopy, " \t,\n()");
  t = strtok (NULL, " \t,\n()");
  if (! t || ! *t) {
    ERROR0 ("Macro does not have a name");
    return lnum;
  }
  nm = add_macro (t, curcontext, me);
  while ((t = strtok(NULL, " \t,\n(")) && (i < MAXARGS)) {
    tl = strlen (t);
    if (t[tl-1] == ')') {
      found = 1;
      t[tl-1] = '\0';
    }
    if (*t)
      symtab[nm].args[i++] = strdup (t);
  }
  symtab[nm].narg = i;
  if (t) {
    ERROR2 ("Too many arguments (> %d) to macro %s\n",
	     MAXARGS, symtab[nm].name);
  }
  if (! found)
    ERROR1 ("Did not find right parenthesis for macro %s\n",
	     symtab[nm].name);

  free (lcopy);
  symtab[nm].macro[0] = strdup (line);
  symtab[nm].lnum = lnum;
  symtab[nm].fname = strdup (fname);
  found = 0;  i = 1;
  while (fgets (line, MAXLINE, file) && i < MAXMACRO) {
    lnum++;
    white = strspn (line, " \t");
    if (strncasecmp (&(line[white]), "MacroEnd", 8) == 0){
      found = 1;
      break;
    } else
      symtab[nm].macro[i++] = strdup (line);
  }
  if (! found) {
    if (i == MAXMACRO)
      ERROR2 ("Macro %s too long (limit is %d lines)\n",
	       symtab[nm].name, MAXMACRO);
    else
      ERROR1 ("MacroEnd not found for macro %s\n", symtab[nm].name);
  }
  symtab[nm].macrolines = i;
  symtab[nm].macro = (char **)
    realloc ((void *) symtab[nm].macro, (i) * sizeof (char *));
  return lnum;
}
char *
grabfield (char *buf, int fnum, char *bits)
{
  int k, j;

  for (k=0, j = field[fnum].end ; j >= field[fnum].start ; k++, j--){
    buf[k] = bits[j];
  }
  buf[k] = '\0';
  return buf;
}
int
fieldis (int fnum, mcode_entry *me, char *bits)
{
  int k, j;
  int rval = 1;

  for (k=0, j = field[fnum].end ; j >= field[fnum].start ; k++, j--){
    if (! bits || ! bits[k]) {
      fprintf (stderr, "Invalid call to fieldis (%d %s)\n",
	       fnum, bits);
      exit(1);
    }
    if (bits[k] != me->bits[j]) {
      rval = 0;
    }
  }
  return rval;
}
void
checkline (mcode_entry *me)
{
  int i;
  char c;
  int ferr=-1;
  char buf[MAXBITS];
  
  for (i = MAXBITS-1 ; i>= 0; i--) {
    c = me->bits[i];
    if (c == 'r') {
      if (ferr != bits_to_field [i]) {
	ferr = bits_to_field[i];
	if (ferr != fnum.cont_imm || me->cimm == NULL)
	  ERROR2 ("Field %s must be fully specified (%s)\n",
		  field[bits_to_field[i]].name,
		  grabfield (buf, ferr, me->bits));
      }
    }
  }
}
/*
 * return values: 0 -> conflict
 *                1 -> works
 *

Existing    ! = replace, ? = error, * = error if don't match, blank == no change
Code char        Opcode char (oc)
      x   0,1   w,z   a,b   A,B   e   E   l   r   u
 x         !     !     !     !    !   !   !   !   !    Don't care
 0,1       *                 ?        ?           ?    Set to 0 or 1 
 w,z       !     !     !     !    !   !   !   !   !    Defaults to 0/1
 a,b       !     !     !     !    !   !   ?   !   ?    Replacable A,B in funct (alu output required)
 A,B       ?                 *        ?   ?       ?    1/0 if opA first, 0/1 if opB first (func only)
 e         !     !     ?     ?        !   ?   !   ?    Replacable E (opA/opB only)
 E         !           ?     ?            ?   ?   ?    Either opA or opB but not both
 l         !     ?     ?     ?    ?   ?       ?   ?    Lable (cimm only)
 r         !                 !    ?   ?   ?       ?    Required
 u         ?                 ?        ?   ?   ?        Unused field
 */
int
mergebits (char oc, char *mc)
{
  if (oc == *mc || oc == 'x')
    return 1;
  switch (*mc) {
  case 'x': case 'w': case 'z':
    *mc = oc;
    return 1;
  case '0': case '1':
    if (strchr ("10ABE", oc))
      return 0;
    return 1;
  case 'a': case 'b':
    if (strchr ("lu", oc))
      return 0;
    *mc = oc;
    return 1;
  case 'A': case 'B':
    if (strchr ("01ABEElu",oc))
      return 0;
    return 1;
  case 'e':
    if (strchr ("abABlu",oc))
      return 0;
    *mc = oc;
    return 1;
  case 'E':
    if (strchr ("abABlru",oc))
      return 0;
    if (strchr ("01",oc))
      *mc = oc;
    return 1;
  case 'l':
    if (! strchr ("01",oc)) 
      return 0;
    *mc = oc;
    return 1;
  case 'r':
    if (strchr ("Elu",oc))
      return 0;
    if (strchr ("01AB",oc))
      *mc = oc;
    return 1;
  case 'u':
    if (strchr ("01ABElr",oc))
      return 0;
    return 1;
  default:
    ERRORX1 ("Newkasm error in mergebits (%c)\n",  "Internal", *mc);
    return 0;
  }
}
/* 
 * I believe that fnum is the number of the field as designated in fields.def.
 */
int 
merge_int_to_field (int val, mcode_entry *me, int fnum, int printerr)
{
  int i, j, rval = 1;
  char f1[MAXBITS];
  grabfield (f1, fnum, me->bits);
  for (i = 1, j = field[fnum].start ; j <= field[fnum].end ; j++, i *= 2) {
    rval = mergebits ((val & i ? '1' : '0'), &(me->bits[j])) && rval;
  }
  if (! rval && printerr)
    ERROR3("Conflict merging integer %d into field %s (%s)\n", 
	   val, field[fnum].name, f1);
  return rval;
}
int 
merge_lab_to_field (int val, mcode_entry *me, char *name)
{
  int i, j, rval = 1;
  char c;
  char f1[MAXBITS];
  int f = fnum.cont_imm;
  grabfield (f1, f, me->bits);
  for (i = 1, j = field[f].start ; j <= field[f].end ; j++, i *= 2) {
    c = (val & i) ? '1' : '0';
    if (me->bits[j] == 'l' || me->bits[j] == c)
      me->bits[j] = c;
    else {
      rval = 0;
    }
  }
  if (! rval)
    ERROR2("Label %s not required by instruction (Cimm = %s).\n", name, f1);
  return rval;
}

int 
merge_string_to_field (char *val, mcode_entry *me, int fnum, int printerr)
{
  int i, j, rval = 1;
  char f1[MAXBITS];
  grabfield (f1, fnum, me->bits);
  for (i = 0, j = field[fnum].end ; j >= field[fnum].start ; j--,i++)
    rval = mergebits (val[i], &(me->bits[j])) && rval;
  if (! rval && printerr)
    ERROR3("Conflict merging %s into field %s (%s)\n", 
	   val, field[fnum].name, f1);
  return rval;
}
void
merge_opcode (mcode_entry *me, int n)
{
  int ferr = -1, errnum = 0, i;
  char f1[MAXBITS], f2[MAXBITS];

  if (n < 0)
    return;

  for (i = MAXBITS-1 ; i>= 0 ; i--) {
    if (! mergebits (opc[n].bits[i], &(me->bits[i]))) {
      if (bits_to_field[i] != ferr && errnum < 3) {
	ferr = bits_to_field[i];
	errnum++;
	ERROR4 ("Conflict in %s between %s (%s) and previously set value (%s).\n",
		 field[ferr].name,
		 opc[n].name,grabfield(f1, ferr, opc[n].bits),
		 grabfield(f2, ferr, me->bits));
      }
    }
  }
}
int
merge_aimmediate (char *s, mcode_entry *me, int printerr)
{
  char *ept;
  int ival;
  
  if (s[0] != '#')
    return 0;
  s++;
  if (! isdigit((int) *s)) {
    if (strcasecmp (s, "scr") == 0) {
      merge_opcode (me, onum.Kscrtoimm);
    } else
      ERROR1("Undefined immediate symbol %s (missing $?)\n", s);
    return 1;  /* 1 says it was parsed as an immediate */
  }
  ival = strtol (s, &ept, 0);
  if (ept == s) {
    ERROR1("No number found in # %s declaration\n", s);
  } else if (ival > AIMMMAX || ival < AIMMMIN) {
    ERROR1("Number (%d) out of range in # declaration\n", ival);
  }
  merge_int_to_field (ival, me, fnum.imm, printerr);
  return 1;
}
int
merge_cimmediate (char *s, mcode_entry *me, int printerr)
{
  int ival;
  char *ept;

  ival = strtol (s, &ept, 0);
  if (ept != s) {
    merge_int_to_field (ival, me, fnum.cont_imm, PRINTERR);
  } else {
    if (me->cimm != NULL) {
      ERROR2("Multiple cimm labels (%s and %s)\n", me->cimm, s);
    } else {
      me->cimm = strdup (s);
    }
  }
  return 1;
}
int
resolve_labels ()
{
  int i, s;
  int rval = 1;
  mcode_entry *me;

  for (i = 0 ; i < numcode ; i++) {
    if (mcode[i].cimm != NULL) {
      me = &(mcode[i]);
      s = lookup_sym (me->cimm, me->context);
      if (s < 0) {
	rval = 0;
	ERROR1 ("Label %s cannot be resolved\n",me->cimm);
	s = lookup_sym (me->cimm, -1);
	if (s >= 0)
	  ERRORX2 ("This matching label is in the wrong context (%d not %d)\n",
		   symtab[s].lstring, symtab[s].context, me->context);
      } else if (symtab[s].type != LABEL) {
	  rval = 0;
	  ERROR2 ("Label %s is of type %s rather than a label\n",
		   mcode[i].cimm, type_name[symtab[s].type]);
	  
      } else {
	me->target = symtab[s].mline;
	merge_lab_to_field (symtab[s].mline,  &(mcode[i]), mcode[i].lstring);
      }
    }
  }
  return rval;
}

int
regnum (char *s)
{
  int reg, lr;
  char *spt;
  if (s[0] == 'L' || s[0] == 'l')
    lr = 0;
  else if (s[0] == 'R' || s[0] == 'r')
    lr = 1;
  else
    return -1;
  if (! isdigit ((int) s[1]))
    return -1;
  reg = strtol (s+1, &spt, 10);
  if (*spt != '\0')
    return -1;
  if (reg < 0 || reg > 31)
    return -1;
  if (lr == 1)
    reg += 32;
  return reg;
}
/* Check to see if there is an A/B resolution issue and if either A or B
   is set   */ 
void
resolveAB (mcode_entry *me)
{
  int haveA = 0, haveB = 0;
  int i;

  haveA = ISSET (MEFIELDSTART (opA));
  haveB = ISSET (MEFIELDSTART (opB));

  /* If we have both or don't have either, nothing to do */
  if (! haveA ^ haveB)
    return;

  /* If only 1 of opA/opB is used, clear the other field */
  if (MEFIELDSTART (opB) == 'E' || MEFIELDSTART (opB) == 'e') {
    for (i = field[fnum.opB].start ; i <= field[fnum.opB].end ; i++)
      me->bits[i] = 'u';
  } else if (MEFIELDSTART (opA) == 'E' || MEFIELDSTART (opA) == 'e') {
    for (i = field[fnum.opA].start ; i <= field[fnum.opA].end ; i++)
      me->bits[i] = 'u';
  }
  for (i = field[fnum.func].start ; i <= field[fnum.func].end ; i++) {
    if (me->bits[i] == 'a' || me->bits[i] == 'A')
      me->bits[i] = haveA ? '1' : '0';
    else if (me->bits[i] == 'b' || me->bits[i] == 'B')
      me->bits[i] = haveB ? '1' : '0';
  }
}
     
int
parseoperand (char *tok, mcode_entry *me, int isC)
{
  int reg;
  int haveA = 0, haveB = 0, /*haveC = 0,*/ haveD = 0;
  int needA = 0, needB = 0, needC = 0, needAorB = 0, needD = 0;
  int rval = 0;

  haveA = ISSET (MEFIELDSTART (opA));
  haveB = ISSET (MEFIELDSTART (opB));
  /*haveC = ISSET (MEFIELDSTART (opC));*/
  haveD = ISSET (MEFIELDSTART (dest));
  needA = MEFIELDSTART (opA) == 'r';
  needB = MEFIELDSTART (opB) == 'r';
  needC = MEFIELDSTART (opC) == 'r';
  needD = MEFIELDSTART (dest) == 'r';
  needAorB = (MEFIELDSTART (opB) == 'E' || MEFIELDSTART (opB) == 'e');

  /* Pure registers, checking destination first */
  reg = regnum (tok);
  if (reg >= 0) {
    if (! isC && needD && ! haveD) {
      merge_int_to_field (reg, me, fnum.dest, PRINTERR);
      rval = 1;
    } else if (! isC && (needA||needAorB) && ! haveA) {
      merge_int_to_field (reg, me, fnum.opA, PRINTERR);
      rval = 1;
    } else if (! isC && needB && ! haveB) {
      rval = 1;
      if (! merge_int_to_field (reg, me, fnum.opC, NOPRINTERR))
	ERROR0("opB and opC cannot be assigned different registers\n");
      merge_opcode (me, onum.KopBreg);
    } else if (isC || needC) {
      rval = 1;
      merge_int_to_field (reg, me, fnum.opC, PRINTERR);
    } else {
      ERROR1("%s is an extra register for this instruction.\n", tok);
      rval = 1; /* used up token */
    }
  } else {
    /* Deal with immediate and signextend register cases */
    if ((tok[0] == 'S' || tok[0] == 's') &&
	((reg = regnum (tok+1)) >= 0)) {
      if (! merge_int_to_field (reg, me, fnum.opC, NOPRINTERR))
	ERROR0("opB and opC cannot be assigned different registers\n");
      merge_opcode (me, onum.KopBsreg);
      rval = 1;
    } else if (merge_aimmediate (tok, me, PRINTERR)) {
      merge_opcode (me, onum.KopBimm);
      rval = 1;
    } 
  }
  return (rval);
}
int
parsereadwrite (int is_read, mcode_entry *me)
{
  char *t;
  int hreg = 0, himm = 0;

  /* {read,write}(reg+imm) */

  t = tok_or_null (NULL, me);
  if (*t != '(') {
    ERROR1("%s must be followed by left parenthesis\n", is_read ? "read" : "write");
    return 1;
  }
  t = tok_or_null (NULL, me);
  while (t && *t != ')') {
    if (merge_aimmediate (t, me, PRINTERR)) {
      himm = 1;
    } else {
      if (! parseoperand (t, me, 1)) {
	ERROR2("Conflict merging %s as a register offset in %s\n", t, is_read ? "read" : "write");
      } else {
	hreg = 1;
      }
    }
    t = tok_or_null (NULL, me);
    if (t && *t == '+')
      t = tok_or_null (NULL, me);
  }
  if (hreg) {
    if (! merge_string_to_field ("00", me, fnum.rm, NOPRINTERR)) 
      ERROR0 ("Conflict between result mux and offset addressing\n");
    if (! himm && ! merge_int_to_field (0, me, fnum.imm, NOPRINTERR))
      ERROR0 ("Conflict setting immediate 0 for index addressing\n");
  }
  if (himm) {
    if (! merge_string_to_field ("zw", me, fnum.rm, NOPRINTERR))
      ERROR0 ("Conflict between result mux and absolute addressing\n");
  }
  if (! himm && ! hreg) {
    ERROR0("No address found in memory access\n");
  }
  return 1;
}
int
parsefield (char *tok, mcode_entry *me)
{
  char *s;
  int f;
  int n;
  
  if (! (s=strchr (tok, '.')))
      return 0;
  *s = '\0';
  s++;
  if ( (f = lookup_field (tok)) < 0) {
    ERROR1("Unknown field (%s) in dot-expression\n", tok);
    return 1;
  }
  if ( (n = lookup_opcode (s)) < 0) {
    ERROR1("Unknown opcode specifier (%s) in dot-expression\n", s);
    return 1;
  }
  if (! opc[n].fpt) {
    ERROR1("Opcode (%s) in dot-expression not specified as a field\n", s);
    return 1;
  }
  if (* (opc[n].fpt) != f) {
    ERROR3("Opcode (%s) in dot-expression affects field %s rather than %s\n",
	   s, field[*(opc[n].fpt)].name, tok);
    return 1;
  }
  merge_string_to_field (opc[n].fdef, me, f, PRINTERR);
  return 1;
}
void
do_fbinv (mcode_entry *me, int perr)
{
  char c;
  switch (c=me->bits[field[fnum.fbinv].start]) {
  case '0':
  case '1':
    if (perr) 
      ERROR1("Invalid FBINV:  field already set to %c\n", c);
    else
      c = (c == '0' ?  '1' : '0');
    break;
  case 'w':
    c = '0';
    break;
  case 'z':
  case 'x':
  case 'r':
    c = '1';
    break;
  case 'u':
  default:
    ERROR1("Invalid FBINV:  field set to %c cannot be inverted.\n", c);
    break;
  }
  me->bits[field[fnum.fbinv].start] = c;
}
int
Kfbinv (mcode_entry *me)
{
  do_fbinv (me, PRINTERR);
  return 1;
}
int
Kcmpswap (mcode_entry *me)
{
  char *fbf = &me->bits[field[fnum.fb].start];
  if (fbf[3] != '0' || fbf[4] != '0' || fbf[2] != '0') {
    ERROR0 ("CMP multiprecision comparison cannot be applied.\n");
  } else if (! (fbf[0] == '1' && fbf[1] == '0')) {
    ERROR0 ("CMP should be used with this instruction (no operand swap).\n");
  }
/* else */
{
    fbf[2] = '1';
    fbf[1] = '0';
    fbf[0] = '0';
    do_fbinv (me, NOPRINTERR);
  }
  return 1;
}
int
Kcmp (mcode_entry *me)
{
  char *fbf = &me->bits[field[fnum.fb].start];
  if (fbf[3] != '0' || fbf[4] != '0' || fbf[2] != '0') {
    ERROR0 ("CMP multiprecision comparison cannot be applied.\n");
  } else if (fbf[0] == '1' && fbf[1] == '0') {
    ERROR0 ("CMPSWAP should be used with this instruction (operands are swapped).\n");
  }
/*  else */
    {
    fbf[2] = '1';
    fbf[1] = '0';
    fbf[0] = '0';
  }
  return 1;
}
int
Kdefine (mcode_entry *me)
{
  char *tok;
  tok = tok_or_null (NULL, me);
  add_define (tok, curcontext, curcontext, tok+strlen(tok)+1, me);
  return 0;
}
int
Kinclude (mcode_entry *me)
{
  char *tok;
  tok = tok_or_null (NULL, me);
  if (! tok || ! *tok)
    ERROR0 ("File name missing from include\n");
  else
    dofile (tok, me->lstring);
  return 0;
}
int
KbeginCond (mcode_entry *me)
{
  conddepth++;
  return 0;
}
int
KendCond (mcode_entry *me)
{
  if (conddepth == 0) {
    ERROR0("Unmatched endCond ignored\n");
  } else
    conddepth--;
  return 0;
}
char loopstack[20][15];
int loopstackcount = -1;

int
KbeginLoopScr (mcode_entry *me)
{
  static int lnum = -1;
  lnum ++;
  if (loopstackcount >= 14) {
    ERROR0 ("Loop overflow -- only 15 nested loops allowed.\n");
    return 0;
  }
  loopstackcount++;
  sprintf (loopstack[loopstackcount], "beginLoop_%d", lnum);
  /* Loop indices are always global so that one can use macros
     for start of loop and end of loop code. */
  add_label (loopstack[loopstackcount], 0, me, me->index+1);
  return 1;
}
int
KbeginLoop (mcode_entry *me)
{
  merge_opcode (me, onum.KbeginLoop);
  return KbeginLoopScr (me);
}
int
KendLoop (mcode_entry *me)
{
  if (loopstackcount < 0) {
    ERROR0 ("Loop underflow -- no matching BeginLoop.\n");
    return 0;
  }
  merge_cimmediate (loopstack[loopstackcount], me, PRINTERR);
  loopstackcount--;
  merge_opcode (me, onum.KendLoop);
  return 1;
}
int
Kread (mcode_entry *me)
{
  parsereadwrite (1, me);
  merge_opcode (me, onum.Kread);
  return 1;
}
int
Kwrite (mcode_entry *me)
{
  parsereadwrite (0, me);
  merge_opcode (me, onum.Kwrite);
  return 1;
} 
int
parsemacro (mcode_entry *me, char *t)
{
  char newlstring[MAXLINE];
  char oldlstring[MAXLINE-10];  // -10 to avoid potential overflow in sprintf ADB
  char tline[MAXLINE];
  int i, ns;
  char **macdef;
  int ocontext = curcontext;
  int mycontext = numcontexts;
  int old_conddepth = conddepth;
  int err = 0;
        
  /* a macro is whitespace, symbol, [whitespace],  rparen, args, lparent, where
     symbol is *not* read or write */

  /* now separate the tokens */
  ns = lookup_sym (t, curcontext);
  if (ns < 0 || symtab[ns].type != MACRO) {
    return 0;
  }
  macdef = symtab[ns].macro;
  if (macdef == NULL) {
    ERROR1("Skipping recursive call of macro %s\n", t);

  }
  mycontext = numcontexts++;
  context_parent[mycontext] = ocontext;
  symtab[ns].macro = NULL;
  t = tok_or_null (NULL, me);
  if (t == NULL || *t != '(') {
    ERROR1("Macro call %s must have a left parenthesis.\n", symtab[ns].name);
    err = 1;
  }
  i = 0;
  if (!err) 
    while ((t = tok_or_null (NULL, me)) && (i < symtab[ns].narg)) 
      add_define (symtab[ns].args[i++], mycontext, ocontext, t, me);
  if (! err && ! t) {
    ERROR1("Macro call %s must have a right parenthesis.\n", symtab[ns].name);
    err = 1;
  } else if (! err && *t != ')') {
    ERROR1("Macro call %s has extra parameters.\n", symtab[ns].name);
    err = 1;
  }

  if (! err && i != symtab[ns].narg) {
    ERROR3("Wrong number of arguments for macro %s (expect %d found %d)\n",
	   symtab[ns].name, symtab[ns].narg, i);
    err=1;
  }
  if (! err) {
    strcpy (oldlstring, me->lstring);
    for (i = 1 ; i < symtab[ns].macrolines ; i++) {
      mcode_entry *nme;
      sprintf (newlstring, "%s:%d:%s",
	       symtab[ns].fname, symtab[ns].lnum+i, oldlstring);
      curcontext = mycontext;	/* restore context in case of macro */
      nme = init_code_line (newlstring, curcontext, conddepth);
      expand_line (tline, macdef[i], nme);
      parseline (tline, nme);
    }
    if (conddepth != old_conddepth) {
      ERRORX2 ("Macro %s has unmatched beginCond/endCond (depth change %d)\n",
	       newlstring, symtab[ns].name, old_conddepth - conddepth);
    }
  }
  symtab[ns].macro = macdef;
  curcontext = ocontext;
  return 1;
}
void
parseline (char *line, mcode_entry *me)
{
  int symnum, n;
  int gencode = 0;
  int notdone = 1;
  char *tok;
  int tlen;
  /*int not_first = 0;*/

  tok = tok_or_null (line, me);
  while (tok && tok[tlen=strlen(tok)-1] == ':') {
    add_label (tok, curcontext, me, me->index);
    tok = tok_or_null (NULL, me);
  }
 if (parsemacro (me, tok))
    return;

  while (tok && notdone) {
    symnum = lookup_sym (tok, me->context);
    if (symnum >= 0) {
      /* Lookup_sym only returns valid labels but could be
	 in different context  */
      switch (symtab[symnum].type) {
      case MACRO:
	ERROR0 ("Macro must be first token in line\n");
	tok = NULL;
	return;
      case LABEL:
	tok = symtab[symnum].name;
	break;
      case DEFINE:
	ERROR1("Defined symbols (%s) must be preceded by $\n", tok);
	tok = tok_or_null (NULL, me);
	break;
      }
    }
    /*not_first = 1;*/
    if (tok) {
      n = lookup_opcode (tok);
      if (n < 0) {
	if (parseoperand (tok, me, 0) || parsefield (tok, me)) {
	  ;
	} else {
	  merge_cimmediate (tok, me, PRINTERR);
	}
	gencode = 1;
      } else if (opc[n].fcall) {
	gencode = (*opc[n].fcall) (me) || gencode;
	if (opc[n].ownline == 2) {
	  notdone = 0;
	} else if (opc[n].ownline == 1) {
	  tok = tok_or_null (NULL, me);
	  if (tok)
	    ERROR1("Extra fields after %s ignored\n", opc[n].name);
	  notdone = 0;
	}
      } else {
	merge_opcode (me, n);
	gencode = 1;
      }
      resolveAB (me);
      tok = tok_or_null (NULL, me);
    }
  }
  if (gencode) {
    mergebits((conddepth>0?'z':'w'), &(me->bits[field[fnum.force].start]));
    checkline (me);
    commit_code_line(me);
  }
}
FILE *open_kasm (char *f, char *f2)
{
 FILE *infile;
 if (f != f2)
   strcpy (f2, f);
 if (! (infile = fopen (f2, "r"))) {
   strcat (f2, ".kasm");
   infile = fopen (f2, "r");
 }
 return infile;
}
int
dofile (char *file, char *rlstring)
{
  char line[MAXLINE];
  char eline[MAXEXPANDLINE];
  char f2[MAXLINE-20]; // -20 to avoid potential overflow in sprintf
 int lnum=0;
  FILE *infile = NULL;
  char lstring[MAXLINE];
  int oldconddepth;
  mcode_entry *me;

  oldconddepth = conddepth;
  
  if (! file || ! *file) {
    ERRORX0 ("Missing kasm file name\n", rlstring);
    return 0;
  }
  if (! (infile = open_kasm (file, f2))) {
    char *ppath;
    ppath = getenv ("KESTRELLIB");
    ppath = ppath ? ppath : KESTRELLIB;
    if (ppath[strlen(ppath)] != '/')
      sprintf (f2, "%s/%s", ppath, file);
    else
      sprintf (f2, "%s%s", ppath, file);
    if (! (infile = open_kasm (f2, f2))) {
      ERRORX2 ("Include file %s not in . or $KESTRELLIB (%s)",
	       rlstring, file, ppath);
      return 0;
    }
  }

  while (fgets (line, MAXLINE, infile)) {
    lnum++;
    sprintf (lstring, "%s:%d", f2, lnum);
    me = init_code_line (lstring, curcontext, conddepth);
    expand_line (eline, line, me);
    if (strncasecmp (eline, "MacroDef", 8) == 0) {
      lnum= define_macro (eline, infile, f2, lnum, me);
    } else
      parseline (eline, me);
  }
  
  if (conddepth != oldconddepth) {
    ERROR1 ("Unmatched beginCond/endCond in file (depth change %d)\n",
	     oldconddepth - conddepth);
  }
  fclose (infile);
  return 1;
}
void
print_code (FILE *f, int annotate, int hex)
{
  int i,j, k, w, e;
  char c;
  mcode_entry *me;

  if (annotate && !hex) {
    putc(';', f);
    for (i = numfield-2 ; i >= 0  ; i--) {
      if (field[i].start == field[i].end)
	putc (toupper(field[i].name[0]), f);
      else {
	putc ('|', f);
	for (j = 0, k=0 ; j < (field[i].end - field[i].start) ; j++) {
	  k |= ! field[i].name[j];
	  putc (k ? '.' : field[i].name[j], f);
	}
      }
    }
    putc ('\n', f);
  }
  for (j = 0; j < numcode ; j++){
    me = &(mcode[j]);
    if (annotate) {
      fprintf (f, ";- %5x %s\n", j, me->lstring);
      if (me->oline)
	fprintf (f, ";  %s\n", me->oline);
      if (me->eline)
	fprintf (f, ";  %s\n", me->eline);
    }
    if (! hex) {
      for (i = MAXBITS-1 ; i>= 0; i--) {
	c = me->bits[i];
	if (c == 'r') {
	  putc ('?',f);
	} else 
	  putc (c, f);
      }
      putc ('\n',f);
    } else {
      for (i = MAXBITS-1 ; i> 0; i-=4) {
	for (e=0,w = 0, k = 0 ; k < 4 ; k++) {
	  c = me->bits[i-k];
	  w <<=1;
	  if (c == 'r')
	    e = 1;
	  else if (c == '1' || c == 'w')
	    w |= 0x1;
	}
	if (e)
	  putc ('?',f);
	else
	  putc (w < 10 ? ('0' + w) : ('a' + w - 10),f);
      }
      putc ('\n', f);
    }
  }
  
  if (annotate) {
    fprintf (f, ";\n; Symbol Table\n");
    fprintf (f, ";\n; Address %-20s %s\n", "Symbol", "Used at");
    for (i = 0 ; i < numsym ; i++) {
      if (symtab[i].type == LABEL) {
	fprintf (f,";  %5x  %-20s", symtab[i].mline, symtab[i].name);
	for (j = 0 ; j < numcode ; j++) {
	  if (mcode[j].target == symtab[i].mline &&
	      strcasecmp (mcode[j].cimm, symtab[i].name) == 0) 
	    fprintf (f, "%5x ", j);
	}
	fprintf (f, "\n");
      }
    }
  }
    
}

void
append_last_instr()
{
  mcode_entry *me;

  me = init_code_line ("KasmInternal", 0, 0);
  
  merge_string_to_field( "11", me, fnum.pc_out_sel, PRINTERR);
  merge_string_to_field( "00101", me, fnum.func, PRINTERR);
  merge_string_to_field( "01000", me, fnum.fb, PRINTERR);
  merge_string_to_field( "1", me, fnum.cbreak, PRINTERR);
  commit_code_line(me);
}
int
commandline_define (char *def, mcode_entry *me)
{
  char *s;
  
  for (s = def ; *s && *s != '=' ; s++);
  if (*s != '=') {
    ERROR1 ("Invalid define (%s) on command line\n", def);
    return 0;
  }
  *s = '\0';
  s++;
  if (add_define (def, 0, 0, s, me) == -1)
    return 0;
  return 1;
}
  
void
usage_exit (char *p)
{
  fprintf (stderr, "Usage:  %s [-g]  [-Dsymname=value] [-o outputfile] file[.kasm]\n \tCreate Kestrel object file file.ko\n\t\t -g annotate the output file\n\t\t -D define a symbol.\n",
	   p);
  fprintf (stderr, "\t\t-v verbose annotation of output file\n");
  fprintf (stderr, "\t\t-b binary output\n");
  fprintf (stderr, "\t\t-o output file name\n");
  exit(1);
}
int
main (int argc, char **argv)
{
  mcode_entry *me; /*machine code entry; me is the current line of mach.code*/
  int farg;
  int ann = 0;
  int hex = 1;
  char *s;
  char oname[1000];
  FILE *ofile;

  fprintf (stderr, "%s:  UCSC Kestrel 1 Assembler (compilied %s)\n",
	   argv[0],DATE);
  fprintf (stderr,
	   " (c) 1998 The Regents of the University of California, Santa Cruz\n");
  initialize ();
  init_mcode ();
  context_parent[curcontext] = curcontext;
  me = init_code_line ("KasmInternal", 0, 0);
  me->cimm = strdup("start");
  merge_opcode (me, onum.Kjump);
  commit_code_line(me);
  oname[0] = '\0';

  for (farg = 1 ; farg < argc && argv[farg][0] == '-'  ; farg++) {
    switch (argv[farg][1]) {
    case 'g': ann = 1;
      break;
    case 'v': verbose = 1; ann=1;
      break;
    case 'b': hex = 0;
      break;
    case 'o': farg++;
      if (farg < argc)
	strcpy (oname, argv[farg]);
      else
	fprintf (stderr, "-o must be followed by an output file name");
      break;
    case 'D':
      if (! commandline_define (argv[farg]+2, me))
	usage_exit (argv[0]);
      break;
    default:
      fprintf (stderr, "Unknown assembler option\n");
      usage_exit (argv[0]);
    }
  }

  if (farg != argc-1)
    usage_exit(argv[0]);

  if ((s = strrchr (argv[farg], '/')))
    s++;
  else
    s = argv[farg];
  if (oname[0] == '\0') {
    strcpy (oname, s);
    if ((s = strrchr (oname, '.')) && (strcmp (s, ".kasm") == 0)) 
      *s = '\0';
    strcat (oname, ".ko");
  }
  if (! (ofile = fopen (oname, "w"))) {
    fprintf (stderr, "Could not open output file %s\n", oname);
    usage_exit (argv[0]);
  }
  
  if (dofile (argv[farg], argv[0])) {
    append_last_instr();
    resolve_labels ();
    resolve_bits();
    check_code();
    correct_board_errors();
    print_code (ofile, ann, hex);
  }  else
    usage_exit (argv[0]);
  fclose (ofile);
  return 0;
  
}
