extern GSList *mime_programs;

struct mime_program
{
  int id;
  const char *name;
  const char *mime;
  const char *command;
};

extern int sql_start (void);

extern struct mime_program *new_mime_program (const char *name, const char *mime, const char *command);

extern void del_mime_program (struct mime_program *e);

extern void del_mime_program_all (void);

extern void sql_close (void);
