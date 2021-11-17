#include "sgi_allocator.h"
#include <vector>

using namespace std;

int main() {
	// 容器初始化的时候会调用allocate会申请空间初始化内存池
	// 传入allocator的参数__n为1，应该是申请一个元素的空间，尝试初始化内存池

	// 初始化完成后会再次调用allocate，传入的参数__n就是我们这里手动传入的10
	
	// 里面static _S_free_list[]的地址会改变，不明白为什么。
	// 就比如初始化vector的时候传入allocate的__n为1以及传入allocate为我们手动设置的10时，_S_free_list的地址不一样

	// 然后push_back的时候就会一直使用后面的这个_S_free_list的地址
	// 第一次push_back的时候会开辟一定的空间，然后再构造对象，后面push_back的时候若空间够用就不会再开辟空间，而直接构造对象
	vector<int, SGIAllocator<int>> vec(10);
	for (int i = 0; i < 20; i++) {
		int val = rand() % 100;
		cout << val << " ";
		vec.push_back(val);
	}
	cout << endl;
	for (int i = 10; i < 30; i++) {
		cout << vec[i] << " ";
	}
	cout << endl;
	return 0;
}
