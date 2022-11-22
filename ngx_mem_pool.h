#ifndef NGX_MEM_POOL
#define NGX_MEM_POOL

#include <iostream>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

using namespace std;

// 类型重定义
using u_char = unsigned char;
using ngx_uint_t = unsigned int;

// 销毁内存池前调用的函数，一般用于清理外部资源
typedef void (*ngx_pool_cleanup_pt)(void* data);

struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt       handler;   // 函数指针，保存清理函数的地址
    void                      *data;     // 指向需要释放的资源
    ngx_pool_cleanup_s        *next;     // 释放资源的函数都放在一个链表，用next指向这个链表
};

// 大块内存的头部信息
struct ngx_pool_large_s {
    ngx_pool_large_s   *next;
    void               *alloc;
};

// 类型声明
struct ngx_pool_s;
// nginx头部信息
struct ngx_pool_data_s {
    u_char            *last;         // 指向可用内存的起始地址
    u_char            *end;          // 指向可用内存的末尾地址
    ngx_pool_s        *next;         // 指向下一个内存块 
    ngx_uint_t        failed;        // 当前内存块分配空间失败的次数
};

// nginx头部信息和管理成员信息
struct ngx_pool_s {
    ngx_pool_data_s        d;         // ngx_pool_data_s头信息
    size_t                 max;       // 小块内存和大块内存的分界线
    ngx_pool_s             *current;  // 指向可用于分配空间的第一个内存块（failed < 4）的起始地址
    ngx_pool_large_s       *large;    // 指向大块内存（链表）的入口地址
    ngx_pool_cleanup_s     *cleanup;  // 指向所有预置的清理函数（链表）的入口
};

// 数值n调整为align的倍数
#define ngx_align(n, align)  (((n) + (align - 1)) & ~(align - 1))
// 指针p调整到align整数倍的地址
#define ngx_align_ptr(p, align)  (u_char *) (((ngx_uint_t) (p) + ((ngx_uint_t) align - 1)) & ~((ngx_uint_t) align - 1))


// 默认一个物理页面的大小
const int ngx_pagesize = 4096;
// 小块内存池允许分配的最大空间
const int NGX_MAX_ALLOC_FROM_POOL = ngx_pagesize - 1;
const int NGX_DEFAULT_POOL_SIZE = 16 * 1024;
// 内存池对齐参数
const int NGX_POOL_ALIGNMENT = 16;
// 内存池最小空间：可以放入一个小块内存头信息以及两个大块内存的头信息
const int NGX_MIN_POOL_SIZE = ngx_align((sizeof(ngx_pool_s) + 2 * sizeof(ngx_pool_large_s)), NGX_POOL_ALIGNMENT);
// nginx小块内存分配时的对齐参数
const int NGX_ALIGNMENT = sizeof(unsigned long);

class NgxMemPool {
public:
    NgxMemPool(size_t size);
    ~NgxMemPool();
    // 创建指定大小的内存池
    // bool ngx_create_pool(size_t size);
    // 考虑字节内存对齐，向内存池申请size字节
    void* ngx_palloc(size_t size);
    // 不考虑字节内存对齐，向内存池申请size字节
    void* ngx_pnalloc(size_t size);
    // 调用的是ngx_palloc，且会把内存初始化为0
    void* ngx_pcalloc(size_t size);
    // 释放大块内存
    void ngx_pfree(void* p);
    // 重置内存池（释放大块内存，重置小块内存）
    void ngx_reset_pool();
    // 销毁内存池，释放所有的资源
    // void ngx_destroy_pool();
    // 添加清理的回调函数，会在ngx_destroy_pool中调用
    ngx_pool_cleanup_s* ngx_pool_cleanup_add(size_t size);
private:
    ngx_pool_s* pool_;
    // 小块内存分配
    void* ngx_palloc_small(size_t size, bool is_align);
    // 大块内存分配
    void* ngx_palloc_large(size_t size);
    // 当前小块内存池的剩余空间不够时，重新分配新的小块内存池
    void* ngx_palloc_block(size_t size);
};

#endif