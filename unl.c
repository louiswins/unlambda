#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef NDEBUG
size_t n_mallocs = 0;
size_t n_frees = 0;
void *my_malloc(size_t size) {
	void *ret = malloc(size);
	++n_mallocs;
	if (ret == NULL) {
		fprintf(stderr, "OUT OF MEMORY: %lu mallocs, %lu frees\n", (unsigned long)n_mallocs, (unsigned long)n_frees);
		exit(EXIT_FAILURE);
	}
	return ret;
}
void my_free(void *ptr) {
	++n_frees;
	free(ptr);
}

#define malloc my_malloc
#define free my_free
#endif

typedef struct expr expr;
typedef struct fun fun;

/* A "function": i or .x or `sk or something like that. */
struct fun {
	int refcount;
	enum { KAY, KAY1, ESS, ESS1, ESS2, EYE, VEE, DEE, DEE1, DOT } type;
	union {
		fun *onefunc;
		struct { fun *f1, *f2; } twofunc;
		expr *expr;
		char toprint;
	} v;
};

/* An "expression": either a function or an application of one function to another. */
struct expr {
	int refcount;
	enum { FUNCTION, APPLICATION } type;
	union {
		fun *func;
		struct { expr *func, *arg; } ap;
	} v;
};

fun *fun_addref(fun *f);
void fun_decref(fun *f);
expr *expr_addref(expr *e);
void expr_decref(expr *e);

#define IS_STATIC_FUN_TYPE(type) ((type) == KAY || (type) == ESS || (type) == EYE || (type) == VEE || (type) == DEE)

fun *fun_addref(fun *f) {
	if (!IS_STATIC_FUN_TYPE(f->type))
		f->refcount++;
	return f;
}
void fun_decref(fun *f) {
	/* These are static and shared. Ignore them. */
	if (IS_STATIC_FUN_TYPE(f->type)) return;
	f->refcount--;
	if (f->refcount == 0) {
		switch (f->type) {
		case KAY1:
		case ESS1:
			fun_decref(f->v.onefunc);
			break;
		case ESS2:
			fun_decref(f->v.twofunc.f1);
			fun_decref(f->v.twofunc.f2);
			break;
		case DEE1:
			expr_decref(f->v.expr);
			break;
		case DOT:
			break;
		default:
			fprintf(stderr, "Memory corruption at %d\n", __LINE__);
			return;
		}
		free(f);
	}
}

expr *expr_addref(expr *e) {
	e->refcount++;
	return e;
}
void expr_decref(expr *e) {
	e->refcount--;
	if (e->refcount == 0) {
		switch (e->type) {
		case FUNCTION:
			fun_decref(e->v.func);
			break;
		case APPLICATION:
			expr_decref(e->v.ap.func);
			expr_decref(e->v.ap.arg);
			break;
		default:
			fprintf(stderr, "Memory corruption at %d\n", __LINE__);
			return;
		}
		free(e);
	}
}


fun s_fun = { 1, ESS };
fun k_fun = { 1, KAY };
fun i_fun = { 1, EYE };
fun v_fun = { 1, VEE };
fun d_fun = { 1, DEE };

void unexpected_eof() {
	fprintf(stderr, "Parsing error: unexpected EOF.\n");
	exit(1);
}

expr *make_expr() {
	expr *ret = malloc(sizeof(*ret));
	ret->refcount = 1;
	return ret;
}
fun *make_fun() {
	fun *ret = malloc(sizeof(*ret));
	ret->refcount = 1;
	return ret;
}

expr* parse() {
	int ch;
	expr *ret = make_expr();
	for (;;) {
		ch = getchar();
		if (ch == EOF) unexpected_eof();
		if (isspace(ch)) continue;
		if (ch == '#') {
			while ((ch = getchar()) != EOF && ch != '\n');
			continue;
		}
		break;
	}
	switch (ch) {
	case '`':
		ret->type = APPLICATION;
		ret->v.ap.func = parse();
		ret->v.ap.arg = parse();
		return ret;
	case 's':
	case 'S':
		ret->type = FUNCTION;
		ret->v.func = &s_fun;
		return ret;
	case 'k':
	case 'K':
		ret->type = FUNCTION;
		ret->v.func = &k_fun;
		return ret;
	case 'i':
	case 'I':
		ret->type = FUNCTION;
		ret->v.func = &i_fun;
		return ret;
	case 'v':
	case 'V':
		ret->type = FUNCTION;
		ret->v.func = &v_fun;
		return ret;
	case '.':
		ch = getchar();
		if (ch == EOF) unexpected_eof();
		ret->type = FUNCTION;
		ret->v.func = make_fun();
		ret->v.func->type = DOT;
		ret->v.func->v.toprint = ch;
		return ret;
	case 'r':
	case 'R':
		ret->type = FUNCTION;
		ret->v.func = make_fun();
		ret->v.func->type = DOT;
		ret->v.func->v.toprint = '\n';
		return ret;
	case 'd':
	case 'D':
		ret->type = FUNCTION;
		ret->v.func = &d_fun;
		return ret;
	default:
		fprintf(stderr, "Parse error: unexpected %c (0x%02x).\n", ch, ch);
		exit(1);
	}
}

void print_fun(fun *func);
void print_expr(expr *prog);

