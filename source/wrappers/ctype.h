#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int isspace(char c);

int isdigit(int c);
int isalpha(int c);
int islower(int c);

char tolower(char c);
char toupper(char c);

#ifdef __cplusplus
}
#endif