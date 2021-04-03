#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXRESERVED 6
#define MAXTOKENLEN 40
#define BUFLEN 256
#define TRUE 1
#define FALSE 0

/* token type */
typedef enum {
	/* book-keeping tokens */
	STARTFILE, ENDFILE, ERROR,
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
	START, INNUM, INID, INLT, INGT, INASSIGN, INNE, INDIV, INCOMMENT, INCOMMENT2, DONE
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

/* global variables */
FILE* fpIn;
FILE* fpOut;
FILE* code;
int lineno = 0;         // source line number for listing

/* lexeme of ID or reserved word */
char tokenString[MAXTOKENLEN + 1];

/* variables related to current line */
char lineBuf[BUFLEN];
int linepos = 0;        // current position in linebuf
int bufsize = 0;        // current size of buffer string
int EOF_flag = FALSE;

/* lineBuf에 문장 하나를 읽고,
   linepos를 통해 하나의 char씩 반환을 한다. */
int getNextChar() {
	if (linepos >= bufsize) { // 라인 하나를 모두 분석 했을 경우 새로운 문장 읽기
		lineno++;
		if (fgets(lineBuf, BUFLEN - 1, fpIn)) {
			fprintf(fpOut, "%4d: %s", lineno, lineBuf);
			bufsize = strlen(lineBuf);
			linepos = 0;
			return lineBuf[linepos++];
		}
		else {
			EOF_flag = TRUE;
			return EOF;
		}
	}
	return lineBuf[linepos++]; // 다음 char 반환
}

/* Lookahead function
   delimiter을 만났을 때 버리지 않고 backing up */
void ungetNextChar() {
	if (!EOF_flag)
		linepos--;
}

/* print function */
void printToken(TokenType token, const char* tokenString) {
	switch (token) {
	case ELSE:
	case IF:
	case INT:
	case RETURN:
	case VOID:
	case WHILE:
		fprintf(fpOut, "reserved word: %s\n", tokenString);
		break;
	case PLUS: fprintf(fpOut, "+\n"); break;
	case MINUS: fprintf(fpOut, "-\n"); break;
	case MUL: fprintf(fpOut, "*\n"); break;
	case DIV: fprintf(fpOut, "/\n"); break;
	case LT: fprintf(fpOut, "<\n"); break;
	case LE: fprintf(fpOut, "<=\n"); break;
	case GT: fprintf(fpOut, ">\n"); break;
	case GE: fprintf(fpOut, ">=\n"); break;
	case EQ: fprintf(fpOut, "==\n"); break;
	case NE: fprintf(fpOut, "!=\n"); break;
	case ASSIGN: fprintf(fpOut, "=\n"); break;
	case SEMI: fprintf(fpOut, ";\n"); break;
	case COMMA: fprintf(fpOut, ",\n"); break;
	case LPAREN: fprintf(fpOut, "(\n"); break;
	case RPAREN: fprintf(fpOut, ")\n"); break;
	case LSQUARE: fprintf(fpOut, "[\n"); break;
	case RSQUARE: fprintf(fpOut, "]\n"); break;
	case LCURLY: fprintf(fpOut, "{\n"); break;
	case RCURLY: fprintf(fpOut, "}\n"); break;
	case ENDFILE: fprintf(fpOut, "EOF\n"); break;
	case NUM:
		fprintf(fpOut, "NUM, val= %s\n", tokenString);
		break;
	case ID:
		fprintf(fpOut, "ID, name= %s\n", tokenString);
		break;
	case ERROR:
		fprintf(fpOut, "ERROR: %s\n", tokenString);
		break;
	default: /* should never happen */
		fprintf(fpOut, "Unknown token: %d\n", token);
	}
}

/* return next token in source file */
TokenType getToken(void) {
	int tokenStringIndex = 0;
	TokenType currentToken = STARTFILE;
	StateType state = START; // 시작 state는 항상 START
	int save; // tokenString에 토큰을 저장할지 확인하는 flag
	while (state != DONE)
	{
		int c = getNextChar();
		save = TRUE;
		switch (state)
		{
		case START:
			if ((c == ' ') || (c == '\t') || (c == '\n'))
				save = FALSE;
			else if (isdigit(c))
				state = INNUM;
			else if (isalpha(c))
				state = INID;
			else if (c == '<')
				state = INLT;
			else if (c == '>')
				state = INGT;
			else if (c == '=')
				state = INASSIGN;
			else if (c == '/')
				state = INDIV;
			else if (c == '!')
				state = INNE;
			else
			{
				state = DONE;
				switch (c)
				{
				case EOF:
					save = FALSE;
					currentToken = ENDFILE;
					break;
				case '+':
					currentToken = PLUS;
					break;
				case '-':
					currentToken = MINUS;
					break;
				case '*':
					currentToken = MUL;
					break;
				case ';':
					currentToken = SEMI;
					break;
				case ',':
					currentToken = COMMA;
					break;
				case '(':
					currentToken = LPAREN;
					break;
				case ')':
					currentToken = RPAREN;
					break;
				case '[':
					currentToken = LSQUARE;
					break;
				case ']':
					currentToken = RSQUARE;
					break;
				case '{':
					currentToken = LCURLY;
					break;
				case '}':
					currentToken = RCURLY;
					break;
				default:
					currentToken = ERROR;
					break;
				}
			}
			break;
		case INNUM:
			if (isalpha(c))  /* number 다음 ID가 오는 경우 에러토큰으로 인식 */
			{
				state = DONE;
				ungetNextChar();
				currentToken = ERROR;
			}
			else if (!isdigit(c))
			{
				state = DONE;
				ungetNextChar();
				save = FALSE;
				currentToken = NUM;
			}
			break;
		case INID:
			if (!isalpha(c))
			{
				ungetNextChar();
				save = FALSE;
				state = DONE;
				if (currentToken != ERROR)
					currentToken = ID;
			}
			break;
		case INLT:
			state = DONE;
			if (c == '=')
				currentToken = LE;
			else
			{
				ungetNextChar();
				save = FALSE;
				currentToken = LT;
			}
			break;
		case INGT:
			state = DONE;
			if (c == '=')
				currentToken = GE;
			else
			{
				ungetNextChar();
				save = FALSE;
				currentToken = GT;
			}
			break;
		case INASSIGN:
			state = DONE;
			if (c == '=')
				currentToken = EQ;
			else
			{
				ungetNextChar();
				save = FALSE;
				currentToken = ASSIGN;
			}
			break;
		case INNE:
			state = DONE;
			if (c == '=')
				currentToken = NE;
			else
			{
				ungetNextChar();
				save = FALSE;
				currentToken = ERROR;
			}
			break;
		case INDIV:
			if (c == '*') {
				save = FALSE;
				state = INCOMMENT;
			}
			else {
				state = DONE;
				currentToken = DIV;
			}
			break;
		case INCOMMENT:
			tokenStringIndex = 0;	// tokenString에 이미 저장된 토큰(/)를 지우기 위함
			save = FALSE;
			if (c == EOF)
			{
				state = DONE;
				currentToken = ENDFILE;
			}
			else if (c == '*')
				state = INCOMMENT2;
			break;
		case INCOMMENT2:
			save = FALSE;
			if (c == EOF)
			{
				state = DONE;
				currentToken = ENDFILE;
			}
			else if (c == '/')
				state = START;
			else if (c != '*')
				state = INCOMMENT;
			break;
		case DONE:
		default: /* should never happen */
			fprintf(fpOut, "Scanner Bug: state= %d\n", state);
			state = DONE;
			currentToken = ERROR;
			break;
		}
		if ((save) && (tokenStringIndex <= MAXTOKENLEN))
			tokenString[tokenStringIndex++] = (char)c;
		if (state == DONE)
		{
			tokenString[tokenStringIndex] = '\0';
			if (currentToken == ID)
				currentToken = reservedLookup(tokenString);
		}
	}
	fprintf(fpOut, "\t%d: ", lineno);
	printToken(currentToken, tokenString);

	return currentToken;
}

/* main */
void main(int argc, char* argv[]) {
	char inputFile[50], outputFile[50];

	if (argc != 3) {
		fprintf(stderr, "usage: %s <input_file.c> <output_file.txt>\n", argv[0]);
		exit(1);
	}

	/* check for file extension */
	strcpy(inputFile, argv[1]);
	if (strchr(inputFile, '.') == NULL)
		strcat(inputFile, ".c");
	strcpy(outputFile, argv[2]);
	if (strchr(outputFile, '.') == NULL)
		strcat(outputFile, ".txt");

	fpIn = fopen(inputFile, "r");
	fpOut = fopen(outputFile, "w");
	// fpOut = stdout; //for test
	if (fpIn == NULL) {
		fprintf(stderr, "File %s not found\n", inputFile);
		exit(1);
	}

	fprintf(fpOut, "C- COMPILATION: %s\n", inputFile);

	while (getToken() != ENDFILE);

	fclose(fpIn);
	fclose(fpOut);
}