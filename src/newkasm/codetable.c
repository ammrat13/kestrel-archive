#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
//#include <malloc.h>
#include "newkasm.h"

struct _opBd {char *name; char *val;}  opBdef[8] =
{{"opC", "000"}, {"sopC", "001"}, {"mdr", "010"}, {"smdr", "011"},
 {"mhi", "100"}, {"smhi", "101"}, {"bs",  "110"}, {"imm", "111"}};

char *type_name[4] = {"unknown", "macro", "define", "label"};
#define FLD(_n,_g,_start,_end,_deflt,_doc) int _n;
/* fnum provides field numbering, such as 'fnum.opA' */
struct fnum {
  #include "fields.def"
} fnum;
#undef FLD
#define FLD(_n,_g,_start,_end,_deflt,_doc) \
{0, #_n, _g, _start, _end, _deflt, _doc, &(fnum._n)},
/* field provides the field specifications */
opcfield field[] =
{
  #include "fields.def"
  {-1, NULL, -1, -1, -1, "", "End of fields", NULL}
};
#undef FLD
#define OPC(_n,_kvalue,_cvalue,_lname,_op,_doc,_comm) int K ## _n;
#define FOPC(_n,_filed,_fvalue,_lname,_op,_doc,_comm) int K ## _n;
#define FCALL(_n,_ownline,_kvalue,_cvalue,_lname,_op,_doc,_comm) int K ## _n;
/* onum provides opcode numbering such as 'onum.inva' */
struct onum {
  #include "opc.def"
} onum;
#undef OPC
#undef FOPC
#undef FCALL
#define OPC(_n,_kvalue,_cvalue,_lname,_op,_doc,_comm) 
#define FOPC(_n,_filed,_fvalue,_lname,_op,_doc,_comm) 
#define FCALL(_n,_ownline,_kvalue,_cvalue,_lname,_op,_doc,_comm) int K ## _n (mcode_entry *me);
#include "opc.def" 
#undef OPC
#undef FOPC
#undef FCALL
#define OPC(_n,_kvalue,_cvalue,_lname,_op,_doc,_comm) \
{0, #_n, _cvalue, _kvalue, NULL, NULL, _lname,_op,_doc,_comm, &(onum.K ## _n), NULL, 0},
#define FOPC(_n,_field,_fvalue,_lname,_op,_doc,_comm) \
{0, #_n, NULL, NULL, _fvalue, &(fnum._field), _lname,_op,_doc,_comm, &(onum.K ## _n), NULL, 0},
#define FCALL(_n,_ownline,_kvalue,_cvalue,_lname,_op,_doc,_comm) \
{0, #_n, _cvalue, _kvalue, NULL, NULL, _lname,_op,_doc,_comm, &(onum.K ## _n), &K ## _n,_ownline},
opcodes opc[] =
{
  #include "opc.def"
  {-1, NULL, NULL, NULL, NULL, NULL, "End of opcodes", NULL, NULL, NULL, NULL, NULL, 0}
};

int bits_to_field[MAXBITS];
int numfield = 0;
int numopc = 0 ;


void
initialize ()
{
  int cnt, i, j, f;
  int stop = 0;
  char *c;
  for (cnt=0; field[cnt].id != -1; *field[cnt].fnpt = field[cnt].id = cnt, cnt++) {
    for (j = field[cnt].start ; j <= field[cnt].end ; j++)
      bits_to_field[j] = cnt;
  };
  numfield = cnt;
  for (cnt=0; opc[cnt].id != -1; *opc[cnt].onpt = opc[cnt].id = cnt, cnt++) {
    if (opc[cnt].kdef || opc[cnt].cdef || opc[cnt].fpt) {
      f = numfield-1;
      if (opc[cnt].fpt)
	c = KESTRELTRUEX;
      else
	c = opc[cnt].kdef ?  opc[cnt].kdef : KESTRELX;
      for (i = MAXBITS-1 ; *c ; c++)  {
	if (*c != ' ')
	  opc[cnt].bits[i--] = *c;
	else {
	  f--;
	  if (i != field[f].end) {
	    fprintf (stderr, "Opcode %s: bad length field %s at bit %d\n",
		     opc[cnt].name, field[f].name, i);
	    stop = 1;
	  }
	}
      }
      f--;
      if (opc[cnt].fpt)
	c = CONTROLTRUEX;
      else
	c = opc[cnt].cdef ? opc[cnt].cdef : CONTROLX;
      for ( ; *c ; c++)  {
	if (*c != ' ')
	  opc[cnt].bits[i--] = *c;
	else {
	  f--;
	  if (i != field[f].end) {
	    fprintf (stderr, "Opcode %s: bad length field %s at bit %d\n",
		     opc[cnt].name, field[f].name, i);
	    stop = 1;
	  }
	}
      }
      if (i != -1) {
	fprintf (stderr,
		 "Incorrect number of bits (%d rather than %d) in %s\n",
		 MAXBITS-i, MAXBITS, opc[cnt].name);
	stop = 1;
      }
      if (opc[cnt].fpt != NULL) {
	f = *opc[cnt].fpt;
	for (i = field[f].end, j = 0 ; i >= field[f].start ; j++, i--)
	  opc[cnt].bits[i] = opc[cnt].fdef[j];
	if (j != strlen (opc[cnt].fdef)) {
	  fprintf (stderr,
		   "Incorrect number of bits for field (%lu rather than %d) in %s\n",
		   strlen (opc[cnt].fdef), j, opc[cnt].name);
	  stop = 1;
	}
      }
    } else if (opc[cnt].fcall != NULL) {
      ;
    } else {
      fprintf (stderr, "Unknown typeof opcode\n");
      stop = 1;
    }
  }
  
    
  numopc = cnt;

  if (stop)
    exit (1);
}
