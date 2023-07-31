
// poopstain
int spc2_start();
int spc2_finish(int *final_size, char *filename, u16 num_spc);
int spc2_write_header(u16 num_spc);
int spc2_write_metadata(char *filename, spc2_metadata *o, spc_struct *s, spc_idx6_table *t);
int spc2_write_spc(char *filename, spc_struct *s, spc_idx6_table *t);
