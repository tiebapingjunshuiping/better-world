#include"malloc.h"
typedef struct tree{
	unsigned int type;/*type%2:0 data 1 other tree;type/2:length*/
	union{
		char* c;
		struct tree* t;
	}car;
	struct tree* cdr;
}TREE;
TREE* cons(int type,void* car,TREE* cdr){
	TREE* result=malloc(sizeof(TREE));
	result->type=type;
	if(type%2==0){
		result->car.c=car;
	}
	else{
		result->car.t=car;
	}
	result->cdr=cdr;
	return result; 
}
void free_tree(TREE* t){
	if(t==NULL) return;
	else{
		if(t->type%2==0){
			free(t->car.c);
		}
		else{
			free_tree(t->car.t);
		}
		free_tree(t->cdr);
	}
}
#define END 192
#define LIST 128
#define CONTENT 64
#define SMALL 0
#define MIDDLE 1
int error_code;
#define TOO_FEW_ARGUMENTS 2
#define TOO_MANY_ARGUMENTS 3
#define UNDEFINED_SYMBOL 4
#define LANK_OF_DATA 5
#define RECIEVE_ERROR 6
char type(char c){
	return c&(128+64);	
} 
char flag(char c){
	return c&1;
} 
jmp_buf j;
int (*get_data) (unsigned int,char*);
void get_data_with_error(unsigned  int i,char* buf){ /*先送出的先抵达，低位在先*/ 
	if((*get_data)(i,buf)==0){
		longjmp(j,RECIEVE_ERROR);
	}
}
unsigned int char2unsigned(char c){
	return c<0?256+c:c;
}
#define eq(a,b) (char2unsigned(a^b)==0)
unsigned int get_length(char f){
	char temp_space;
	unsigned int result;
	get_data_with_error(1,&temp_space);
	result=char2unsigned(temp_space);
	if(eq(f,SMALL)){
		return result;
	}
	else{
		get_data_with_error(1,&temp_space);
		result=result+char2unsigned(temp_space)*128;
		return result;
	}
	
}
void get_length_and_type(char* pt,unsigned int * pl){
	char head;
	get_data_with_error(1,&head);
	*pt=type(head);
	char f=flag(head);
	*pl=get_length(f);
}
void print_tree(TREE*);
TREE* decode_helper(){
	TREE* stack=NULL;
	char t;
	unsigned int l;
	get_length_and_type(&t,&l);
	while(!eq(t,END)){
		if(eq(t,LIST)){
			unsigned int i;
			print_tree(stack);
			printf("%d\n",l);
			TREE* cur=stack;
			for(i=1;i<=l-1;i++){
				if(cur==NULL){
					free_tree(stack);
					longjmp(j,TOO_FEW_ARGUMENTS);	
				}
				else{
					cur=cur->cdr;
				}
			}
			stack=cons(1,stack,cur->cdr);
			cur->cdr=NULL;
		}
		else if(eq(t,CONTENT)){
			char* leaf=malloc(l);
			get_data_with_error(l,leaf);
			printf("%u$ ",l);
			stack=cons(l*2,leaf,stack);
		}
		else{
			free_tree(stack);
			longjmp(j,UNDEFINED_SYMBOL);
		}
		get_length_and_type(&t,&l);
	}
	return stack; 
}
TREE* decode(int (*get_data_func) (unsigned int,char*)){
	get_data=get_data_func;
	int i=setjmp(j);
	if(i==0){
		return decode_helper();
	}
	else{
		error_code=i;
		return NULL;
	}
}
jmp_buf j2;
int (*send_func)(unsigned int,char*);
void send_with_error(unsigned int i,char* buf){
	if((*send_func)(i,buf)==0){
		longjmp(j2,1);
	} 
}
void send_head(unsigned int l,char flag){
	if(l>255){
		char c[3]={ flag | MIDDLE,l%256,l/256};
		send_with_error(3,c);
	}
	else{
		char c[2]={ flag | SMALL,l};
		send_with_error(2,c);
	}
}
unsigned int encode_and_send_helper(TREE* t,unsigned int i){ /*该函数返回值表示发送的列表长度，返回后立刻发送LIST操作符*/ 
	if(t==NULL){
		return i;
	}
	else{
		int k=encode_and_send_helper(t->cdr,i+1);
		if(t->type%2==0){
			unsigned int l=t->type/2;
			send_head(l,CONTENT);
			send_with_error(t->type/2,t->car.c);
		}
		else{
			unsigned int j=encode_and_send_helper(t->car.t,0);
			send_head(j,LIST);
		}
		return k;
	}
}
int encode_and_send(TREE* t,int (*send_data)(unsigned int,char*)){
	send_func=send_data;
	int i=setjmp(j2);
	if(i==0){
		unsigned int j=encode_and_send_helper(t,0);
		send_head(j,LIST);
		char c=END;
		send_with_error(1,&c);
		return 1;
	}
	else{
		return 0;
	}	
}
