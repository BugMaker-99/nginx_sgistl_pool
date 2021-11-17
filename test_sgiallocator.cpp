#include "sgi_allocator.h"
#include <vector>

using namespace std;

int main() {
	// ������ʼ����ʱ������allocate������ռ��ʼ���ڴ��
	// ����allocator�Ĳ���__nΪ1��Ӧ��������һ��Ԫ�صĿռ䣬���Գ�ʼ���ڴ��

	// ��ʼ����ɺ���ٴε���allocate������Ĳ���__n�������������ֶ������10
	
	// ����static _S_free_list[]�ĵ�ַ��ı䣬������Ϊʲô��
	// �ͱ����ʼ��vector��ʱ����allocate��__nΪ1�Լ�����allocateΪ�����ֶ����õ�10ʱ��_S_free_list�ĵ�ַ��һ��

	// Ȼ��push_back��ʱ��ͻ�һֱʹ�ú�������_S_free_list�ĵ�ַ
	// ��һ��push_back��ʱ��Ὺ��һ���Ŀռ䣬Ȼ���ٹ�����󣬺���push_back��ʱ�����ռ乻�þͲ����ٿ��ٿռ䣬��ֱ�ӹ������
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
