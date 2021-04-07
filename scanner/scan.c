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
	START, INNUM, IDNUMERROR, INID, INLT, INGT, INASSIGN, INNE, INDIV, INCOMMENT, INCOMMENT2, DONE
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

/* lineBuf�� ���� �ϳ��� �а�,
   linepos�� ���� �ϳ��� char�� ��ȯ�� �Ѵ�. */
int getNextChar() {
	if (linepos >= bufsize) { // ���� �ϳ��� ��� �м� ���� ��� ���ο� ���� �б�
		lineno++;
		if (fgets(lineBuf, BUFLEN - 1, fpIn)) {
			fprintf(fpOut, "%4d: %s", lineno, lineBuf);
			bufsize = (int)strlen(lineBuf);
			linepos = 0;
			return lineBuf[linepos++];
		}
		else {
			EOF_flag = TRUE;
			return EOF;
		}
	}
	return lineBuf[linepos++]; // ���� char ��ȯ
}

/* Lookahead function.
   delimiter�� ������ �� ������ �ʰ� backing up */
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
	int tokenStringIndex = 0;			// ��ū ���ڿ�(tokenString)�� index
	TokenType currentToken = STARTFILE;	// ���� ��ū
	StateType state = START;			// ���� state�� �׻� START
	int save;							// tokenString�� ��ū�� �������� Ȯ���ϴ� flag
	while (state != DONE)	// ��ū�� DONE�� �ƴ� �� ���� �ݺ�
	{
		int c = getNextChar();	// ���� character �о����
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
				default: // ������ū
					currentToken = ERROR;
					break;
				}
			}
			break;
		case INNUM:
			// digit�� ������ ���� X
			// digit�� letter�� �ٴ� ��� ������ū���� �ν� (e.g., 111aaa)
			if (isalpha(c))
				state = IDNUMERROR;
			else if (!isdigit(c))
			{
				state = DONE;
				ungetNextChar();	// Lookahead. ���ڸ� �Ҹ����� �ʰ� �ǵ����� �Լ�
				save = FALSE;
				currentToken = NUM;
			}
			break;
		case IDNUMERROR:
			// digit �Ǵ� letter�� ������ ���� X
			if (!isalpha(c) && !isdigit(c)) {
				state = DONE;
				ungetNextChar();	// Lookahead
				save = FALSE;
				currentToken = ERROR;
			}
			break;
		case INID:
			// letter�� ������ ���� X
			// letter�� digit�� �ٴ� ��� ������ū���� �ν� (e.g., aaa111)
			if (isdigit(c))
				state = IDNUMERROR;
			else if (!isalpha(c))
			{
				ungetNextChar();	// Lookahead
				save = FALSE;
				state = DONE;
				currentToken = ID;
			}
			break;
		case INLT:
			state = DONE;
			if (c == '=')	// ��ū '<='�� �ν�
				currentToken = LE;
			else
			{
				ungetNextChar();	// Lookahead
				save = FALSE;
				currentToken = LT;
			}
			break;
		case INGT:
			state = DONE;
			if (c == '=')	// ��ū '>='�� �ν�
				currentToken = GE;
			else
			{
				ungetNextChar();	// Lookahead
				save = FALSE;
				currentToken = GT;
			}
			break;
		case INASSIGN:
			state = DONE;
			if (c == '=')	// ��ū '=='�� �ν�
				currentToken = EQ;
			else
			{
				ungetNextChar();	// Lookahead
				save = FALSE;
				currentToken = ASSIGN;
			}
			break;
		case INNE:
			state = DONE;
			if (c == '=')	// ��ū '!='�� �ν�
				currentToken = NE;
			else
			{
				ungetNextChar();	// Lookahead
				save = FALSE;
				currentToken = ERROR;
			}
			break;
		case INDIV:
			if (c == '*') {		// '/*' ���� comment�� ������ �ν�
				save = FALSE;	// comment�� save ���� ����
				state = INCOMMENT;
			}
			else {
				ungetNextChar();	// Lookahead
				state = DONE;
				currentToken = DIV;
			}
			break;
		case INCOMMENT:
			// *�� �ƴ� character�� ������ ���� X
			tokenStringIndex = 0;	// tokenString�� �̹� ����� character(/)�� �����ϱ� ���� ��ġ
			save = FALSE;
			if (c == EOF) {			// non-final state���� ���α׷��� ����Ǹ� ���� �޼��� ���
				fprintf(fpOut, "ERROR: %s\n", "\"stop before ending\"");
				exit(EXIT_FAILURE);
			}
			else if (c == '*')
				state = INCOMMENT2;
			break;
		case INCOMMENT2:
			// *�� ������ ���� X
			save = FALSE;
			if (c == '/')
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
			tokenString[tokenStringIndex++] = (char)c;		// ��ū ���ڿ��� character �߰�
		if (state == DONE)
		{
			tokenString[tokenStringIndex] = '\0';
			if (currentToken == ID)
				// ID(�ĺ���)�� ����� ���̺� �ִ��� Ȯ��. ������ �ش� ����� ��ū�� ��ȯ��
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
	// fpOut = stdout; // for test
	if (fpIn == NULL) {
		fprintf(stderr, "File %s not found\n", inputFile);
		exit(1);
	}

	fprintf(fpOut, "C- COMPILATION: %s\n", inputFile);

	while (getToken() != ENDFILE);

	fclose(fpIn);
	fclose(fpOut);
}