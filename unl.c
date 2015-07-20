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
typedef struct cont cont;

/* A "function": i or .x or `sk or something like that. */
struct fun {
	int refcount;
	enum { KAY, KAY1, ESS, ESS1, ESS2, EYE, VEE, DEE, DEE1, DOT, SEE, CONT } type;
	union {
		fun *onefunc; /* for KAY1 and ESS1: `k(onefunc) or `s(onefunc) */
		struct { fun *f1, *f2; } twofunc; /* for ESS2: ``s(f1)(f2) */
		expr *expr; /* for DEE1: `d(expr) */
		char toprint; /* for DOT: .(toprint) */
		cont *cont;
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

/* A "continuation": what to do next. You'll toss it a fun*.
   EVAL_APPLY: get "func", eval "e", apply "func" to "e", next()
   APPLY: get "arg", apply "f" to "arg", next()
   APPLY_DEE: get "func", apply "func" to "f", next()
   TERM: terminate execution */
struct cont {
	int refcount;
	enum { EVAL_APPLY, APPLY, APPLY_DEE, TERM } type;
	union {
		expr *e;
		fun *f;
	} v;
	cont *next;
};

fun *fun_addref(fun *f);
void fun_decref(fun *f);
expr *expr_addref(expr *e);
void expr_decref(expr *e);
cont *cont_addref(cont *c);
void cont_decref(cont *c);

#define IS_STATIC_FUN_TYPE(type) ((type) == KAY || (type) == ESS || (type) == EYE || (type) == VEE || (type) == DEE || (type) == SEE)
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
		case CONT:
			cont_decref(f->v.cont);
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

#define IS_STATIC_CONT_TYPE(type) ((type) == TERM)
cont *cont_addref(cont *c) {
	if (!IS_STATIC_CONT_TYPE(c->type)) {
		c->refcount++;
	}
	return c;
}
void cont_decref(cont *c) {
	if (IS_STATIC_CONT_TYPE(c->type)) return;
	c->refcount--;
	if (c->refcount == 0) {
		switch (c->type) {
		case EVAL_APPLY:
			expr_decref(c->v.e);
			break;
		case APPLY:
		case APPLY_DEE:
			fun_decref(c->v.f);
			break;
		default:
			fprintf(stderr, "Memory corruption at %d\n", __LINE__);
			return;
		}
		cont_decref(c->next);
		free(c);
	}
}


fun s_fun = { 1, ESS };
fun k_fun = { 1, KAY };
fun i_fun = { 1, EYE };
fun v_fun = { 1, VEE };
fun d_fun = { 1, DEE };
fun c_fun = { 1, SEE };

cont term_c = { 1, TERM };

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
cont *make_cont(cont *next) {
	cont *ret = malloc(sizeof(*ret));
	ret->refcount = 1;
	ret->next = next;
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
	case 'c':
	case 'C':
		ret->type = FUNCTION;
		ret->v.func = &c_fun;
		return ret;
	default:
		fprintf(stderr, "Parse error: unexpected %c (0x%02x).\n", ch, ch);
		exit(EXIT_FAILURE);
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
	case SEE:
		putchar('c');
		break;
	case CONT:
		printf("<cont>");
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

void toss(cont *c, fun *arg);
void apply(fun *func, fun *arg, cont *c);
void eval(expr *e, cont *c);
void end_the_program(fun *result);

/* Toss val to the continuation c */
void toss(cont *c, fun *val) {
	switch (c->type) {
	case EVAL_APPLY:
		if (val->type == DEE) {
			cont *next = cont_addref(c->next);
			fun *f = make_fun();
			f->type = DEE1;
			f->v.expr = expr_addref(c->v.e);
			cont_decref(c);
			/* no fun_decref(arg) because it's builtin */
			toss(next, f);
		} else {
			cont *next = make_cont(cont_addref(c->next));
			expr *e = expr_addref(c->v.e);
			next->type = APPLY;
			next->v.f = val; /* no addref because we're about to decref it anyway */
			cont_decref(c);
			eval(e, next);
		}
		return;
	case APPLY:
	{
		cont *next = cont_addref(c->next);
		fun *func = fun_addref(c->v.f);
		cont_decref(c);
		apply(func, val, next);
		return;
	}
	case APPLY_DEE:
	{
		cont *next = cont_addref(c->next);
		fun *arg = fun_addref(c->v.f);
		cont_decref(c);
		apply(val, arg, next);
		return;
	}
	case TERM:
		end_the_program(val);
		return;
	default:
		fprintf(stderr, "Memory corruption at %d\n", __LINE__);
		return;
	}
}

/* apply "func" to "arg", tossing the result to "c" */
void apply(fun *func, fun *arg, cont *c) {
	switch (func->type) {
	case KAY:
	{
		fun *val = make_fun();
		val->type = KAY1;
		val->v.onefunc = arg;
		/* No fun_decref(func) because it's builtin, no fun_decref(arg) because we gave it to val */
		toss(c, val);
		break;
	}
	case KAY1:
	{
		fun *val = fun_addref(func->v.onefunc);
		fun_decref(func);
		fun_decref(arg);
		toss(c, val);
		break;
	}
	case ESS:
	{
		fun *val = make_fun();
		val->type = ESS1;
		val->v.onefunc = arg;
		/* No fun_decref(func) because it's builtin, no fun_decref(arg) because we gave it to val */
		toss(c, val);
		break;
	}
	case ESS1:
	{
		fun *val = make_fun();
		val->type = ESS2;
		val->v.twofunc.f1 = fun_addref(func->v.onefunc);
		val->v.twofunc.f2 = arg;
		fun_decref(func);
		/* No fun_decref(arg) because we gave it to val */
		toss(c, val);
		break;
	}
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
		ez->v.func = arg; /* No fun_addref because we're giving it away */

		es1->type = APPLICATION;
		es1->v.ap.func = ex;
		es1->v.ap.arg = expr_addref(ez); /* This one is an extra reference */
		es2->type = APPLICATION;
		es2->v.ap.func = ey;
		es2->v.ap.arg = ez; /* This one we're giving away */

		e->type = APPLICATION;
		e->v.ap.func = es1;
		e->v.ap.arg = es2;
		
		fun_decref(func);
		/* No fun_decref(arg) because we gave it away */
		eval(e, c);
		break;
	}
	case DOT:
		putchar(func->v.toprint);
		fun_decref(func);
		toss(c, arg);
		break;
	case DEE:
	{
		expr *e = make_expr();
		fun *val = make_fun();
		e->type = FUNCTION;
		e->v.func = arg;
		val->type = DEE1;
		val->v.expr = e;
		/* No fun_decref(func) because it's builtin, no fun_decref(arg) because we gave it to e */
		toss(c, val);
		break;
	}
	case EYE:
		toss(c, arg);
		break;
	case VEE:
		fun_decref(arg);
		toss(c, func); /* fun_addref unneeded because it's a builtin */
		break;
	case DEE1:
	{
		cont *next = make_cont(c);
		expr *e = expr_addref(func->v.expr);
		next->type = APPLY_DEE;
		next->v.f = arg;
		fun_decref(func);
		/* No fun_decref(arg) nor cont_decref(c) because we gave them to next */
		eval(e, next);
		break;
	}
	case SEE:
	{
		fun *f = make_fun();
		f->type = CONT;
		f->v.cont = cont_addref(c);
		/* No fun_decref(func) because it's builtin, no fun_decref(arg) because we're giving it to apply */
		apply(arg, f, c);
		break;
	}
	case CONT:
	{
		cont *next = cont_addref(func->v.cont);
		fun_decref(func);
		cont_decref(c);
		toss(next, arg);
		break;
	}
	default:
		fprintf(stderr, "Memory corruption at %d. Acting like i.\n", __LINE__);
		toss(c, arg);
		break;
	}
}

/* evaluate "e", tossing the result to "c" */
void eval(expr *e, cont *c) {
	if (e->type == FUNCTION) {
		fun *f = fun_addref(e->v.func);
		expr_decref(e);
		toss(c, f);
	} else { /* e->type == APPLICATION */
		cont *next = make_cont(c);
		next->type = EVAL_APPLY;
		next->v.e = expr_addref(e->v.ap.arg);
		expr *func = expr_addref(e->v.ap.func);
		expr_decref(e);
		eval(func, next);
	}
}

int main() {
	cont *c = &term_c; /* will call end_the_program() once execution is complete */
	expr *prog = parse();

	eval(prog, c);

	return 0;
}

void end_the_program(fun *result) {
	printf("\nResult: ");
	print_fun(result);
	putchar('\n');

#ifndef NDEBUG
	fun_decref(result);

	printf("\nSTATS:\n%lu allocations\n%lu frees\n", (unsigned long)n_mallocs, (unsigned long)n_frees);
	if (n_mallocs > n_frees) {
		printf("MEMORY LEAK!\n");
	} else if (n_mallocs < n_frees) {
		printf("DOUBLE FREE!\n");
	}
#endif
}
