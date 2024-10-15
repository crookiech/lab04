#include <iostream>
#include <map>
#include <memory>
#include <cstddef>

template <class T>
struct pool_allocator {
    using value_type = T;
    void* pool;
    pool_allocator () noexcept {}
    template <class U> pool_allocator (const pool_allocator<U>&) noexcept {}
    T* allocate (std::size_t n) {
        return static_cast<T*>(::operator new(n*sizeof(T)));
    }
    void deallocate (T* p, std::size_t n) { ::operator delete(p); }
    template< class U >
    struct rebind {
        typedef pool_allocator<U> other;
    };
};

template <class T, class U>
constexpr bool operator== (const pool_allocator<T>& a1, const pool_allocator<U>& a2) noexcept {
    return true;
}
template <class T, class U>
constexpr bool operator!= (const pool_allocator<T>& a1, const pool_allocator<U>& a2) noexcept {
    return false;
}

template <typename T, typename pool_allocator>
class BidirectionalList {
private:
    struct Node {
        T value;
        Node* next;
        Node* previous;
        Node(const T& value) : value(value), next(nullptr), previous(nullptr) {}
    };
    Node* head;
    Node* tail;
    int numberOfElements;
    pool_allocator allocator;
public:
    struct Iterator {
        Node* current;
        Iterator(Node* node) : current(node) {}
        T& operator*() {
            if (current == nullptr){
                std::cout << "Неверный индекс!" << std::endl;
            }
            return current->value;
        }
        T& get() {
            if (current == nullptr) {
                std::cout << "Неверный индекс!" << std::endl;
            }
            return current->value;
        }
        Iterator& operator++() {
            if (current != nullptr) current = current->next;
            return *this;
        }
        Iterator& operator--() {
            if (current != nullptr && current->previous != nullptr)
                current = current->previous;
            return *this;
        }
        bool operator!=(const Iterator& other) const {
            return current != other.current;
        }
    };
    Iterator begin() { 
        return Iterator(head); 
    }
    Iterator end() { 
        return Iterator(nullptr); 
    }
    BidirectionalList() : head(nullptr), tail(nullptr), numberOfElements(0) {}
    BidirectionalList(BidirectionalList&& other) noexcept
        :  head(other.head), tail(other.tail), numberOfElements(other.numberOfElements) {
        other.head = nullptr; 
        other.tail = nullptr;
        other.numberOfElements = 0;
    }
    void push_back(const T& value) {
        typename pool_allocator::template rebind<Node>::other nodeAlloc;
        Node* newNode = nodeAlloc.allocate(1);
        new(newNode) Node(value); // Используем placement new для инициализации узла
        if (head == nullptr) {
            head = newNode;
            tail = newNode;
        } else {
            tail->next = newNode;
            newNode->previous = tail;
            tail = newNode;
        }
        ++numberOfElements;
    }
    void print() const {
        Node* current = head;
        while (current != nullptr) {
            std::cout << current->value;
            if (current->next != nullptr) {
                std::cout << ", ";
            }
            current = current->next;
        }
    }
    T& operator[](int index) {
        if (index < 0 || index >= numberOfElements) {
            std::cout << "Неверный индекс!" << std::endl;
        }
        Node* current = head;
        for (int i = 0; i < index; ++i) {
            current = current->next;
        }
        return current->value;
    }
    int get_size() const {
        return numberOfElements;
    }
    ~BidirectionalList() {
        Node* current = head;
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
    }
};

// Функция для вычисления факториала
int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);
}

int main() {
    // создание экземпляра std::map<int, int>
    std::map<int, int> standardMap;
    // заполнение 10 элементами, где ключ – это число от 0 до 9, а значение – факториал ключа
    for (int i = 0; i < 10; ++i) {
        standardMap[i] = factorial(i);
    }
    // создание экземпляра std::map<int, int> с новым аллокатором, ограниченным 10 элементами
    std::map<int, int, std::less<int>, pool_allocator<std::pair<const int, int>>> allocatorMap;
    // заполнение 10 элементами, где ключ – это число от 0 до 9, а значение – факториал ключа
    for (int i = 0; i < 10; ++i) {
        allocatorMap[i] = factorial(i);
    }
    // вывод на экран всех значений хранящихся в контейнере
    for (const auto& pair : allocatorMap) {
        std::cout << pair.first << " " << pair.second << std::endl;
    }
    // // создание экземпляра своего контейнера для хранения значений типа int
    // BidirectionalList<int> list;
    // // заполнение 10 элементами от 0 до 9
    // for (int i = 0; i <= 9; i++) {
    //     list.push_back(i);
    // }
    // создание экземпляра своего контейнера для хранения значений типа int с новым аллокатором, ограниченным 10 элементами
    BidirectionalList<int, pool_allocator<int> > allocatorList;
    // заполнение 10 элементами от 0 до 9
    for (int i = 0; i <= 9; i++) {
        allocatorList.push_back(i);
    }
    // вывод на экран всех значений, хранящихся в контейнере
    for (auto it = allocatorList.begin(); it != allocatorList.end(); ++it) {
        std::cout << *it << " ";
    }
    return 0;
}
