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
#include <strings.h>
//#include <malloc.h>
#include "newkasm.h"
#include "codetable.h"

/* Stubs to prevent unresolved functions */
#define OPC(_n,_kvalue,_cvalue,_lname,_op,_doc,_comm) 
#define FOPC(_n,_filed,_fvalue,_lname,_op,_doc,_comm) 
#define FCALL(_n,_ownline,_kvalue,_cvalue,_lname,_op,_doc,_comm) int K ## _n (mcode_entry *me) {return 0;}
#include "opc.def" 
#undef OPC
#undef FOPC
#undef FCALL

#define UNKNOWN  0
#define OPERAND  1
#define LOGICAL  2
#define KESTREL  3
#define MULTIPLY 4 
#define SELECT   5
#define COMPARE  6
#define BITSH    7
#define FLAG     8
#define KFIELD   9
#define CONTROL  10
#define CFIELD   11
#define ASM      12


static char *opc_type[] =
{"Unknown", "Instruction operands", "Logical  Instructions",
 "Arithmetic Instructions", "Multiplier Instructions",
 "Selection Instructions",
 "Comparison Instructions",
 "Bitshifter Modifiers", "Flag Modifiers",
 "Other Array Modifiers", "Controller Instructions",
 "Controller Modifiers", "Assembler Directives"
};

static char *index_reorder[] =
{
  "Multiprecision", "Signed", NULL
};

static char *index_subhead[] ={
  "A,", "B,", "C,", "Zero,", NULL
};


#define B(_a) (c->bits[field[fnum._a].end])
#define BN0(_a) (c->bits[field[fnum._a].end] != '0')
#define BN1(_a) (c->bits[field[fnum._a].end] != '1')
#define B0(_a) (c->bits[field[fnum._a].end]  == '0')
#define B1(_a) (c->bits[field[fnum._a].end]  == '1')
#define Bx(_a) (c->bits[field[fnum._a].end] == 'x')
#define BNx(_a) (c->bits[field[fnum._a].end] != 'x')
#define Bf(_a,_n) (c->bits[field[fnum._a].start + _n])
#define BfN0(_a,_n) (c->bits[field[fnum._a].start + _n] != '0')
#define BfN1(_a,_n) (c->bits[field[fnum._a].start + _n] != '1')
#define Bf0(_a,_n) (c->bits[field[fnum._a].start + _n] == '0')
#define Bf1(_a,_n) (c->bits[field[fnum._a].start + _n] == '1')
#define Bf01(_a,_n) (Bf0(_a,_n) | Bf1(_a,_n))

/* Make the operation string from character shorthand */
char *
texify (char *op)
{
  static char buf[400];
  char buf2[100];
  char *t;
  buf[0] = '\0';

  strcpy (buf2, op);
  t = strtok (buf2, " \t\n");
  while (t && *t) {
    switch (*t) {
    case '<':
      if (t[1] == '-')
	strcat (buf, " $\\leftarrow$ ");
      else if (t[1] == '=')
	strcat (buf, " $\\leq$ ");
      else if (! t[1])
	strcat (buf, " $<$ ");
      else {
	strcat (buf, " $"); strcat (buf, t); strcat (buf, "$ ");
      }
      break;
    case '+':
      if (t[1] == '-')
	strcat (buf, " $\\pm$ ");
      else {
	strcat (buf, " $+$ ");
	strcat (buf, t+1); 
      }
      break;
    case '-':
      strcat (buf, " $-$ ");
      strcat (buf, t+1); 
      break;
    case '&' :
      strcat (buf, " $\\wedge$ ");
      break;
    case '|' :
      strcat (buf, " $\\vee$ ");
      break;
    case '^' :
      strcat (buf, " $\\oplus$ ");
      break;
    case '!':
      strcat (buf, " $\\neg$");
      break;
    case '*':
      if (! t[1])
	strcat (buf, " $\\times$ ");
      else {
	strcat (buf, " $"); strcat (buf, t); strcat (buf, "$ ");
      }
      break;
    default:
      if (t[1] == '\0') {
	strcat (buf, " $"); strcat (buf, t); strcat (buf, "$ ");
      } else
	strcat (buf, t);
	strcat (buf, " ");
    }
    t = strtok (NULL, " ");
  }
  return buf;
}

