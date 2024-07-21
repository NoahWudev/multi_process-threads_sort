#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <string>


std::vector<int> generate_random_numbers(int a, int range_min, int range_max) {
    std::vector<int> numbers;
    std::random_device rd; 
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> dis(range_min, range_max);

    for (int i = 0; i < a; ++i) {
        numbers.push_back(dis(gen));
    }

    return numbers;
}


void write_to_csv(const std::vector<int>& numbers, const std::string& filename) {
    std::ofstream file(filename);

    if (file.is_open()) {
        for (const auto& num : numbers) {
            file << num << "\n";
        }
        file.close();
    } else {
        std::cerr << "無法開啟檔案" << filename << std::endl;
    }
}

int main() {
    int num;
    std::cout << "請輸入欲生成筆數:";
    std::cin >> num;

    if (num <= 0) {
        std::cerr << "數字必須大於1" << std::endl;
        return 1;
    }

 
    std::vector<int> numbers = generate_random_numbers(num, 0, 50000);


    std::string filename = "test_data_" + std::to_string(num) + ".csv";

    write_to_csv(numbers, filename);

    std::cout << "作業完成：" << filename << std::endl;

    return 0;
}
