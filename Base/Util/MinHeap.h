#ifndef MinHeap_H
#define MinHeap_H

#include <vector>
#include <iostream>
 
template<typename T>
class MinHeap {
private:
    std::vector<T> data;
    int _maxSize = 0;

    MinHeap::MinHeap(int size)
    {
        _maxSize = size;
        // data.resize(size);
    }
 
    void siftUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (data[p] <= data[i]) break;
            std::swap(data[p], data[i]);
            i = p;
        }
    }
 
    void siftDown(int i, int n) {
        while (true) {
            int j = i * 2 + 1;
            if (j >= n || j + 1 < n && data[j + 1] < data[j]) j = j + 1;
            if (j >= n || data[i] <= data[j]) break;
            std::swap(data[i], data[j]);
            i = j;
        }
    }
 
public:
    void push(T v) {
        if (data.size() >= _maxSize) {
            data[0] = v;
            siftDown(0, data.size());
        } else {
            data.push_back(v);
            siftUp(data.size() - 1);
        }
        
    }
 
    void pop() {
        std::swap(data[0], data[data.size() - 1]);
        data.pop_back();
        if (!data.empty()) {
            siftDown(0, data.size());
        }
    }
 
    T top() {
        return data[0];
    }
 
    bool empty() {
        return data.empty();
    }
};
 
// int main() {
//     MinHeap<int> minHeap;
 
//     minHeap.push(10);
//     minHeap.push(3);
//     minHeap.push(7);
//     minHeap.push(1);
 
//     while (!minHeap.empty()) {
//         std::cout << minHeap.top() << ' ';
//         minHeap.pop();
//     }
 
//     return 0;
// }

#endif