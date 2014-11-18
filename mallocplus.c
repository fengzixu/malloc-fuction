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
	t_block pre;
	t_block next;
	void *magic_ptr;
	int padding;	//字节填充
	char data[1];	//数据字段中的第一个字节，也就是malloc函数返回的地址
};

/*编写一个查找一个合适块的函数，利用首次适应算法*/
t_block find_a_block(t_block *last,size_t size){
	//其中的last在之后追加的时候很有用，保存了尾部
	t_block temp = (t_block)firstblock;
	while(temp&&!(temp->free&&temp->size>=size)){
		*last = temp;
		temp = temp->next;
	}
	return temp;
}


/*如果现有的块都不能满足，则要开辟一个新的块*/

t_block add_block(t_block last,size_t size){
	
	t_block t = (t_block)sbrk(0);	//获取现在的break指针
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
	
	t_block newone =(t_block)(b->data+size);	//跨过前一个块的整个数据区
	newone->size = b->size - size - BLOCK_SIZE;	//新分出来的数据区的大小
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
	if(firstblock)	//如果链表不为空，也就是之前已经在属于堆的内存部分开辟了一些空间
	{
		last = (t_block)firstblock;	//保存链表头
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
		firstblock = b;
	}
	return b->data;



}


void *calloc(size_t number, size_t size){

	size_t *ptr;
	size_t s8,i;
	ptr = (size_t *)malloc(number*size);
	if(ptr){
	
	s8 = align8(number*size)>>3;   //算出有多少个八个字节
	for(i = 0; i < s8; i ++)	//以八个字节为单位进行初始化
	
		ptr[i] = 0;
	}
	return ptr;
}

t_block get_block_add(void *p){			//获得这一块中meta的地址，找到magic_ptr返回

	char *temp = (char *)p;
	return (t_block)(p = temp -= BLOCK_SIZE);
}

int judge_ptr(void *p){		//传递进来的是数据区的地址，因为用户也只能够得到数据区的地址

	if(firstblock){
	
	if(p>firstblock&&p<sbrk(0))	//如果传递进来的地址在堆的可映射合法空间
		return p==get_block_add(p)->magic_ptr;	//查看穿进来的地址和malloc申请空间时设置的数据区地址是否相等
	}
	
	return 0;
}


t_block merge_block(t_block p){

	if(p->next&&p->next->free)	//如果要释放的一块的后面有一个已经分配过的块，但还是空闲的
	{
		
		p->size += BLOCK_SIZE+p->next->size;		//更改块的大小
		p->next=p->next->next;				//把合并的块和后续的块连起来
		if(p->next)					//同时后续的第一个块的pre指针要进行更改。
			p->next->pre = p;
	}

	return p;
}

void free(void *p){

	t_block ptr;
	if(judge_ptr(p))	//判断指针是否有效
	{
	
		ptr = get_block_add(p);		//获取meta数据块的指针
		ptr->free = 1;			//这个区域被释放，空闲设为正
		if(ptr->pre && ptr->pre->free)	//前面不为空，并且是空闲的，进行合并，之所以要判断是否为空闲，因为合并函数一直都是向后合并
			ptr = merge_block(ptr->pre);
		else if(ptr->next)		//如果后面的不为空，合并函数内部可以判断是否后面的块为空闲，但是不再改动指针，因为合并之后的空闲区域地址就是我们要释放的这一块的地址。
			merge_block(ptr);
		else
		{
			if(ptr->pre)		//如果两边都不是空，还是有两中可能，一种是两边什么都没有，这一块本来就是堆空间中的最后一块，另一种是前面的已经被占用了
				ptr->pre->next = NULL;
			else
				firstblock = NULL;
			brk(ptr);	//重新设置break指针的值，因为无论堆内存空间中是否为空，释放掉之后都要重新设置
		}
	}
}


void copy_data(t_block start, t_block end){

	size_t *sdata, *edata;
	size_t i;
	sdata = (size_t *)start->magic_ptr;
	edata = (size_t *)end->magic_ptr;
	for(i = 0; (i * 8) < start->size && (i * 8) < end->size; i++)
		edata[i] = sdata[i];
}

void *realloc(void *p, size_t size){
	
	size_t s;
	t_block newone,b;
	void *new_ptr;
	if(!p)		//如果指针为空则默认执行malloc函数
		return malloc(size);
	
	if(judge_ptr(p))	//查看p是不是malloc返回的指针
	{
		s = align8(size);
		b = get_block_add(p);
		if(b->size>=s)			//如果原来分配的大小大于要求的，那么就不再重新分配
		{
			
			if(b->size-s>=(BLOCK_SIZE+8))	//在满足自身要求的同时还有可以分裂的空间
			{
				split_block(b,s);
			}
		}
		else					//如果本身的空间不够，看是否可以向后进行扩展
		{

			if(b->next&&b->free)
			{
				if((b->size+BLOCK_SIZE+b->next->size)>=s)	//如果加上后面的空间满足要求大小
				{
					merge_block(b);
					 if(b->size-s>=(BLOCK_SIZE+8))   //在满足自身要求的同时还有可以分裂的空间
					{
					        split_block(b,s);
					}


				}
			}
		
			else				//向后的空间不能扩展且现有的空间又满足不了需求就只能重新malloc一段内存然后复制数据
			{
		
					new_ptr = malloc(size);
					if(!new_ptr)
						return NULL;
					t_block temp_ptr = get_block_add(new_ptr);
					copy_data(b,temp_ptr);
					free(p);				//数据复制过去之后，旧的空间里面的东西也没用了。要删除掉，防止内存泄露
					return new_ptr;
			}
		}
		return p;
	}
	return NULL;

}








int main()
{
	int i = 0;
	int *p = (int*)malloc(sizeof(int)*5);
	for(i = 2; i <= 6; i++)
		scanf("%d",&p[i]);
	for(i = 2; i <= 6; i++)
		printf("p[%d] = %d\n", i,p[i]);
	
	return 0;
}










   
