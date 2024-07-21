#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <sys/mman.h>
#include <fcntl.h>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>
#include <chrono>
#include<algorithm>

#define min(x, y) (((x)>(y))?(y):(x))

std::mutex mtx;

// 讀取CSV文件
std::vector<int> read_csv( const std::string& filename ) {
    std::ifstream file( filename );
    std::vector<int> data;
    std::string line;

    while (std::getline( file, line )){
        if (!line.empty()){
            data.push_back( std::stoi( line ) );
        }
    }

    return data;
}

// Bubble Sort
void bubble_sort( int* data, size_t size ) {
    for (size_t i = 0; i < size - 1; ++i) {
        for (size_t j = 0; j < size - i - 1; ++j) {
            if (data[j] > data[j + 1]) {
                std::swap( data[j], data[j + 1] );
            }
        }
    }
}

// 合併排序函數
void merge( int* arr, int left, int mid, int right ) {
    //mtx.lock();
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

int* create_shm_ptr( std::vector<int> data ) {
    size_t size = data.size() * sizeof(int);
    int shm_fd = shm_open("/my_shared_memory", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(-1);
    }
    if (ftruncate(shm_fd, size) == -1) {
        perror("ftruncate failed");
        shm_unlink("/my_shared_memory");
        exit(-1);
    }
    int* shm_ptr = (int*)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap failed");
        shm_unlink("/my_shared_memory");
        exit(-1);
    }

    return shm_ptr;
}

void t_act_sort( std::vector<int> data, int* data_ptr, int K ) {
    auto start = std::chrono::high_resolution_clock::now(); // 開始計時
    // 切割成 K 份
    size_t chunk_size= data.size() / K;
    std::vector<std::pair<int*, size_t>> chunks;
    for (int i = 0; i < K; ++i) {
        int* start_ptr = data_ptr + i * chunk_size;
        size_t current_chunk_size = (i == K - 1) ? (data.size() - i * chunk_size) : chunk_size;
        chunks.push_back(std::make_pair(start_ptr, current_chunk_size));
    }

    // 創建K個子線程進行冒泡排序
    std::vector<std::thread> threads;
    for (int i = 0; i < K; ++i) {
        threads.emplace_back(bubble_sort, chunks[i].first, chunks[i].second );
    }
    for (auto& thread : threads) {
        thread.join();
    }
    threads.clear();
    std::vector<std::thread>().swap( threads );

    // 合併排序子線程
    std::vector<std::thread> merge_threads;
    int num_merges = K;
    int step = 1;
    while (num_merges > 1) {
        for (int i = 0; i < num_merges / 2; ++i) {
            // 子線程進行合併排序
            int left = i * 2 * step * chunk_size;
            int mid = left + step * chunk_size - 1;
            int right = min( left + 2 * step * chunk_size - 1, (int)data.size() - 1 );
            merge_threads.emplace_back(merge, data_ptr, left, mid, right);
        }
        // 等待所有合併排序的子線程完成
        for (auto& thread : merge_threads) {
            thread.join();
        }
        merge_threads.clear();
        std::vector<std::thread>().swap( merge_threads );
        step *= 2;
        num_merges = ( num_merges + 1 ) / 2;
    }

    // 結束計時
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "排序完成，耗時: " << diff.count() << " 秒" << std::endl;
}

void p_act_sort( std::vector<int>& data, int K, int* shm_ptr ) {
auto start = std::chrono::high_resolution_clock::now(); // 開始計時
    // 切割成 K 份
    size_t chunk_size = data.size() / K;
    std::vector<std::pair<int*, size_t>> chunks;
    for (int i = 0; i < K; ++i) {
        int* start_ptr = shm_ptr + i * chunk_size;
        size_t current_chunk_size = (i == K - 1) ? (data.size() - i * chunk_size) : chunk_size;
        chunks.push_back(std::make_pair(start_ptr, current_chunk_size));
    }

    // 創建子進程並進行泡沫排序
    for (int i = 0; i < K; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            // 子進程
            bubble_sort(chunks[i].first, chunks[i].second);
            exit(0);
        }
    }

    // 等待所有子進程完成
    for (int i = 0; i < K; ++i) {
        wait(NULL);
    }

    // 合併排序子進程
    int num_merges = K;
    int step = 1;
    while (num_merges > 1) {
        for (int i = 0; i < num_merges / 2; ++i) {
            pid_t pid = fork();
            if (pid == 0) {
                // 子進程進行合併排序
                int left = i * 2 * step * chunk_size;
                int mid = left + step * chunk_size - 1;
                int right = min(left + 2 * step * chunk_size - 1, (int)data.size() - 1);
                merge(shm_ptr, left, mid, right);
                exit(0);
            }
        }
        // 等待所有合併排序的子進程完成
        for (int i = 0; i < num_merges / 2; ++i) {
            wait(NULL);
        }
        step *= 2;
        num_merges = (num_merges + 1) / 2;
    }

    // 結束計時
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "排序完成，耗時: " << diff.count() << " 秒" << std::endl;
}

void de_link_shm( int* shm_ptr, std::vector<int> data ) {
    size_t size = data.size() * sizeof(int);
    munmap(shm_ptr, size);
    shm_unlink("/my_shared_memory");
}

// 將排序好的數據寫回CSV
void write_csv( int* shm_ptr, std::string filename, std::vector<int> data, int x ) {
    if (x==1) filename = "thread_sorted_" + filename;
    else if (x==2) filename = "process_sorted_" + filename;

    std::ofstream outfile( filename );
    for (size_t i = 0; i < data.size(); ++i) {
        outfile << shm_ptr[i] << "\n";
    }

    
    if (x==1) std::cout << "排序完成，結果已寫入 " << filename << std::endl << std::endl;
    else if (x==2) std::cout << "排序完成，結果已寫入 " << filename << std::endl << std::endl;
    
}

int main() {
    int K, mission;
    std::string filename;

    while (1) {
        std::cout << "請輸入要執行的方案: multi-threads - 1, multi-processes - 2, exit - 0" << std::endl;
        std::cin >> mission;


        switch (mission){
        case 0:
            std::cout << "Bye!" << std::endl;
            exit(-1);
            break;
        
        case 1:{
            std::cout << "請輸入數據分割的份數K:";
            std::cin >> K;
            std::cout << "請輸入要處理的CSV文件名稱:";
            std::cin >> filename;
            // 讀取CSV文件
            std::vector<int> data = read_csv( filename );
            int* data_ptr = new int[data.size()];
            std::copy( data.begin(), data.end(), data_ptr ); 

            // 執行排序
            t_act_sort( data, data_ptr, K );

            // 輸出排序後的CSV文件
            write_csv( data_ptr, filename, data, 1 );

            data.clear();
            std::vector<int>().swap(data);
            break;
        }

        case 2:{
            std::cout << "請輸入數據分割的份數K:";
            std::cin >> K;
            std::cout << "請輸入要處理的CSV文件名稱:";
            std::cin >> filename;

            // 讀取CSV文件
            std::vector<int> data = read_csv( filename );

            // 創建共享記憶體
            int* shm_ptr = create_shm_ptr( data );

            // 將數據寫入共享記憶體
            std::copy( data.begin(), data.end(), shm_ptr );

            // 從共享記憶體中讀取數據並寫入文件
            p_act_sort( data, K, shm_ptr );
            write_csv( shm_ptr, filename, data, 2 );

            // 釋放共享記憶體
            de_link_shm( shm_ptr, data );
            break;
        }

        default:
            perror("Bad request! Bye! \n");
            exit(-1);
            break;
        }
        

    }
    return 0;
}
