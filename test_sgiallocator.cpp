#include "sgi_allocator.h"
#include <vector>

using namespace std;

int main() {
	// 容器初始化的时候会调用allocate会申请空间初始化内存池
	// 传入allocator的参数__n为1，应该是申请一个元素的空间

	// 第一次push_back的时候会开辟一定的空间，然后再构造对象，后面push_back的时候若空间够用就不会再开辟空间，而直接构造对象
	// 若空间不够用，则重新申请内存空间，再到新空间做定位new，将数据拷贝到新空间，然后调用deallocate归还chunk块
	vector<int, SGIAllocator<int>> vec;
	for (int i = 0; i < 20; i++) {
		int val = rand() % 100;
		cout << val << " ";
		// push_back的同时，容器也会扩容，VS是1.5倍传入allocate的参数是1、2、3、4、6、9、13、19、28
		vec.push_back(val);
	}
	cout << endl;
	for (int i = 0; i < 20; i++) {
		cout << vec[i] << " ";
	}
	cout << endl;
	return 0;
}
