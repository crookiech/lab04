#include <iostream>
#include <memory>
#include <cstddef>
#include <map>

template <class T>
struct CustomAllocator {
    using value_type = T;
    void* pool;
    // Конструктор по умолчанию инициализирует CustomAllocator (не выбрасывает исключения)
    CustomAllocator () noexcept {
        pool = ::operator new(10*sizeof(T));
    } 
    // Деструктор освобождает всю ранее выделенную память при уничтожении объекта CustomAllocator
    ~CustomAllocator() { ::operator delete(pool); }
    // Конструктор копирования, который позволяет копировать из другого CustomAllocator другого типа U (не выбрасывает исключения)
    template <class U> CustomAllocator (const CustomAllocator<U>&) noexcept {}
    // Функция выделения памяти для n объектов типа T
    T* allocate (std::size_t n) {
        return static_cast<T*>(::operator new((2*n*sizeof(T) + 1))); // используется глобальный operator new
    }
    // Функция освобождения памяти, на которую указывает p
    void deallocate (T* p, std::size_t n) { 
        ::operator delete(p); // используется глобальный operator delete
    } 
    // Структура, которая позволяет повторно связывать аллокатор с другим типом U
    template< class U >
    struct rebind {
        typedef CustomAllocator<U> other;
    };
};

template <class T, class U>
constexpr bool operator== (const CustomAllocator<T>& a1, const CustomAllocator<U>& a2) noexcept {
    return true;
}
template <class T, class U>
constexpr bool operator!= (const CustomAllocator<T>& a1, const CustomAllocator<U>& a2) noexcept {
    return false;
}

template <typename T, typename Allocator = CustomAllocator<T>>
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
    Allocator allocator;
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
    BidirectionalList() : head(nullptr), tail(nullptr), numberOfElements(0), allocator() {}
    BidirectionalList(BidirectionalList&& other) noexcept
        : head(other.head), tail(other.tail), numberOfElements(other.numberOfElements), allocator(std::move(other.allocator)) {
        other.head = nullptr; 
        other.tail = nullptr;
        other.numberOfElements = 0;
    }
    void push_back(const T& value) {
        typename Allocator::template rebind<Node>::other nodeAlloc;
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
                std::cout << " ";
            }
            current = current->next;
        }
    }
    bool empty() {
        if ((head == nullptr) && (tail == nullptr)) {
            return true;
        }
        return false;
    }
    int get_size() const {
        return numberOfElements;
    }
    ~BidirectionalList() {
        Node* current = head;
        while (current != nullptr) {
            Node* next = current->next;
            // Используем аллокатор для освобождения памяти
            typename Allocator::template rebind<Node>::other nodeAlloc;
            nodeAlloc.deallocate(current, 1);
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
    std::map<int, int, std::less<int>, CustomAllocator<std::pair<const int, int>>> allocatorMap;
    // заполнение 10 элементами, где ключ – это число от 0 до 9, а значение – факториал ключа
    for (int i = 0; i < 10; ++i) {
        allocatorMap[i] = factorial(i);
    }
    // вывод на экран всех значений хранящихся в контейнере
    for (const auto& pair : allocatorMap) {
        std::cout << pair.first << " " << pair.second << std::endl;
    }
    // создание экземпляра своего контейнера для хранения значений типа int
    BidirectionalList<int> list;
    // заполнение 10 элементами от 0 до 9
    for (int i = 0; i <= 9; i++) {
        list.push_back(i);
    }
    // создание экземпляра своего контейнера для хранения значений типа int с новым аллокатором, ограниченным 10 элементами
    BidirectionalList<int, CustomAllocator<int> > allocatorList;
    // заполнение 10 элементами от 0 до 9
    for (int i = 0; i <= 9; i++) {
        allocatorList.push_back(i);
    }
    // вывод на экран всех значений, хранящихся в контейнере
    allocatorList.print();
    return 0;
}