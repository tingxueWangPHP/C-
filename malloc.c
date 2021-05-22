#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>
#include <time.h>

void test();

struct s_block {
	size_t size;
	struct s_block *next;
	struct s_block *prev;
	void *addr;
	char isfree;
};

struct s_block *head;

struct s_block * find_block(struct s_block *head, size_t size) {
	struct s_block * temp = head;
	while (temp && !(temp->isfree && temp->size >= size)) {
		temp = temp->next;
	}

	return temp;
}

struct s_block * extend_heap(struct s_block *last, size_t size) {
	void *p;
	if ((p = sbrk(sizeof(struct s_block) + size)) == (void *)-1) {
		return NULL;
	}

	void *addr_data = p + sizeof(struct s_block);
	
	struct s_block *b = p;

	b->size = size;
	b->next = NULL;
	b->isfree = 0;
	b->addr = addr_data;
	b->prev = last;
	
	if (last != NULL) {
		last->next = p;
	}
	return addr_data;  	
}

void split_block(struct s_block *b, size_t size) {
	struct s_block *new_block = (void *)b + sizeof(struct s_block) + size;
	bzero(new_block, b->size - size);
	new_block->size = b->size - size - sizeof(struct s_block);
	new_block->next = b->next;
	new_block->isfree = 1;
	new_block->addr = (void *)new_block + sizeof(struct s_block);
	new_block->prev = b;

	b->size = size;
	b->next = new_block;

	//test();
}

struct s_block * getlast(struct s_block *head) {
	while (head->next) {
		head = head->next;
	}
	
	return head;
}

void *malloc_test(size_t size) {
	void *res;
	struct s_block * temp;
	if (!head) {
		res = extend_heap(NULL, size);
		if (res) {
			head = res - sizeof(struct s_block);
		}
	} else {
		temp = find_block(head, size);
		if (temp == NULL) {
			res = extend_heap(getlast(head), size);
		} else {
			if (temp->size - size >= sizeof(struct s_block) + 8) {
				split_block(temp, size);
			}
			temp->isfree = 0;
			res = temp->addr;
		}
	}

	return res;

}

void *calloc_test(size_t nitems, size_t size) {
	void *p;
	p = malloc_test(nitems*size);
	if (p) {
		struct s_block *pp = (void *)p - sizeof(struct s_block);
		memset(p, 0, pp->size);
	}
	
	return p;
}


int merge(struct s_block *pp) {
	
	if (pp->prev == NULL && pp->next == NULL) {
                head = NULL;
                brk(pp);
                return 0;
        }
	
	if (pp->isfree && pp->next && pp->next->isfree) {
		pp->size += sizeof(struct s_block) + pp->next->size;
		pp->next = pp->next->next;
		if (pp->next) {
			pp->next->prev = pp;
		}
	}

	
	return 1;
}

void free_test(void *p) {
	struct s_block *pp;
	if (p > (void *)head && p < sbrk(0)) {
		pp = p - sizeof(struct s_block);
		if (pp->addr == p) {
			pp->isfree = 1;
			if (merge(pp) && pp->prev) {
				merge(pp->prev);
			}
			
		}
	}
}

void test() {
	struct s_block *head1 = head;
        while(head1) {
                printf("isfree is %d\n", head1->isfree);
                printf("size is %d\n", head1->size);
                head1 = head1->next;
        }
	
}


int main(int argc, const char* argv[])
{
	
	int *p;
	p = (int *)calloc_test(sizeof(int), 1);
	*p = 6;
	//free_test(p);
	
	
	
	char *p1;
	p1 = (char *)malloc_test(60);


	strcpy(p1, "12fdfsdfds");

	free_test(p1);

	char *p2;
	p2 = (char *)malloc_test(5);

	struct s_block *pp = (void *)p2-sizeof(struct s_block);

	
	//memset(p2, 'c', 5);
	strncpy(p2, "12345", 5);
	//strcpy(p2, "12345");
	
	test();
	//struct s_block *pp = (void *)p2-sizeof(struct s_block);
	//printf("%d\n", pp->next->size);
	
	return 0;
}


