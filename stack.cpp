#include <cstring>
#include <iostream>

struct Node {
  const char* string;
  Node* previous;
};

const char* readString() {
  const size_t kMinStrSize = 8;
  char symbol;
  size_t str_size = kMinStrSize;
  char* string = new char[str_size];

  std::cin.get();
  symbol = std::cin.get();

  size_t ind = 0;

  while (!std::isspace(symbol) && symbol != EOF) {
    if (ind + 1 >= str_size) {
      char* new_string = new char[str_size * 2];
      memcpy(new_string, string, str_size);
      str_size *= 2;

      delete[] string;

      string = new_string;
    }

    string[ind] = symbol;
    ++ind;

    std::cin.get(symbol);
  }
  string[ind] = 0;
  return string;
}

void push(Node*& top, size_t& size) {
  top = new Node{readString(), top};
  ++size;
  std::cout << "ok" << std::endl;
}

void back(Node* top) {
  std::cout << (top == nullptr ? "error" : top->string) << std::endl;
}

void pop(Node*& top, size_t& size) {
  if (top == nullptr) {
    std::cout << "error" << std::endl;
    return;
  }
  std::cout << top->string << std::endl;
  Node* previous = top->previous;
  delete[] top->string;
  delete top;
  top = previous;
  --size;
}

void clear(Node*& top, size_t& size) {
  for (size_t i = 0; i < size; ++i) {
    Node* previous = top->previous;
    delete[] top->string;
    delete top;
    top = previous;
  }
  size = 0;
  std::cout << "ok" << std::endl;
}

int main() {
  const size_t kCommandSize = 6;
  char command[kCommandSize];
  Node* top = nullptr;
  size_t size = 0;

  while (true) {
    std::cin >> command;
    if (strcmp(command, "exit") == 0) {
      std::cout << "bye" << std::endl;
      break;
    }
    if (strcmp(command, "push") == 0) {
      push(top, size);
    } else if (strcmp(command, "size") == 0) {
      std::cout << size << std::endl;
    } else if (strcmp(command, "clear") == 0) {
      clear(top, size);
    } else if (strcmp(command, "pop") == 0) {
      pop(top, size);
    } else if (strcmp(command, "back") == 0) {
      back(top);
    }
  }

  for (size_t i = 0; i < size; ++i) {
    Node* previous = top->previous;
    delete[] top->string;
    delete top;
    top = previous;
  }
}