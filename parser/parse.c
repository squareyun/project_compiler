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
#define MAXCHILDREN 3

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

/* DFA state for scanner */
typedef enum {
	START, INNUM, IDNUMERROR, INID, INLT, INGT, INASSIGN, INNE, INDIV, INCOMMENT, INCOMMENT2, DONE
} StateType;

/* syntax tree for parsing */
typedef enum { StmtK, ExpK } NodeKind;
typedef enum { ExpressionK, CompoundK, SelectionK, IterationK, ReturnK, CallK } StmtKind;
typedef enum { VarDeclK, VarArrayDeclK, FuncDeclK, AssignK, OpK, ConstK, IdK } ExpKind;
typedef enum { Void, Integer } ExpType;

typedef struct treeNode {
	struct treeNode* child[MAXCHILDREN];
	struct treeNode* sibling;
	NodeKind nodekind;
	union { StmtKind stmt; ExpKind exp; } kind;
	union {
		TokenType op;
		int val;
		char* name;
	} attr;
	ExpType type; /* for type checking of exps */
	int lineno;
	int paramCheck;
	int arraysize;
} TreeNode;

/* reserved words talbe */
struct {
	char* str;
	TokenType tok;
} reservedWords[MAXRESERVED]
= { {"else",ELSE},{"if",IF},{"int",INT},{"return",RETURN},{"void",VOID},{"while",WHILE} };

/* global variables */
FILE* fpIn, * fpOut;
int lineno = 0;         // source line number for listing
TokenType token;
int indentno = 0;

/* macros to increase/decrease indentation */
#define INDENT indentno+=2
#define UNINDENT indentno-=2

/* lexeme of ID or reserved word */
char tokenString[MAXTOKENLEN + 1];

/* variables related to current line */
char lineBuf[BUFLEN];
int linepos = 0;        // current position in linebuf
int bufsize = 0;        // current size of buffer string
int EOF_flag = FALSE;

/* !for scanner! declaration of funtion */
TokenType reservedLookup(char* s);
int getNextChar();
void ungetNextChar();
void printToken(TokenType token, const char* tokenString);
TokenType getToken(void);

/* !for parser! declaration of function */
TreeNode* newStmtNode(StmtKind kind);
TreeNode* newExpNode(ExpKind kind);
void match(TokenType expected);
void syntaxError(char* message);
ExpType type_checker(void);
char* copyString(char* s);
TreeNode* parse(void);
TreeNode* declaration_list();
TreeNode* declaration();
TreeNode* var_declaration(void);
TreeNode* params(void);
TreeNode* param_list(ExpType type);
TreeNode* param(ExpType type);
TreeNode* compound_stmt(void);
TreeNode* local_decl(void);
TreeNode* stmt_list(void);
TreeNode* stmt(void);
TreeNode* expression_stmt(void);
TreeNode* selection_stmt(void);
TreeNode* iteration_stmt(void);
TreeNode* return_stmt(void);
TreeNode* expr(void);
TreeNode* simple_expr(TreeNode* f);
TreeNode* add_expr(TreeNode* f);
TreeNode* term(TreeNode* f);
TreeNode* factor(TreeNode* f);
TreeNode* call(void);
TreeNode* args(void);
TreeNode* args_list(void);
static void printSpaces(void);
char* typeName(ExpType type);
void printTree(TreeNode* tree);


/* main */
void main(int argc, char* argv[]) {
	TreeNode* syntaxTree;
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
	//fpOut = stdout; // for test
	if (fpIn == NULL) {
		fprintf(stderr, "File %s not found\n", inputFile);
		exit(1);
	}

	fprintf(fpOut, "C- COMPILATION: %s\n", inputFile);

	// while (getToken() != ENDFILE);
	syntaxTree = parse();
	fprintf(fpOut, "\nSyntax tree:\n");
	printTree(syntaxTree);

	fclose(fpIn);
	fclose(fpOut);
}


/***************scanner function***************/
/* lookup if identifier is reserved word */
TokenType reservedLookup(char* s) {
	int i;
	for (i = 0; i < MAXRESERVED; i++)
		if (!strcmp(s, reservedWords[i].str))
			return reservedWords[i].tok;
	return ID;
}

