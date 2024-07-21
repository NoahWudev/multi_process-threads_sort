#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <chrono>
#include<algorithm>

#define min(x, y) (((x)>(y))?(y):(x))

// 讀取CSV文件
std::vector<int> read_csv( const std::string& filename ) {
    std::ifstream file(filename);
    std::vector<int> data;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty()) {
            data.push_back(std::stoi(line));
        }
    }

    return data;
}

// Bubble Sort
void bubble_sort( int* data, size_t size ) {
    for (size_t i = 0; i < size - 1; ++i) {
        for (size_t j = 0; j < size - i - 1; ++j) {
            if (data[j] > data[j + 1]) {
                std::swap(data[j], data[j + 1]);
            }
        }
    }
}

// 合併排序函數
void merge( int* arr, int left, int mid, int right ) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    int* L = new int[n1];
    int* R = new int[n2];

    for (int i = 0; i < n1; i++)
        L[i] = arr[left + i];
    for (int j = 0; j < n2; j++)
        R[j] = arr[mid + 1 + j];

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }

    delete[] L;
    delete[] R;
}
void act_sort( std::vector<int>& data, int* data_ptr ) {
    auto start = std::chrono::high_resolution_clock::now(); // 開始計時
    
    bubble_sort(data_ptr, data.size());

    // 結束計時
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "排序完成，耗時: " << diff.count() << " 秒" << std::endl;
}

// 將排序好的數據寫回CSV
void write_csv( int* shm_ptr, std::string filename, std::vector<int> data ) {
    std::ofstream outfile( "pure_sorted_" + filename );
    for (size_t i = 0; i < data.size(); ++i) {
        outfile << shm_ptr[i] << "\n";
    }
}


int main() {
    std::string filename;
    std::cout << "請輸入要處理的CSV文件名稱:";
    std::cin >> filename;

    // 讀取CSV文件
    std::vector<int> data = read_csv( filename );
    int* data_ptr = new int[data.size()];
    std::copy( data.begin(), data.end(), data_ptr ); 

    // 從共享記憶體中讀取數據並寫入文件
    act_sort( data, data_ptr );
    write_csv( data_ptr, filename, data );
    
    std::cout << "排序完成，結果已寫入 process_sorted_" << filename << std::endl;

    return 0;
}