void print_fun(fun *func) {
	switch (func->type) {
	case KAY:
		putchar('k');
		break;
	case KAY1:
		printf("`k");
		print_fun(func->v.onefunc);
		break;
	case ESS:
		putchar('s');
		break;
	case ESS1:
		printf("`s");
		print_fun(func->v.onefunc);
		break;
	case ESS2:
		printf("``s");
		print_fun(func->v.twofunc.f1);
		print_fun(func->v.twofunc.f2);
		break;
	case EYE:
		putchar('i');
		break;
	case VEE:
		putchar('v');
		break;
	case DEE:
		putchar('d');
		break;
	case DEE1:
		printf("`d");
		print_expr(func->v.expr);
		break;
	case DOT:
		if (func->v.toprint == '\n') {
			putchar('r');
		} else {
			printf(".%c", func->v.toprint);
		}
		break;
	default:
		printf("(corrupted memory)");
		break;
	}
}

void print_expr(expr *prog) {
	switch (prog->type) {
	case FUNCTION:
		print_fun(prog->v.func);
		break;
	case APPLICATION:
		putchar('`');
		print_expr(prog->v.ap.func);
		print_expr(prog->v.ap.arg);
		break;
	default:
		printf("(corrupted memory)");
		break;
	}
}



fun *apply(fun *func, fun *arg);
fun *eval(expr *e);

fun *apply(fun *func, fun *arg) {
	fun *ret;
	switch (func->type) {
	case KAY:
		ret = make_fun();
		ret->type = KAY1;
		ret->v.onefunc = fun_addref(arg);
		break;
	case KAY1:
		ret = fun_addref(func->v.onefunc);
		break;
	case ESS:
		ret = make_fun();
		ret->type = ESS1;
		ret->v.onefunc = fun_addref(arg);
		break;
	case ESS1:
		ret = make_fun();
		ret->type = ESS2;
		ret->v.twofunc.f1 = fun_addref(func->v.onefunc);
		ret->v.twofunc.f2 = fun_addref(arg);
		break;
	case ESS2:
	{
		/* Because of the possibility of seeing d somewhere, we'll carefully
		   construct expressions to evaluate. */
		expr *ex = make_expr();
		expr *ey = make_expr();
		expr *ez = make_expr();
		expr *es1 = make_expr();
		expr *es2 = make_expr();
		expr *e = make_expr();

		ex->type = FUNCTION;
		ex->v.func = fun_addref(func->v.twofunc.f1);
		ey->type = FUNCTION;
		ey->v.func = fun_addref(func->v.twofunc.f2);
		ez->type = FUNCTION;
		ez->v.func = fun_addref(arg);

		es1->type = APPLICATION;
		es1->v.ap.func = expr_addref(ex);
		es1->v.ap.arg = expr_addref(ez);
		es2->type = APPLICATION;
		es2->v.ap.func = expr_addref(ey);
		es2->v.ap.arg = expr_addref(ez);

		expr_decref(ex);
		expr_decref(ey);
		expr_decref(ez);

		e->type = APPLICATION;
		e->v.ap.func = expr_addref(es1);
		e->v.ap.arg = expr_addref(es2);

		expr_decref(es1);
		expr_decref(es2);

		ret = eval(expr_addref(e));

		expr_decref(e);
		break;
	}
	case DOT:
		putchar(func->v.toprint);
		ret = fun_addref(arg);
		break;
	case DEE:
	{
		expr *e = make_expr();
		e->type = FUNCTION;
		e->v.func = fun_addref(arg);
		ret = make_fun();
		ret->type = DEE1;
		ret->v.expr = e;
		break;
	}
	case EYE:
		ret = fun_addref(arg);
		break;
	case VEE:
		ret = func; /* fun_addref unneeded because it's a builtin */
		break;
	case DEE1:
	{
		fun *action = eval(expr_addref(func->v.expr));
		if (action->type == DEE) {
			expr *e = make_expr();
			e->type = FUNCTION;
			e->v.func = fun_addref(arg);
			ret = make_fun();
			ret->type = DEE1;
			ret->v.expr = e;
		} else {
			ret = apply(fun_addref(action), fun_addref(arg));
		}
		fun_decref(action);
		break;
	}
	default:
		fprintf(stderr, "Memory corruption at %d. Acting like i.\n", __LINE__);
		ret = fun_addref(arg);
		break;
	}
	fun_decref(func);
	fun_decref(arg);
	return ret;
}

fun *eval(expr *e) {
	fun *ret;
	if (e->type == FUNCTION) {
		ret = fun_addref(e->v.func);
	} else { /* e->type == APPLICATION */
		fun *func = eval(expr_addref(e->v.ap.func));
		if (func->type == DEE) {
			ret = make_fun();
			ret->type = DEE1;
			ret->v.expr = expr_addref(e->v.ap.arg);
		} else {
			fun *arg = eval(expr_addref(e->v.ap.arg));
			ret = apply(fun_addref(func), fun_addref(arg));
			fun_decref(arg);
		}
		fun_decref(func);
	}
	expr_decref(e);
	return ret;
}

int main() {
	fun *ret;

	expr *prog = parse();
	ret = eval(prog); /* no expr_addref() because we don't use prog afterwards. */

	printf("\nResult: ");
	print_fun(ret);
	putchar('\n');

#ifndef NDEBUG
	fun_decref(ret);

	printf("\nSTATS:\n%lu allocations\n%lu frees\n", (unsigned long)n_mallocs, (unsigned long)n_frees);
	if (n_mallocs > n_frees) {
		printf("MEMORY LEAK!\n");
	} else if (n_mallocs < n_frees) {
		printf("DOUBLE FREE!\n");
	}
#endif
	return 0;
}
