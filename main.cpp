#include <iostream>
#include <memory>
#include <cstddef>
#include <map>

template <class T, size_t poolSize = 10>
struct CustomAllocator {
private:
    size_t currentMemory; //текущий размер блока
    size_t allocatedElements; //количество выделенных элементов
    T * memory; //указатель на начало блока
public:
    using value_type = T;
    void* pool;
    // Конструктор по умолчанию инициализирует CustomAllocator (не выбрасывает исключения)
    CustomAllocator () : currentMemory(poolSize), allocatedElements(0), memory(static_cast<T*>(std::malloc(poolSize * sizeof(T)))) {
        if (!memory) {
            throw std::bad_alloc();  // Если выделение памяти не удалось, выбрасываем исключение std::bad_alloc
        }      
    }
    // Деструктор освобождает всю ранее выделенную память при уничтожении объекта CustomAllocator
    ~CustomAllocator() { std::free(memory); }
    // Конструктор копирования, который позволяет копировать из другого CustomAllocator другого типа U (не выбрасывает исключения)
    template <class U> CustomAllocator (const CustomAllocator<U>&) noexcept {}
    // Функция выделения памяти для n объектов типа T
    T* allocate (std::size_t n) {
        return static_cast<T*>(::operator new(n*sizeof(T))); // используется глобальный operator new
    }
    // Функция освобождения памяти, на которую указывает p
    void deallocate (T* p, std::size_t n) { ::operator delete(p); } // используется глобальный operator delete 
    // Структура, которая позволяет повторно связывать аллокатор с другим типом U
    template< class U >
    struct rebind {
        typedef CustomAllocator<U> other;
    };
};

// Оператор == для сравнения аллокаторов (всегда возвращает true)
template <class T, class U>
constexpr bool operator== (const CustomAllocator<T>& a1, const CustomAllocator<U>& a2) noexcept {
    return true;
}
// Оператор != для сравнения аллокаторов (всегда возвращает false)
template <class T, class U>
constexpr bool operator!= (const CustomAllocator<T>& a1, const CustomAllocator<U>& a2) noexcept {
    return false;
}

// Класс ограниченного контейнера (с фиксированным размером)
template <typename T, size_t maxSize, typename Allocator = CustomAllocator<T>>
class LimitedContainer {
private:
    Allocator alloc; // экземпляр аллокатора
    T* data; // указатель на данные
    size_t size; // текущий размер контейнера
public:
    // Конструктор по умолчанию (инициализирует данные, размер и аллокатор)
    LimitedContainer() : alloc(), data(nullptr), size(0) {}
    // Деструктор
    ~LimitedContainer() {
        if (data) {
            alloc.deallocate(data, size);
        }
    }
    // Добавление элемента в конец контейнера, если контейнер заполнен, выбрасывается исключение
    void push_back(const T& value) {
        if (size == maxSize) { // проверка на заполненность контейнера
            throw std::runtime_error("Контейнер уже заполнен!"); 
        }
        if (size == 0) { // если контейнер пустой, выделяем память для данных
            data = alloc.allocate(maxSize); 
        }
        new (&data[size]) T(value); // использование placement new для создания нового элемента типа T
        ++size; // увеличение размера контейнера
    }
    // Возвращение ссылки на элемент по заданному индексу
    const T& operator[](size_t index) const {
        if (index >= size) {
            throw std::out_of_range("Неверный индекс!");
        }
        return data[index];
    }
    // Возвращение текущего размера контейнера
    size_t getSize() const {
        return size;
    }
    // Проверка, пуст ли контейнер
    bool empty() const {
        return size == 0;
    }
};

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
    size_t numberOfElements;
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
    ~BidirectionalList() { clear(); }
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
    size_t get_size() const {
        return numberOfElements;
    }
    // Очистка контейнера
    void clear() {
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
    std::map<int, int, std::less<int>, CustomAllocator<std::pair<const int, int>, 10>> allocatorMap;
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

    LimitedContainer<std::pair<const int, int>, 10, CustomAllocator<std::pair<const int, int>, 10>> limited_map;
    for (int i = 0; i < 10; ++i) {
        limited_map.push_back(std::make_pair(i, factorial(i)));
    }
    std::cout << "LimitedContainer: ";
    for (size_t i = 0; i < limited_map.getSize(); ++i) {
        std::cout << limited_map[i].first << " " << limited_map[i].second << std::endl;
    }
    return 0;
}