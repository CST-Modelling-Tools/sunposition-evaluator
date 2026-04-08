#include "mica_binary_dataset_builder.h"

#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[])
{
    try {
        if (argc != 3) {
            std::cerr
                << "Usage: build_mica_binary_dataset <input.csv> <output.bin>\n";
            return 1;
        }

        const std::string input_csv_path = argv[1];
        const std::string output_binary_path = argv[2];

        build_mica_binary_dataset(input_csv_path, output_binary_path);

        std::cout << "Wrote binary dataset to " << output_binary_path << '\n';
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Error: " << exception.what() << '\n';
        return 1;
    }
}
