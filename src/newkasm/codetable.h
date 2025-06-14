extern int bits_to_field[MAXBITS];
extern int numfield;
extern int numopc;
extern opcfield field[];
extern opcodes opc[];
extern char *type_name[4];

void initialize ();
#define FLD(_n,_g,_start,_end,_deflt,_doc) int _n;
/* fnum provides field numbering, such as 'fnum.opA' */
extern struct fnum {
#include "fields.def"
} fnum;
#undef FLD
#define OPC(_n,_kvalue,_cvalue,_lname,_op,_doc,_comm) int K ## _n;
#define FOPC(_n,_filed,_fvalue,_lname,_op,_doc,_comm) int K ## _n;
#define FCALL(_n,_ownline,_kvalue,_cvalue,_lname,_op,_doc,_comm) int K ## _n;
/* onum provides opcode numbering such as 'onum.inva' */
extern struct onum {
#include "opc.def"
} onum;
#undef OPC
#undef FOPC
#undef FCALL

/*
#define BOPC  0
#define BSOPC 1
#define BIMM  7
#define BSTART 2
#define BEND   6


struct _opBd {char *name; char *val;}  opBdef[8];
*/
