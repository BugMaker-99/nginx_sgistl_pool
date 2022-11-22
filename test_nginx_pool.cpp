#include "ngx_mem_pool.h"

typedef struct Data stData;
struct Data {
    char* ptr;
    FILE* pfile;
};

void func1(void* p) {
    char* p_ = (char*)p;
    cout << "free ptr mem : "<< p << endl;
    free(p_);
}

void func2(void* fp) {
    FILE* fp_ = (FILE*)fp;
    cout << "close file : " << fp << endl;
    fclose(fp_);
}

int main_nginx() {
    // max = 512 - sizeof(ngx_pool_t)
    // 创建总空间为512字节的nginx内存块
    NgxMemPool pool(512);

    // 从小块内存池分配的
    void* p1 = pool.ngx_palloc(128);

    if (p1 == nullptr) {
        cout<<"ngx_palloc 128 bytes fail..."<<endl;
        return -1;
    }

    // 从大块内存池分配的
    stData* p2 = (stData*)pool.ngx_palloc(512);
    if (p2 == nullptr) {
        cout << "ngx_palloc 512 bytes fail..."<<endl;
        return -1;
    }

    // 占用外部堆内存
    p2->ptr = (char*)malloc(12);
    strcpy(p2->ptr, "hello world");
    // 文件描述符
    p2->pfile = fopen("data.txt", "w");

    ngx_pool_cleanup_s* c1 = pool.ngx_pool_cleanup_add(sizeof(char*));
    c1->handler = func1;   // 设置回调函数
    c1->data = p2->ptr;    // 设置资源地址

    ngx_pool_cleanup_s* c2 = pool.ngx_pool_cleanup_add(sizeof(FILE*));
    c2->handler = func2;
    c2->data = p2->pfile;

    return 0;
}
