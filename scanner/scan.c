#include <stdio.h>
#include <string.h>

#define MAXRESERVED 6

/* token type */
typedef enum {
	/* book-keeping tokens */
	ERROR,
	/* reserved words */
	ELSE, IF, INT, RETURN, VOID, WHILE,
	/* special symbols */
	PLUS, MINUS, MUL, DIV, LT, LE, GT, GE, EQ, NE, ASSIGN, SEMI, COMMA,
	LPAREN, RPAREN, LSQUARE, RSQUARE, LCURLY, RCURLY, LCOMMENT, RCOMMENT,
	/* multicharacter tokens */
	ID, NUM
} TokenType;

/* DFA state */
typedef enum {
	START,INASSIGN,INCOMMENT,INNUM,INID,DONE
} StateType;

/* reserved words talbe */
struct {
	char* str;
	TokenType tok;
} reservedWords[MAXRESERVED]
= { {"else",ELSE},{"if",IF},{"int",INT},{"return",RETURN},{"void",VOID},{"while",WHILE} };

/* lookup if identifier is reserved word */
TokenType reservedLookup(char* s) {
	int i;
	for (i = 0; i < MAXRESERVED; i++)
		if (!strcmp(s, reservedWords[i].str))
			return reservedWords[i].tok;
	return ID;
}

void main() {
}