#ifndef RESPONSES_H
#define RESPONSES_H

#include "methods.h"

int handle_404(response* out);
int handle_500(response* out);
int handle_403(response* out);
int handle_200(response* out);

#endif  // !RESPONSES_H