/* lineBuf에 문장 하나를 읽고,
   linepos를 통해 하나의 char씩 반환을 한다. */
int getNextChar() {
	if (linepos >= bufsize) { // 라인 하나를 모두 분석 했을 경우 새로운 문장 읽기
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
	return lineBuf[linepos++]; // 다음 char 반환
}

/* Lookahead function.
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
	int tokenStringIndex = 0;			// 토큰 문자열(tokenString)의 index
	TokenType currentToken = STARTFILE;	// 현재 토큰
	StateType state = START;			// 시작 state는 항상 START
	int save;							// tokenString에 토큰을 저장할지 확인하는 flag
	while (state != DONE)	// 토큰이 DONE이 아닐 때 까지 반복
	{
		int c = getNextChar();	// 다음 character 읽어오기
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
				default: // 에러토큰
					currentToken = ERROR;
					break;
				}
			}
			break;
		case INNUM:
			// digit을 읽으면 전이 X
			// digit과 letter가 붙는 경우 에러토큰으로 인식 (e.g., 111aaa)
			if (isalpha(c))
				state = IDNUMERROR;
			else if (!isdigit(c))
			{
				state = DONE;
				ungetNextChar();	// Lookahead. 문자를 소모하지 않고 되돌리는 함수
				save = FALSE;
				currentToken = NUM;
			}
			break;
		case IDNUMERROR:
			// digit 또는 letter을 읽으면 전이 X
			if (!isalpha(c) && !isdigit(c)) {
				state = DONE;
				ungetNextChar();	// Lookahead
				save = FALSE;
				currentToken = ERROR;
			}
			break;
		case INID:
			// letter을 읽으면 전이 X
			// letter과 digit이 붙는 경우 에러토큰으로 인식 (e.g., aaa111)
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
			if (c == '=')	// 토큰 '<='을 인식
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
			if (c == '=')	// 토큰 '>='을 인식
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
			if (c == '=')	// 토큰 '=='을 인식
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
			if (c == '=')	// 토큰 '!='을 인식
				currentToken = NE;
			else
			{
				ungetNextChar();	// Lookahead
				save = FALSE;
				currentToken = ERROR;
			}
			break;
		case INDIV:
			if (c == '*') {		// '/*' 으로 comment의 시작을 인식
				save = FALSE;	// comment는 save 하지 않음
				state = INCOMMENT;
			}
			else {
				ungetNextChar();	// Lookahead
				state = DONE;
				currentToken = DIV;
			}
			break;
		case INCOMMENT:
			// *이 아닌 character를 읽으면 전이 X
			tokenStringIndex = 0;	// tokenString에 이미 저장된 character(/)를 무시하기 위한 조치
			save = FALSE;
			if (c == EOF) {			// non-final state에서 프로그램이 종료되면 에러 메세지 출력
				fprintf(fpOut, "ERROR: %s\n", "\"stop before ending\"");
				exit(EXIT_FAILURE);
			}
			else if (c == '*')
				state = INCOMMENT2;
			break;
		case INCOMMENT2:
			// *를 읽으면 전의 X
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
			tokenString[tokenStringIndex++] = (char)c;		// 토큰 문자열에 character 추가
		if (state == DONE)
		{
			tokenString[tokenStringIndex] = '\0';
			if (currentToken == ID)
				// ID(식별자)가 예약어 테이블에 있는지 확인. 있으면 해당 예약어 토큰이 반환됨
				currentToken = reservedLookup(tokenString);
		}
	}
	fprintf(fpOut, "\t%d: ", lineno);
	printToken(currentToken, tokenString);

	return currentToken;
}



/*********************************************/
/****************parser function**************/
/*********************************************/

TreeNode* newStmtNode(StmtKind kind)
{
	TreeNode* t = (TreeNode*)malloc(sizeof(TreeNode));
	int i;
	if (t == NULL)
		fprintf(fpOut, "Out of memory error at line %d\n", lineno);
	else {
		for (i = 0; i < MAXCHILDREN; i++)
			t->child[i] = NULL;
		t->sibling = NULL;
		t->nodekind = StmtK;
		t->kind.stmt = kind;
		t->lineno = lineno;
	}
	return t;
}

TreeNode* newExpNode(ExpKind kind)
{
	TreeNode* t = (TreeNode*)malloc(sizeof(TreeNode));
	int i;
	if (t == NULL)
		fprintf(fpOut, "Out of memory error at line %d\n", lineno);
	else {
		for (i = 0; i < MAXCHILDREN; i++)
			t->child[i] = NULL;
		t->sibling = NULL;
		t->nodekind = ExpK;
		t->kind.exp = kind;
		t->lineno = lineno;
		t->type = Void;
		t->paramCheck = FALSE;
	}
	return t;
}

void match(TokenType expected)
{
	if (token == expected)
		token = getToken();
	else {
		syntaxError("unexpected token -> ");
		printToken(token, tokenString);
		fprintf(fpOut, "      ");
	}
}

void syntaxError(char* message)
{
	fprintf(fpOut, "\n>>> ");
	fprintf(fpOut, "Syntax error at line %d: %s", lineno, message);
}

ExpType type_checker(void)
{
	switch (token)
	{
	case INT:
		token = getToken();
		return Integer;
	case VOID:
		token = getToken();
		return Void;
	default: syntaxError("unexpected token(type_checker) -> ");
		printToken(token, tokenString);
		token = getToken();
		return Void;
	}
}

char* copyString(char* s)
{
	int n;
	char* t;
	if (s == NULL)
		return NULL;
	n = (int)strlen(s) + 1;
	t = malloc(n);
	if (t == NULL)
		fprintf(fpOut, "Out of memory error at line %d\n", lineno);
	else
		strcpy(t, s);
	return t;
}

TreeNode* parse(void)
{
	TreeNode* t = NULL;
	token = getToken();
	t = declaration_list();
	if (token != ENDFILE)
		syntaxError("Code ends before file\n");
	return t;
}

TreeNode* declaration_list()
{
	TreeNode* t = declaration();
	TreeNode* p = t;
	while (token != ENDFILE)
	{
		TreeNode* q;
		q = declaration();
		if (q != NULL) {
			if (t == NULL) t = p = q;
			else
			{
				p->sibling = q;
				p = q;
			}
		}
	}
	return t;
}

// 토큰 되돌리기를 구현하는 대신 케이스를 나누어 이렇게 구현하였음.
TreeNode* declaration() {
	TreeNode* t = NULL;
	ExpType type;
	char* name;

	type = type_checker();
	name = copyString(tokenString);
	match(ID);
	switch (token)
	{
	case SEMI: // var-declaration 첫번째 경우
		t = newExpNode(VarDeclK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = type;
		}
		match(SEMI);
		break;
	case LSQUARE: // var-declaration 두번째 경우
		t = newExpNode(VarArrayDeclK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = type;
		}
		match(LSQUARE);
		if (t != NULL)
			t->arraysize = atoi(tokenString);
		match(NUM);
		match(RSQUARE);
		match(SEMI);
		break;
	case LPAREN: // fun-declaration 경우
		t = newExpNode(FuncDeclK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = type;
		}
		match(LPAREN);
		if (t != NULL)
			t->child[0] = params();
		match(RPAREN);
		if (t != NULL)
			t->child[1] = compound_stmt();
		break;
	default: syntaxError("unexpected token in declaration -> ");
		printToken(token, tokenString);
		token = getToken();
		break;
	}
	return t;
}

// declaration 문법에서 사용하는 것은 아니지만 추후 사용을 위해 구현
// fun_declaration은 추후에 사용할 일이 없어 따로 구현X
TreeNode* var_declaration(void)
{
	TreeNode* t = NULL;
	ExpType type;
	char* name;

	type = type_checker();
	name = copyString(tokenString);
	match(ID);
	switch (token)
	{
	case SEMI:
		t = newExpNode(VarDeclK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = type;
		}
		match(SEMI);
		break;
	case LSQUARE:
		t = newExpNode(VarArrayDeclK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = type;
		}
		match(LSQUARE);
		if (t != NULL)
			t->arraysize = atoi(tokenString);
		match(NUM);
		match(RSQUARE);
		match(SEMI);
		break;
	default: syntaxError("unexpected token(var_decl) -> ");
		printToken(token, tokenString);
		token = getToken();
		break;
	}
	return t;
}

TreeNode* params(void)
{
	ExpType type;
	TreeNode* t = NULL;

	type = type_checker();
	// type_checker()결과 token을 하나 소비하게됨
	// 그러면 fun_declaration의 grammar에 따라 다음 토큰은 ')'이 된다.
	if (type == Void && token == RPAREN)
	{
		t = newExpNode(VarDeclK);
		t->paramCheck = TRUE;
		t->type = Void;
	}
	else
		t = param_list(type);
	return t;
}

TreeNode* param_list(ExpType type)
{
	TreeNode* t = param(type);
	TreeNode* p = t;
	TreeNode* q = NULL;
	while (token == COMMA)
	{
		match(COMMA);
		q = param(type_checker());  // type 체크 들어갔음 -> 토큰 하나 읽어옴
		if (q != NULL) {
			if (t == NULL) t = p = q;
			else /* now p cannot be NULL either */
			{
				p->sibling = q;
				p = q;
			}
		}
	}
	return t;
}

TreeNode* param(ExpType type)
{
	TreeNode* t = NULL;
	char* name;

	name = copyString(tokenString);
	match(ID);
	if (token == LSQUARE)
	{
		match(LSQUARE);
		match(RSQUARE);
		t = newExpNode(VarArrayDeclK);
	}
	else
		t = newExpNode(VarDeclK);
	if (t != NULL)
	{
		t->attr.name = name;
		t->type = type;
		t->paramCheck = TRUE;
	}
	return t;
}

TreeNode* compound_stmt(void)
{
	TreeNode* t = newStmtNode(CompoundK);
	match(LCURLY);
	t->child[0] = local_decl();
	t->child[1] = stmt_list();
	match(RCURLY);
	return t;
}

TreeNode* local_decl(void)
{
	TreeNode* t = NULL;
	TreeNode* p = NULL;

	if (token == INT || token == VOID)
		t = var_declaration();
	p = t;
	if (t != NULL)
	{
		while (token == INT || token == VOID)
		{
			TreeNode* q;
			q = var_declaration();
			if (q != NULL) {
				if (t == NULL) t = p = q;
				else
				{
					p->sibling = q;
					p = q;
				}
			}
		}
	}
	return t;
}

TreeNode* stmt_list(void)
{
	TreeNode* t = NULL;
	TreeNode* p = NULL;

	if (token == RCURLY)
		return NULL;
	t = stmt();
	p = t;
	while (token != RCURLY)
	{
		TreeNode* q;
		q = stmt();
		if (q != NULL) {
			if (t == NULL) t = p = q;
			else
			{
				p->sibling = q;
				p = q;
			}
		}
	}

	return t;
}

TreeNode* stmt(void)
{
	TreeNode* t = NULL;
	switch (token)
	{
	case LCURLY:
		t = compound_stmt();
		break;
	case IF:
		t = selection_stmt();
		break;
	case WHILE:
		t = iteration_stmt();
		break;
	case RETURN:
		t = return_stmt();
		break;
	case ID:
	case LPAREN:
	case NUM:
	case SEMI:
		t = expression_stmt();
		break;
	default: syntaxError("unexpected token(stmt) -> ");
		printToken(token, tokenString);
		token = getToken();
		return Void;
	}
	return t;
}

TreeNode* expression_stmt(void)
{
	TreeNode* t = NULL;

	if (token == SEMI)
		match(SEMI);
	else if (token != RCURLY)
	{
		t = expr();
		match(SEMI);
	}
	return t;
}

TreeNode* selection_stmt(void)
{
	TreeNode* t = newStmtNode(SelectionK);

	match(IF);
	match(LPAREN);
	if (t != NULL)
		t->child[0] = expr();
	match(RPAREN);
	if (t != NULL)
		t->child[1] = stmt();
	if (token == ELSE)
	{
		match(ELSE);
		if (t != NULL)
			t->child[2] = stmt();
	}

	return t;
}

TreeNode* iteration_stmt(void)
{
	TreeNode* t = newStmtNode(IterationK);

	match(WHILE);
	match(LPAREN);
	if (t != NULL)
		t->child[0] = expr();
	match(RPAREN);
	if (t != NULL)
		t->child[1] = stmt();
	return t;
}

TreeNode* return_stmt(void)
{
	TreeNode* t = newStmtNode(ReturnK);

	match(RETURN);
	if (token != SEMI && t != NULL)
		t->child[0] = expr();
	match(SEMI);
	return t;
}

TreeNode* expr(void)
{
	TreeNode* t = NULL;
	TreeNode* q = NULL;
	int flag = FALSE;

	if (token == ID)
	{
		q = call();
		flag = TRUE;
	}

	if (flag == TRUE && token == ASSIGN)
	{
		if (q != NULL && q->nodekind == ExpK && q->kind.exp == IdK)
		{
			match(ASSIGN);
			t = newExpNode(AssignK);
			if (t != NULL)
			{
				t->child[0] = q;
				t->child[1] = expr();
			}
		}
		else
		{
			syntaxError("attempt to assign to something not an lvalue\n");
			token = getToken();
		}
	}
	else
		t = simple_expr(q);
	return t;
}

TreeNode* simple_expr(TreeNode* f)
{
	TreeNode* t = NULL, * q = NULL;
	TokenType oper;
	q = add_expr(f);
	if (token == LT || token == LE || token == GT || token == GE || token == EQ || token == NE)
	{
		oper = token;
		match(token);
		t = newExpNode(OpK);
		if (t != NULL)
		{
			t->child[0] = q;
			t->child[1] = add_expr(NULL);
			t->attr.op = oper;
		}
	}
	else
		t = q;
	return t;
}

TreeNode* add_expr(TreeNode* f)
{
	TreeNode* t = NULL;
	TreeNode* q = NULL;

	t = term(f);
	if (t != NULL)
	{
		while (token == PLUS || token == MINUS)
		{
			q = newExpNode(OpK);
			if (q != NULL) {
				q->child[0] = t;
				q->attr.op = token;
				t = q;
				match(token);
				t->child[1] = term(NULL);

			}
		}
	}
	return t;
}

TreeNode* term(TreeNode* f)
{
	TreeNode* t = NULL;
	TreeNode* q = NULL;

	t = factor(f);
	if (t != NULL)
	{
		while (token == MUL || token == DIV)
		{
			q = newExpNode(OpK);
			if (q != NULL) {
				q->child[0] = t;
				q->attr.op = token;
				t = q;
				match(token);
				t->child[1] = factor(NULL);

			}
		}
	}
	return t;
}

TreeNode* factor(TreeNode* f)
{
	TreeNode* t = NULL;

	if (f != NULL)
		return f;

	switch (token)
	{
	case LPAREN:
		match(LPAREN);
		t = expr();
		match(RPAREN);
		break;
	case ID:
		t = call();
		break;
	case NUM:
		t = newExpNode(ConstK);
		if (t != NULL)
		{
			t->attr.val = atoi(tokenString);
			t->type = Integer;
		}
		match(NUM);
		break;
	default: syntaxError("unexpected token(factor) -> ");
		printToken(token, tokenString);
		token = getToken();
		return Void;
	}
	return t;
}

TreeNode* call(void)
{
	TreeNode* t = NULL;
	char* name = "";

	if (token == ID)
		name = copyString(tokenString);
	match(ID);

	if (token == LPAREN)
	{
		match(LPAREN);
		t = newStmtNode(CallK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->child[0] = args();
		}
		match(RPAREN);
	}
	else if (token == LSQUARE)
	{
		t = newExpNode(IdK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = Integer;
			match(LSQUARE);
			t->child[0] = expr();
			match(RSQUARE);
		}
	}
	else
	{
		t = newExpNode(IdK);
		if (t != NULL)
		{
			t->attr.name = name;
			t->type = Integer;
		}
	}
	return t;
}

TreeNode* args(void)
{
	if (token == RPAREN)
		return NULL;
	else
		return args_list();
}

TreeNode* args_list(void)
{
	TreeNode* t = NULL;
	TreeNode* p = NULL;

	t = expr();
	p = t;
	if (t != NULL)
	{
		while (token == COMMA)
		{
			match(COMMA);
			TreeNode* q = expr();
			if (q != NULL) {
				if (t == NULL) t = p = q;
				else
				{
					p->sibling = q;
					p = q;
				}
			}
		}
	}
	return t;
}


static void printSpaces(void)
{
	int i;
	for (i = 0; i < indentno; i++)
		fprintf(fpOut, " ");
}

char* typeName(ExpType type)
{
	static char i[] = "int";
	static char v[] = "void";
	static char invalid[] = "<<invalid type>>";

	switch (type)
	{
	case Integer: return i; break;
	case Void:    return v; break;
	default:      return invalid;
	}
}

void printTree(TreeNode* tree)
{
	INDENT;
	while (tree != NULL) {
		printSpaces();
		if (tree->nodekind == StmtK)
		{
			switch (tree->kind.stmt) {
			case SelectionK:
				fprintf(fpOut, "if:\n");
				INDENT;
				printSpaces();
				fprintf(fpOut, "Condition:\n");
				printTree(tree->child[0]);
				printSpaces();
				fprintf(fpOut, "Body:\n");
				if (tree->child[1]->kind.stmt == CompoundK) {
					printTree(tree->child[1]->child[0]);
					printTree(tree->child[1]->child[1]);
				}
				else {
					printTree(tree->child[1]);
					printSpaces();
					fprintf(fpOut, "Else body:\n");
					printTree(tree->child[2]);
				}
				UNINDENT;
				break;
			case IterationK:
				fprintf(fpOut, "while:\n");
				INDENT;
				printSpaces();
				fprintf(fpOut, "Condition:\n");
				printTree(tree->child[0]);
				printSpaces();
				fprintf(fpOut, "Body:\n");
				printTree(tree->child[1]->child[0]);
				printTree(tree->child[1]->child[1]);
				UNINDENT;
				break;
			case ReturnK:
				fprintf(fpOut, "return:\n");
				printTree(tree->child[0]);
				break;
			case CallK:
				fprintf(fpOut, "Call: %s\n", tree->attr.name);
				INDENT;
				printSpaces();
				if (tree->child[0] == NULL)
					fprintf(fpOut, "Args: nothing\n");
				else {
					fprintf(fpOut, "Args:\n");
					printTree(tree->child[0]);
				}
				UNINDENT;
				break;
			default:
				fprintf(fpOut, "Unknown ExpNode kind\n");
				break;
			}
		}
		else if (tree->nodekind == ExpK)
		{
			switch (tree->kind.exp) {
			case VarDeclK:
				if (tree->type == Void)
					fprintf(fpOut, "Declare variable: (null), type: void\n");
				else
					fprintf(fpOut, "Declare variable: %s, type: %s\n", tree->attr.name, typeName(tree->type));
				break;
			case VarArrayDeclK:
				if (tree->paramCheck == TRUE)
					fprintf(fpOut, "Declare array: %s[], type: %s\n", tree->attr.name, typeName(tree->type));
				else
					fprintf(fpOut, "Declare array: %s[%d], type: %s\n", tree->attr.name, tree->arraysize, typeName(tree->type));
				break;
			case FuncDeclK:
				fprintf(fpOut, "Declare function: %s, type: %s\n", tree->attr.name, typeName(tree->type));
				INDENT;
				printSpaces();
				fprintf(fpOut, "params:\n");
				printTree(tree->child[0]);
				printSpaces();
				fprintf(fpOut, "Function Body:\n");
				printTree(tree->child[1]->child[0]);
				printTree(tree->child[1]->child[1]);
				UNINDENT;
				break;
			case AssignK:
				fprintf(fpOut, "assign:\n");
				INDENT;
				printTree(tree->child[0]);
				printTree(tree->child[1]);
				UNINDENT;
				break;
			case OpK:
				fprintf(fpOut, "Op: ");
				printToken(tree->attr.op, "\0");
				INDENT;
				printTree(tree->child[0]);
				printTree(tree->child[1]);
				UNINDENT;
				break;
			case IdK:
				fprintf(fpOut, "Id: %s\n", tree->attr.name);
				if (tree->child[0] != NULL) {
					INDENT;
					printTree(tree->child[0]);
					UNINDENT;
				}
				break;
			case ConstK:
				fprintf(fpOut, "Const: %d\n", tree->attr.val);
				break;
			default:
				fprintf(fpOut, "Unknown ExpNode kind\n");
				break;
			}
		}
		else
			fprintf(fpOut, "Unknown node kind\n");
		tree = tree->sibling;
	}
	UNINDENT;
}