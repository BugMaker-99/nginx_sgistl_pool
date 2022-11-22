#include "ngx_mem_pool.h"

using namespace std;

NgxMemPool::NgxMemPool(size_t size) {
    pool_ = (ngx_pool_s*)malloc(ngx_align(size, NGX_POOL_ALIGNMENT));

    if (pool_ == nullptr) {
        return ;
    }

    pool_->d.last = (u_char*)pool_ + sizeof(ngx_pool_s);
    pool_->d.end = (u_char*)pool_ + size;
    pool_->d.next = nullptr;
    pool_->d.failed = 0;

    size = size - sizeof(ngx_pool_s);
    pool_->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    pool_->current = pool_;
    pool_->large = nullptr;
    pool_->cleanup = nullptr;
}

// 1.调用所有的预置的清理函数 2.释放大块内存 3.释放小块内存池所有内存
NgxMemPool::~NgxMemPool() {
    ngx_pool_s* p, * next_block;
    ngx_pool_large_s* l;
    ngx_pool_cleanup_s* c;

    cout << "开始销毁内存池..." << endl;
    // 释放外部资源
    for (c = pool_->cleanup; c; c = c->next) {
        if (c->handler) {
            cout << "run cleanup: " << c << endl;;
            c->handler(c->data);
        }
    };

    // 释放大块内存
    for (l = pool_->large; l; l = l->next) {
        if (l->alloc) {
            cout << "free 大块内存:" << l->alloc << endl;
            free(l->alloc);
        }
    }

    // 释放小块内存
    for (p = pool_, next_block = pool_->d.next; /* void */; p = next_block, next_block = next_block->d.next) {
        cout << "free 小块内存 :" << p << endl;
        free(p);
        if (next_block == nullptr) {
            break;
        }
    }
}

void* NgxMemPool::ngx_palloc(size_t size){
    if (size <= pool_->max) {
        return ngx_palloc_small(size, true);
    }
    return ngx_palloc_large(size);
}

// 不考虑字节内存对齐，向内存池申请size字节
void* NgxMemPool::ngx_pnalloc(size_t size) {
    if (size <= pool_->max) {
        return ngx_palloc_small(size, false);
    }
    return ngx_palloc_large(size);
}

// 调用的是ngx_palloc，且会把内存初始化为0
void* NgxMemPool::ngx_pcalloc(size_t size) {
    void   *p;

    p = ngx_palloc(size);
    if (p) {
        memset(p, 0, size);
    }

    return p;
}

// 小块内存分配
void* NgxMemPool::ngx_palloc_small(size_t size, bool is_align) {
    u_char* m;
    ngx_pool_s* p = pool_->current;

    do {
        m = p->d.last;

        if (is_align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }

        if ((size_t)(p->d.end - m) >= size) {
            p->d.last = m + size;
            return m;
        }

        p = p->d.next;

    } while (p);

    return ngx_palloc_block(size);
}

// 当前小块内存池的块不够时，重新分配新的小块内存池
void* NgxMemPool::ngx_palloc_block(size_t size) {
    u_char       *m;
    size_t       pool_size;
    ngx_pool_s   *p, *new_block;

    pool_size = (size_t)(pool_->d.end - (u_char*)pool_);

    m = (u_char*)malloc(ngx_align(pool_size, NGX_POOL_ALIGNMENT));

    if (m == nullptr) {
        return nullptr;
    }

    new_block = (ngx_pool_s*)m;

    new_block->d.end = m + pool_size;
    new_block->d.next = nullptr;
    new_block->d.failed = 0;

    m += sizeof(ngx_pool_data_s);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    new_block->d.last = m + size;

    for (p = pool_->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool_->current = p->d.next;
        }
    }

    p->d.next = new_block;

    return m;
}

// 大块内存分配
void* NgxMemPool::ngx_palloc_large(size_t size) {
    void* p;
    ngx_uint_t         n;
    ngx_pool_large_s* large;

    p = malloc(ngx_align(size, NGX_ALIGNMENT));
    if (p == nullptr) {
        return nullptr;
    }

    n = 0;

    for (large = pool_->large; large; large = large->next) {
        if (large->alloc == nullptr) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = (ngx_pool_large_s*)ngx_palloc_small(sizeof(ngx_pool_large_s), true);
    if (large == nullptr) {
        cout << "free:" << p << endl;
        free(p);
        return nullptr;
    }

    large->alloc = p;
    large->next = pool_->large;
    pool_->large = large;

    return p;
}

// ngx_pfree专门用于释放大块内存
void NgxMemPool::ngx_pfree(void* p) {
    ngx_pool_large_s* l;

    for (l = pool_->large; l; l = l->next) {
        if (p == l->alloc) {
            cout << "free 大块内存:" << l->alloc << endl;
            free(l->alloc);
            l->alloc = nullptr;
            return;
        }
    }
}

// 重置内存池（释放大块内存，重置小块内存）
void NgxMemPool::ngx_reset_pool() {
    // 遍历cleanup链表，释放占用的外部资源
    for (ngx_pool_cleanup_s* c = pool_->cleanup; c; c = c->next) {
        if (c->handler) {
            cout << "run cleanup: " << c << endl;;
            c->handler(c->data);
        }
    };

    for (ngx_pool_large_s* l = pool_->large; l; l = l->next) {
        if (l->alloc) {
            cout << "free 大块内存:" << l->alloc << endl;
            free(l->alloc);
        }
    }

    pool_->d.last = (u_char*)pool_ + sizeof(ngx_pool_s);
    pool_->d.failed = 0;

    for (ngx_pool_s* p = pool_->d.next; p; p = p->d.next) {
        p->d.last = (u_char*)p + sizeof(ngx_pool_data_s);
        p->d.failed = 0;
    }

    pool_->current = pool_;
    pool_->large = nullptr;
}

// 添加清理的回调函数，会在析构函数中调用
// size表示pool_->cleanup->data指针的大小
ngx_pool_cleanup_s* NgxMemPool::ngx_pool_cleanup_add(size_t size) {
    // 存放清理函数信息的块
    ngx_pool_cleanup_s* clean_block = (ngx_pool_cleanup_s*)ngx_palloc(sizeof(ngx_pool_cleanup_s));

    if (clean_block == nullptr) {
        return nullptr;
    }

    if (size) {
        clean_block->data = ngx_palloc(size);
        if (clean_block->data == nullptr) {
            return nullptr;
        }
    }else {
        clean_block->data = nullptr;
    }
    clean_block->handler = nullptr;

    // 头插法插入clean_block
    clean_block->next = pool_->cleanup;
    pool_->cleanup = clean_block;

    cout<<"add cleanup: " << clean_block << endl;;

    return clean_block;
}
