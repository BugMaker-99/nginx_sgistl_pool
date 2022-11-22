# nginx_sgistl_pool
提取nginx内存池部分源码用C++11重写<br>
提取SGI STL部分源码编写的空间配置器<br>

注意在64位Linux下，指针类型占8字节，在宏函数ngx_align_ptr中会把指针类型p强转为数据类型，应该根据特定平台指针宽度调整强转的数据类型大小
