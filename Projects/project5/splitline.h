/*
 * splitline.h
 */

char * next_cmd(char *prompt, FILE *fp);
char ** splitline(char *line);
char *newstr(char *s, int l);
void freelist(char **list);
void * emalloc(size_t n);
void * erealloc(void *p, size_t n);