void
doopcode (FILE *ofile, opcodes *c)
{
  char *s, *t;
  int x = 0, f, i;
  char ibuf[100];
  int is_opcode;

   is_opcode =  (Bf(dest,0) == 'r' || B(opB) == 'r' ||
		B(opA) == 'r' || B(opC) == 'r');
  

  /* Opcodes get caps, others get lowercase */
  if (c->type != OPERAND && (Bf(dest,0) == 'r' || B(opA) != 'x'))
    for (s = c->name, t = ibuf ; *s ; *t++ = toupper (*s++));
  else
    for (s = c->name, t = ibuf ; *s ; *t++ = tolower (*s++));
  *t = '\0';
    
  fprintf (ofile, "\\kasm{%s}{%s}{%s}{%s}\n",
	   ibuf, c->longname, "Unused", texify(c->operation));

  if (1) {
    fprintf (ofile, "{\\tt %s ", ibuf);
    if (B(dest) == 'r')
      fprintf (ofile, "%s{\\sc dest}", x++ ? ", " : "");
    if (B(opB) == 'e' || B(opB) == 'E')
      fprintf (ofile, "%s[{\\sc opA} {\\em or} {\\sc opB}]", x++ ? ", " : "");
    if (B(opB) == 'r') 
      fprintf (ofile, "%s{\\sc opB}", x++ ? ", " : "");
    if (B(opA) == 'r')
      fprintf (ofile, "%s{\\sc opA}", x++ ? ", " : "");
    if (B(opC) == 'r')
      fprintf (ofile, "%s{\\sc opC}", x++ ? ", " : "");
    if (B(imm) == 'r')
      fprintf (ofile, "%s \\#{\\sc Imm}", x++ ? ", " : "");
    if (B(cont_imm) == 'r')
      fprintf (ofile, "%s {\\sc CImm}", x++ ? ", " : "");
    if (B(cont_imm) == 'l')
      fprintf (ofile, "%s {\\sc Label}", x++ ? ", " : "");
    fprintf (ofile, "}{");
  } else {
    fprintf (ofile, "{\\{\\tt %s}}{", ibuf);
  }
  
  /* Latches and things used */
  x = 0;
  if ((B1(mp) && BN1(ci)) ||
      (Bf0(fb,4) && Bf1(fb,3) && Bf1(fb,2)&&Bf1(fb,1)&&Bf0(fb,0)))
    fprintf (ofile, "%sCarry", x++ ? ",": "");
  if (B1(mp) &&  B1(ci) && Bf1(func,0)) 
    fprintf (ofile, "%sMHi", x++? "," : "");
  if (Bf0(bit,3) && Bf0(bit,2) && Bf0(bit,1) && Bf1(bit,0))
    fprintf (ofile, "%sMsk", x++? "," : "");
  /* BS used if used in BS or mesh or fb flag bus */
  if ( ((Bf1(bit,0) || Bf1(bit,1)  || Bf1(bit,2)  || Bf1(bit,3))  &&
	(! ((Bf0(bit,3) && Bf0(bit,2) && ! (Bf1(bit,1) && Bf1(bit,0)))
	    || (Bf0(bit,2) && Bf0(bit,1) && Bf0(bit,0))))) ||
       (Bf1(fb,4)&& ! (Bf0(fb,3) && Bf0(fb,2))) ||
       (Bf0(fb,4) && Bf1 (fb,3) && Bf0 (fb,2))  ||
       (Bf1(opB,2) && Bf1(opB,1) && Bf0 (opB,0)))
    fprintf (ofile, "%sBS", x++? "," : "");
  /* cmp used if eqlatch used or comparator w/o reset */
  if ((Bf0(fb,3) && Bf1(fb,4) && Bf0(fb,2)) ||
      (Bf0(fb,4) && Bf0(fb,3) && (Bf1(fb,2) || (Bf1(fb,1) && Bf1(fb,0)))))
      fprintf (ofile, "%sCmp", x++? "," : "");
  if (Bf0(opB,2) && Bf1(opB,1))
      fprintf (ofile, "%sMdr", x++? "," : "");
  if (Bf1(opB,2) && Bf0(opB,1))
      fprintf (ofile, "%sMHi", x++? "," : "");
      
  if ( ! x)
    fprintf (ofile, "--");
  fprintf (ofile, "}{");
  /* Things set */
  x = 0;
  if (Bf01(opB,0) && Bf01(opB,1) && Bf01(opB,2))
      fprintf (ofile, "%sOpB", x++? "," : "");    
  if (Bf1(bit,0) || Bf1(bit,1)  || Bf1(bit,2)  || Bf1(bit,3)) {
    if (! (Bf0(bit,3) && Bf0(bit,2) && (Bf0(bit,1) || Bf0(bit,0))))
      fprintf (ofile, "%sBS", x++? "," : "");
    /* Unch 0011 1000 1101 1111 */
    if (! (  (Bf1(bit,3) &&
	      ((Bf1(bit,2) && Bf1(bit,0)) ||
	       (Bf0(bit,2) && Bf0(bit,1) && Bf0 (bit,0))))
	   ||( Bf0(bit,3) && Bf0(bit,2) && Bf1(bit,1) && Bf1(bit,0))))
      fprintf (ofile, "%sMsk", x++? "," : "");
  }
  if (Bf0(fb,4) && Bf0(fb,3) && ! (Bf0(fb,2) && Bf1(fb,1) && Bf1(fb,0)))
      fprintf (ofile, "%sCmp", x++? "," : "");
  if (B1(mp) &&  B1(ci)) 
    fprintf (ofile, "%sMHi", x++? "," : "");
  if (B1(lc))
    fprintf (ofile, "%sCarry", x++ ? ",": "");
  if (B1(wr))
    fprintf (ofile, "%sSRAM", x++ ? ",": "");
  if (B1(rd))
    fprintf (ofile, "%sMDR", x++ ? ",": "");

  if ( ! x)
    fprintf (ofile, "--");
  fprintf (ofile, "}\n{{%s%s}{%s}\n{",
	   c->fcall ? "{(Calls internal kasm function.)} ":"",
	   c->doc, c->comments);
  if (c->type <= KFIELD || c->kdef) {
    for (f = fnum.force ; f >= fnum.imm ; f--) {
      for (i = field[f].end ; i >= field[f].start ; i--) 
	fprintf (ofile, "%c", c->bits[i]);
      fprintf (ofile, "~");
    }
  }
  
  fprintf (ofile, "}\n{");
  if (c->type == CONTROL || c->type == CFIELD || c->cdef) {
    for (f = fnum.imm-1 ; f >= 0 ; f--) {
      for (i = field[f].end ; i >= field[f].start ; i--) 
	fprintf (ofile, "%c", c->bits[i]);
      fprintf (ofile, "~");
    }
  }
  
  fprintf (ofile, "}}\n{");
  /* Index terms */
  if (c->type == BITSH || strstr(c->longname, "itshifter")) 
    fprintf (ofile, "\\index{Bitshifter!%s}", c->longname);
  if (c->type == FLAG)
    fprintf (ofile, "\\index{Flag bus!%s}", c->longname);
  strcpy (ibuf, c->longname);
  t = strchr (ibuf, ',');
  if (t != NULL)
    *t = '!';
  fprintf (ofile, "\\index{%s}}\n\n", ibuf);
}
void
set_types ()
{
  int i;
  opcodes *c;
  for (i = 0, c=opc ; i < numopc ; i++, c++) {
    if  (! c->kdef && ! c->cdef && ! c->fpt) 
      c->type = ASM;
    else if (c->cdef && ! c->kdef)
      c->type = CONTROL;
    else if (c->kdef && ! c->cdef) {
      if ((strncmp (c->name, "op",2) == 0) || (strcmp (c->name, "destreg") == 0))
	c->type = OPERAND;
      else if (B(mp) == '1' && B(ci) == '1')
	c->type = MULTIPLY;
      else if  	((Bf1(rm,0)&& Bf1(rm,1)))
	c->type = SELECT;
      else if ((Bf0(fb,4) && Bf0(fb,3)) ||
	       (Bf1(fb,4) && Bf0(fb,3) && Bf0(fb,2)
		&& Bf0(fb,1)))
	c->type = COMPARE;
      else if (Bf01(bit,0))
	c->type = BITSH;
      else if (! c->fcall) {
	if (Bf1(lc,0))
	  c->type = KESTREL;
	else
	  c->type = LOGICAL;
      }
      else
	c->type = KFIELD;
    } else if (c->fpt) {
      if (*c->fpt == fnum.bit)
	c->type = BITSH;
      else if (*c->fpt == fnum.opB)
	c->type = OPERAND;
      else if (*c->fpt == fnum.fb || strncasecmp ("fb", c->name,2) == 0)
	c->type = FLAG;
      else if (*c->fpt  >= fnum.imm)
	c->type = KFIELD;
      else
	c->type = CFIELD;
    } else
      c->type = CONTROL;
  }
}
int
compare_opcodes (const void * oo1, const void * oo2)
{
  opcodes *o1 = * (opcodes **) oo1;
  opcodes *o2 = * (opcodes **) oo2;

   if (o1->type < o2->type)
    return -1;
  else if (o1->type > o2->type)
    return 1;
  else
    return strcasecmp (o1->name, o2->name);
}

void
sort_and_output (FILE *f)
{
  int i, ltype;
  opcodes **opcpt;

  opcpt = malloc (numopc * sizeof (opcodes *));
  for (i = 0 ; i < numopc ; i++)
    opcpt[i] = opc+i;

  qsort (opcpt, numopc, sizeof (opcodes *), compare_opcodes);

  ltype = -1;
  for (i = 0 ; i < numopc ; i++) {
    if (ltype != opcpt[i]->type) {
      ltype = opcpt[i]->type;
      fprintf (f, "\\clearpage\n\\section{%s}\n", opc_type[ltype]);
    }
    doopcode (f, opcpt[i]);
  }
}

int
main ()
{
  FILE *f;

  f = fopen ("kasmopc.tex", "w");
  fprintf (f,
	   "%%  THIS FILE IS AUTOMATICALLY GENERATED BY genmanual\n");
  initialize();

  set_types();
  
  sort_and_output (f);

  fclose (f);
  exit(0);
  
}
