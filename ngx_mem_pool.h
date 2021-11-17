#pragma once
#include <iostream>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

using namespace std;

// �����ض���
using u_char = unsigned char;
using ngx_uint_t = unsigned int;

// �����ڴ��ǰ���õĺ�����һ�����������ⲿ��Դ
typedef void (*ngx_pool_cleanup_pt)(void* data);

struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt       handler;   // ����ָ�룬�����������ĵ�ַ
    void                      *data;     // ָ����Ҫ�ͷŵ���Դ
    ngx_pool_cleanup_s        *next;     // �ͷ���Դ�ĺ���������һ��������nextָ���������
};

// ����ڴ��ͷ����Ϣ
struct ngx_pool_large_s {
    ngx_pool_large_s   *next;
    void               *alloc;
};

// ��������
struct ngx_pool_s;
// nginxͷ����Ϣ
struct ngx_pool_data_s {
    u_char            *last;         // ָ������ڴ����ʼ��ַ
    u_char            *end;          // ָ������ڴ��ĩβ��ַ
    ngx_pool_s        *next;         // ָ����һ���ڴ�� 
    ngx_uint_t        failed;        // ��ǰ�ڴ�����ռ�ʧ�ܵĴ���
};

// nginxͷ����Ϣ�͹����Ա��Ϣ
struct ngx_pool_s {
    ngx_pool_data_s        d;
    size_t                 max;
    ngx_pool_s             *current;  // ָ������ڷ���ռ�ĵ�һ���ڴ�飨failed < 4������ʼ��ַ
    ngx_pool_large_s       *large;    // ָ�����ڴ棨��������ڵ�ַ
    ngx_pool_cleanup_s     *cleanup;  // ָ������Ԥ�õ������������������
};

// ��ֵn����Ϊalign�ı���
#define ngx_align(n, align)  (((n) + (align - 1)) & ~(align - 1))
// ָ��p������align�������ĵ�ַ
#define ngx_align_ptr(p, align)  (u_char *) (((ngx_uint_t) (p) + ((ngx_uint_t) align - 1)) & ~((ngx_uint_t) align - 1))


// Ĭ��һ������ҳ��Ĵ�С
const int ngx_pagesize = 4096;
// С���ڴ�������������ռ�
const int NGX_MAX_ALLOC_FROM_POOL = ngx_pagesize - 1;
const int NGX_DEFAULT_POOL_SIZE = 16 * 1024;
// �ڴ�ض������
const int NGX_POOL_ALIGNMENT = 16;
// �ڴ����С�ռ䣺���Է���һ��С���ڴ�ͷ��Ϣ�Լ���������ڴ��ͷ��Ϣ
const int NGX_MIN_POOL_SIZE = ngx_align((sizeof(ngx_pool_s) + 2 * sizeof(ngx_pool_large_s)), NGX_POOL_ALIGNMENT);
// nginxС���ڴ����ʱ�Ķ������
const int NGX_ALIGNMENT = sizeof(unsigned long);

class NgxMemPool {
public:
    NgxMemPool(size_t size);
    ~NgxMemPool();
    // ����ָ����С���ڴ��
    // bool ngx_create_pool(size_t size);
    // �����ֽ��ڴ���룬���ڴ������size�ֽ�
    void* ngx_palloc(size_t size);
    // �������ֽ��ڴ���룬���ڴ������size�ֽ�
    void* ngx_pnalloc(size_t size);
    // ���õ���ngx_palloc���һ���ڴ��ʼ��Ϊ0
    void* ngx_pcalloc(size_t size);
    // �ͷŴ���ڴ�
    void ngx_pfree(void* p);
    // �����ڴ�أ��ͷŴ���ڴ棬����С���ڴ棩
    void ngx_reset_pool();
    // �����ڴ�أ��ͷ����е���Դ
    // void ngx_destroy_pool();
    // �������Ļص�����������ngx_destroy_pool�е���
    ngx_pool_cleanup_s* ngx_pool_cleanup_add(size_t size);
private:
    ngx_pool_s* pool_;
    // С���ڴ����
    void* ngx_palloc_small(size_t size, bool is_align);
    // ����ڴ����
    void* ngx_palloc_large(size_t size);
    // ��ǰС���ڴ�صĿ鲻��ʱ�����·����µ�С���ڴ��
    void* ngx_palloc_block(size_t size);
};
