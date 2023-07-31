

#define SPC_LOAD_SUCCESS	0;
#define SPC_LOAD_FILEERR	1;
#define SPC_LOAD_INVALID	2;
#define SPC_LOAD_UNDEFINED	10;

int spc_load(char *filename, spc_struct *s, spc_idx6_table *t);