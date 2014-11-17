#include<sys/types.h>
#include<unistd.h>
#include<stdio.h>

#define BLOCK_SIZE 24      //记录块信息一个节点的大小
typedef struct s_block *t_block;
void *firstblock = NULL;
//此结构体记录了每在堆中分配一个块，这个块的一些详细的信息
struct s_block{
	size_t size;	//每一块数据区的大小
	int free;	//是否为空的标志位
	t_block next;	//指向下一个节点的指针
	int padding;	//字节填充
	char data[1];	//数据字段中的第一个字节，也就是malloc函数返回的地址
};

/*编写一个查找一个合适块的函数，利用首次适应算法*/
t_block find_a_block(t_block *last,size_t size){
	//其中的last在之后追加的时候很有用，保存了尾部
	t_block temp = firstblock;
	while(temp&&!(temp->free&&temp->size>=size)){
		*last = temp;
		temp = temp->next;
	}
	return temp;
}


/*如果现有的块都不能满足，则要开辟一个新的块*/

t_block add_block(t_block *last,size_t size){
	
	t_block t = sbrk(0);	//获取现在的break指针
	if(sbrk(BLOCK_SIZE+size) == (void*)-1)	//判断是否移动指针成功
		return NULL;
	t->size = size;		//设置好新块的详细信息
	t->next = NULL;
	if(last)			//接上最后新分配的节点
		last->next = t;
	t->free = 0;			//初始化信息
	return t;
	
}

/*处理内碎片*/
void split_block(t_block b, size_t size){
	
	t_block newone = b->data+s;	//跨过前一个块的整个数据区
	newone->size = b->size - s - BLOCK_SIZE;	//新分出来的数据区的大小
	newone->free = 1;
	newone->next = b->next;
	b->next = newone;	//把分开的节点接起来
	b->size = size;
}

/*为了字节对齐，暂时不太明白是什么意思，可能是对内存管理的一种优化把*/
size_t align8(size_t s) {
        if(s & 0x7 == 0)
	    return s;
	return ((s >> 3) + 1) << 3;
 }
void *malloc(size_t size){

	t_block t,b,last;
	size_t s = align8(size);
	if(first_block)	//如果链表不为空，也就是之前已经在属于堆的内存部分开辟了一些空间
	{
		last = first_block;	//保存链表头
		b = find_a_block(&last,s);	//找看是否有合适的块
		if(b)	//找到
		{ 
			if((b->size-s)>=(BLOCK_SIZE+8))		//看是否可以处理内碎片 
				split_block(b,s);
			b->free = 0;		//把剩余的部分已经分裂出去，剩下的都已经用完了，所以不再可用

		}
	
		else{	//如果没找到合适的
		
			b = add_block(last,size);	//在末尾添加一块	
			if(!b)
				return NULL;

		   }
	}
	else{
	//如果链表是空的，那么就把这个作为链表的头部
		
		b = add_block(NULL,size);
		if(!b)
			return NULL;
		first_block = b;
	}
	return b->data;



}


void *calloc(size_t number, size_t size){

	size_t *ptr;
	size_t s8,i;
	ptr = malloc(number*size);
	if(ptr){
	
	s8 = align8(number*size)>>3;   //算出有多少个八个字节
	for(i = 0; i < s8; i ++)	//以八个字节为单位进行初始化
	
		ptr[i] = 0;
	}
	return ptr;
}




















