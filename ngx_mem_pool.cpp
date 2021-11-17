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

// 1.�������е�Ԥ�õ������� 2.�ͷŴ���ڴ� 3.�ͷ�С���ڴ�������ڴ�
NgxMemPool::~NgxMemPool() {
    ngx_pool_s* p, * next_block;
    ngx_pool_large_s* l;
    ngx_pool_cleanup_s* c;

    cout << "��ʼ�����ڴ��..." << endl;
    // �ͷ��ⲿ��Դ
    for (c = pool_->cleanup; c; c = c->next) {
        if (c->handler) {
            cout << "run cleanup: " << c << endl;;
            c->handler(c->data);
        }
    };

    // �ͷŴ���ڴ�
    for (l = pool_->large; l; l = l->next) {
        if (l->alloc) {
            cout << "free ����ڴ�:" << l->alloc << endl;
            free(l->alloc);
        }
    }

    // �ͷ�С���ڴ�
    for (p = pool_, next_block = pool_->d.next; /* void */; p = next_block, next_block = next_block->d.next) {
        cout << "free С���ڴ� :" << p << endl;
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

// �������ֽ��ڴ���룬���ڴ������size�ֽ�
void* NgxMemPool::ngx_pnalloc(size_t size) {
    if (size <= pool_->max) {
        return ngx_palloc_small(size, false);
    }
    return ngx_palloc_large(size);
}

// ���õ���ngx_palloc���һ���ڴ��ʼ��Ϊ0
void* NgxMemPool::ngx_pcalloc(size_t size) {
    void   *p;

    p = ngx_palloc(size);
    if (p) {
        memset(p, 0, size);
    }

    return p;
}

// С���ڴ����
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

// ��ǰС���ڴ�صĿ鲻��ʱ�����·����µ�С���ڴ��
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

// ����ڴ����
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

void NgxMemPool::ngx_pfree(void* p) {
    ngx_pool_large_s* l;

    for (l = pool_->large; l; l = l->next) {
        if (p == l->alloc) {
            cout << "free ����ڴ�:" << l->alloc << endl;
            free(l->alloc);
            l->alloc = nullptr;
            return;
        }
    }
}

// �����ڴ�أ��ͷŴ���ڴ棬����С���ڴ棩
void NgxMemPool::ngx_reset_pool() {
    
    ngx_pool_large_s* l;

    for (l = pool_->large; l; l = l->next) {
        if (l->alloc) {
            cout << "free ����ڴ�:" << l->alloc << endl;
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

// �������Ļص��������������������е���
// size��ʾpool_->cleanup->dataָ��Ĵ�С
ngx_pool_cleanup_s* NgxMemPool::ngx_pool_cleanup_add(size_t size) {
    // �����������Ϣ�Ŀ�
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
    clean_block->next = pool_->cleanup;

    pool_->cleanup = clean_block;

    cout<<"add cleanup: " << clean_block << endl;;

    return clean_block;
}
