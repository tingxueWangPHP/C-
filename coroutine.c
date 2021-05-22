#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#define STACK_SIZE (1024*1024) // 默认栈大小
#define DEFAULT_COROUTINE 16 // 默认初始化创建协程数量

#define COROUTINE_DEAD 0
#define COROUTINE_READY 1
#define COROUTINE_RUNNING 2
#define COROUTINE_SUSPEND 3

struct schedule;

typedef void (*coroutine_func)(struct schedule *, void *ud);

struct schedule * coroutine_open(void);
void coroutine_close(struct schedule *);

int coroutine_new(struct schedule *, coroutine_func, void *ud);
void coroutine_resume(struct schedule *, int id);
int coroutine_status(struct schedule *, int id);
int coroutine_running(struct schedule *);
void coroutine_yield(struct schedule *);

struct schedule {
	char stack[STACK_SIZE];
	ucontext_t main;
	int nco;
	int cap;
	int running;
	struct coroutine **co;
};

struct coroutine {
	coroutine_func func;
	void *ud;
	ucontext_t ctx;
	struct schedule *sch;
	ptrdiff_t cap;
	ptrdiff_t size;
	int status;
	char *stack;
};


static void _save_stack(struct coroutine *, char *);

struct schedule * coroutine_open(void) {
	struct schedule *p = (struct schedule *)malloc(sizeof(struct schedule));
	p->nco = 0;
	p->cap = DEFAULT_COROUTINE;
	p->running = 0;
	p->co = (struct coroutine **)malloc(sizeof(struct coroutine *)*DEFAULT_COROUTINE);
	memset(p->co, 0, sizeof(struct coroutine *)*DEFAULT_COROUTINE);
	return p;
}

struct coroutine * _co_new(struct schedule *S , coroutine_func func, void *ud) {
	struct coroutine * p = (struct coroutine *)malloc(sizeof(struct coroutine));
	p->func = func;
	p->ud = ud;
	p->sch = S;
	p->status = COROUTINE_READY;
	p->cap = 0;
	p->size = 0;
	p->stack = NULL;
	//memset(p->stack, 0, STACK_SIZE);


	return p;
}

int coroutine_new(struct schedule *S, coroutine_func func, void *ud) {
	
	struct coroutine *p = _co_new(S, func, ud);

	if (S->nco >= S->cap) {
		int id = S->nco;
		S->co = realloc(S->co, sizeof(struct coroutine *)*S->cap*2);
		memset(S->co+S->cap, 0, sizeof(struct coroutine *)*S->cap);
		S->nco++;
		S->cap*=2;
		S->co[id] = p;
		return id;
		
	} else {
		int i;
        	for (i=0; i<S->cap; i++) {
                	if (S->co[i] == NULL) {
                        	S->co[i] = p;
                        	S->nco++;


                        	return i;
                	}
        	}
	
	}

}

void _co_delete(struct coroutine *co) {
	free(co->stack);
	free(co);
}







void coroutine_close(struct schedule *S) {
	int i;
	for (i = 0; i< S->nco; i++) {
		_co_delete(S->co[i]);
	}

	free(S->co);
	S->co = NULL;
	free(S);
}

int coroutine_status(struct schedule * S, int id) {
	return S->co[id] == NULL ? COROUTINE_DEAD : S->co[id]->status;
}


int coroutine_running(struct schedule * S) {
	return S->running;

}


static void mainfunc(uint32_t low32, uint32_t hi32) {
	uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
	struct schedule *S = (struct schedule *)ptr;
	int id = coroutine_running(S);
	struct coroutine *co = S->co[id];
	co->func(S, co->ud);
	_co_delete(co);
	S->nco--;
	S->running = -1;
	S->co[id] = NULL;
}

void coroutine_resume(struct schedule * S, int id) {
	struct coroutine *co = S->co[id];
	if (id >= S->cap || co == NULL) {
		return ;
	}
		
	switch(co->status) {
		case COROUTINE_RUNNING:
			return;
		case COROUTINE_READY:
			getcontext(&co->ctx);
			co->ctx.uc_stack.ss_sp = S->stack;
			co->ctx.uc_stack.ss_size = sizeof(S->stack);

			co->ctx.uc_link = &S->main;
			co->status = COROUTINE_RUNNING;
			S->running = id;
			uintptr_t ptr = (uintptr_t)S;
			makecontext(&co->ctx, mainfunc, 2, (uint32_t)ptr, (uint32_t)(ptr>>32));
			swapcontext(&S->main, &co->ctx);
			break;
		case COROUTINE_SUSPEND:{
			char *yy = S->stack + STACK_SIZE - co->size;
			memcpy(S->stack + STACK_SIZE - co->size, co->stack, co->size);
			printf("%d\n", *((int *)yy+1));
				
			co->status = COROUTINE_RUNNING;
			S->running = id;
			swapcontext(&S->main, &co->ctx);
			break;
		}
		default:
			break;
	}
	
	
}


void coroutine_yield(struct schedule * S) {
	int id = coroutine_running(S);
	struct coroutine *co = S->co[id];
	co->status = COROUTINE_SUSPEND;
	S->running = -1;
	_save_stack(co, S->stack + STACK_SIZE);
	swapcontext(&co->ctx, &S->main);
}

static void _save_stack(struct coroutine *C, char *top) {
	//char tt = 0;
	int tt[4] = {1,2,3,5};
	int length = top-(char *)tt;
	if (C->size < length) {
		C->stack = (char *)realloc(C->stack, length);
	}
	C->size = length;
        memcpy(C->stack, tt, length);
}




static void foo(struct schedule * S, void *ud) {
	//puts((char *)ud);
	//coroutine_yield(S);
	int i;
	for (i = 0; i < 5; i++) {
		puts((char *)ud);
		coroutine_yield(S);
	}
}


int main() {
	struct schedule *p = coroutine_open();
	int index1 = coroutine_new(p, foo, (void *)"88");
	int index2 = coroutine_new(p, foo, (void *)"99");
	//coroutine_resume(p, index1);
	//coroutine_resume(p, index1);
	while (coroutine_status(p, index1) && coroutine_status(p, index2)) {
		coroutine_resume(p, index1);
		coroutine_resume(p, index2);
	}

	puts("end");
	coroutine_close(p);
	return 0;
}

