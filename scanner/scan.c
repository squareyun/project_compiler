#include <stdio.h>
#include <string.h>

#define MAXRESERVED 6
#define MAXTOKENLEN 40
#define BUFLEN 256
#define TRUE 1
#define FALSE 0

/* token type */
typedef enum {
    /* book-keeping tokens */
    ENDFILE, ERROR,
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
    START, INASSIGN, INCOMMENT, INNUM, INID, DONE
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
char tokenString[MAXTOKENLEN + 1]

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

TokenType getToken(void) {
    int tokenStringIndex = 0;
    TokenType currentToken;
    StateType state = START;    // 시작 state는 항상 START
    int save;                   // tokenString에 토큰을 저장할지 확인하는 flag
    while (state != DONE)
    {
        int c = getNextChar();
        save = TRUE;
        switch (state)
        {
        case START:
            if (isdigit(c))
                state = INNUM;
            else if (isalpha(c))
                state = INID;
            else if (c == ':')
                state = INASSIGN;
            else if ((c == ' ') || (c == '\t') || (c == '\n'))
                save = FALSE;
            else if (c == '{')
            {
                save = FALSE;
                state = INCOMMENT;
            }
            else
            {
                state = DONE;
                switch (c)
                {
                case EOF:
                    save = FALSE;
                    currentToken = ENDFILE;
                    break;
                case '=':
                    currentToken = EQ;
                    break;
                case '<':
                    currentToken = LT;
                    break;
                case '+':
                    currentToken = PLUS;
                    break;
                case '-':
                    currentToken = MINUS;
                    break;
                case '*':
                    currentToken = TIMES;
                    break;
                case '/':
                    currentToken = OVER;
                    break;
                case '(':
                    currentToken = LPAREN;
                    break;
                case ')':
                    currentToken = RPAREN;
                    break;
                case ';':
                    currentToken = SEMI;
                    break;
                default:
                    currentToken = ERROR;
                    break;
                }
            }
            break;
        case INCOMMENT:
            save = FALSE;
            if (c == EOF)
            {
                state = DONE;
                currentToken = ENDFILE;
            }
            else if (c == '}') state = START;
            break;
        case INASSIGN:
            state = DONE;
            if (c == '=')
                currentToken = ASSIGN;
            else
            { /* backup in the input */
                ungetNextChar();
                save = FALSE;
                currentToken = ERROR;
            }
            break;
        case INNUM:
            if (!isdigit(c))
            { /* backup in the input */
                ungetNextChar();
                save = FALSE;
                state = DONE;
                currentToken = NUM;
            }
            break;
        case INID:
            if (!isalpha(c))
            { /* backup in the input */
                ungetNextChar();
                save = FALSE;
                state = DONE;
                currentToken = ID;
            }
            break;
        case DONE:
        default: /* should never happen */
            fprintf(listing, "Scanner Bug: state= %d\n", state);
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
    if (TraceScan) {
        fprintf(listing, "\t%d: ", lineno);
        printToken(currentToken, tokenString);
    }
    return currentToken;
} /* end getToken */


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
	if (fpIn == NULL) {
		fprintf(stderr, "File %s not found\n", inputFile);
		exit(1);
	}

	fprintf(fpOut, "\nC- COMPILATION: %s\n", inputFile);

	while (getToken() != ENDFILE);

	fclose(fpIn);
	fclose(fpOut);
}