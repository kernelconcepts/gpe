#ifndef _GPE_TIMESHEET_HTML_H
#define _GPE_TIMESHEET_HTML_H

#include "sql.h"

int journal_add_header(char* title);
int journal_add_line(struct task atask);
int journal_add_footer();
int journal_to_file(const char *filename);
int journal_show(const char *filename);

#endif
