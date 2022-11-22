# nginx_sgistl_pool
提取nginx内存池部分源码用C++11重写<br>
提取SGI STL部分源码编写的空间配置器<br>

注意在64位Linux下，指针类型占8字节，在宏函数ngx_align_ptr中会把指针类型p强转为数据类型，应该根据特定平台指针宽度调整强转的数据类型大小<br>
指针p调整到align整数倍的地址，在64位linux下，p是指针类型，占8字节，若ngx_uint_t为int，只占4字节，强转时会发生数据截断，再访问地址会发生Segmentation fault<br>
