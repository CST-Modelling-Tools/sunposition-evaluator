#ifndef MICA_BINARY_DATASET_BUILDER_H
#define MICA_BINARY_DATASET_BUILDER_H

#include <filesystem>

void build_mica_binary_dataset(
    const std::filesystem::path& input_csv_path,
    const std::filesystem::path& output_binary_path);

#endif // MICA_BINARY_DATASET_BUILDER_H
