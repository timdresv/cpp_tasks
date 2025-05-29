#include <iostream>

long long sum(int** arrays, size_t size, size_t* indexes, size_t cur_index,
              const size_t* sizes) {
  if (size == cur_index) {
    return 1;
  }

  long long result = 0;
  bool is_was;
  for (size_t i = 0; i < sizes[cur_index]; ++i) {
    is_was = false;
    for (size_t j = 0; j < cur_index; ++j) {
      if (indexes[j] == i) {
        is_was = true;
        break;
      }
    }

    if (!is_was) {
      indexes[cur_index] = i;
      result += arrays[cur_index][i] *
          sum(arrays, size, indexes, cur_index + 1, sizes);
    }
  }
  return result;
}

int main(int argc, char* argv[]) {
  size_t cnt_of_arrays = argc - 1;
  int** arrays = new int*[cnt_of_arrays];

  size_t* sizes = new size_t[cnt_of_arrays];
  for (size_t i = 0; i < cnt_of_arrays; ++i) {
    sizes[i] = atoi(argv[i + 1]);
  }

  for (size_t i = 0; i < cnt_of_arrays; ++i) {
    arrays[i] = new int[sizes[i]];
    for (size_t j = 0; j < sizes[i]; ++j) {
      std::cin >> arrays[i][j];
    }
  }

  size_t* indexes = new size_t[cnt_of_arrays];
  std::cout << sum(arrays, cnt_of_arrays, indexes, 0, sizes);

  for (size_t i = 0; i < cnt_of_arrays; ++i) {
    delete[] arrays[i];
  }
  delete[] arrays;
  delete[] sizes;
  delete[] indexes;
}