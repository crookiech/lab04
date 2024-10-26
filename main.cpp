#include <iostream>
#include <memory>
#include <cstddef>
#include <map>

// Кастомный аллокатор для выделения памяти для объектов типа T
template <class T, size_t poolSize = 10> // определение шаблона класса, T — тип данных, poolSize — размер пула (по умолчанию 10)
class CustomAllocator {
private:
    size_t currentMemory; // текущий размер выделенного блока памяти
    size_t allocatedElements; // количество элементов в выделенной памяти
    T * memory; // указатель на начало выделенного блока памяти
public:
    using value_type = T; // алиас для типа T
    // Конструктор по умолчанию (не выбрасывает исключения)
    // Инициализирует CustomAllocator (memory инициализируется указателем на блок памяти, выделенный с помощью std::malloc)
    CustomAllocator () : currentMemory(poolSize), allocatedElements(0), memory(static_cast<T*>(std::malloc(poolSize * sizeof(T)))) {
        if (!memory) { // если выделение памяти не удалось 
            throw std::bad_alloc();  // выбрасываем исключение std::bad_alloc
        }      
    }
    // Деструктор (освобождает выделенную память)
    ~CustomAllocator() { std::free(memory); }
    // Конструктор копирования (не выбрасывает исключения)
    // Позволяет копировать из другого CustomAllocator другого типа U 
    template <class U> CustomAllocator (const CustomAllocator<U>&) noexcept {}
    // Функция выделения памяти для n объектов типа T
    T* allocate (std::size_t n) {
        return static_cast<T*>(::operator new(n*sizeof(T))); // используется глобальный operator new
    }
    // Функция освобождения памяти, на которую указывает p
    void deallocate (T* p, std::size_t n) { ::operator delete(p); } // используется глобальный operator delete 
    // Структура, которая позволяет переназначить тип аллокатора
    template< class U >
    struct rebind {
        typedef CustomAllocator<U> other; // other — алиас для типа CustomAllocator<U>
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

// Контейнер с фиксированным размером, использующий кастомный аллокатор
template <typename T, size_t maxSize, typename Allocator = CustomAllocator<T>> // T — тип данных, maxSize — максимальный размер контейнера, Allocator — тип аллокатора (по умолчанию CustomAllocator<T>)
class LimitedContainer {
private:
    Allocator alloc; // экземпляр аллокатора
    T* data; // указатель на данные
    size_t size; // текущий размер контейнера
public:
    // Конструктор по умолчанию (инициализирует данные, размер и аллокатор)
    LimitedContainer() : alloc(), data(nullptr), size(0) {}
    // Деструктор
    // Освобождает память, выделенную для данных (вызывается автоматически)
    ~LimitedContainer() {
        if (data) {
            alloc.deallocate(data, size);
        }
    }
    // Добавление элемента в конец контейнера
    void push_back(const T& value) {
        if (size == maxSize) { // если контейнер заполнен
            throw std::runtime_error("Контейнер уже заполнен!"); // выбрасывание исключения
        }
        if (size == 0) { // если контейнер пустой, выделяем память для данных
            data = alloc.allocate(maxSize);
        }
        new (&data[size]) T(value); // использование placement new для создания нового элемента типа T в выделенной памяти
        ++size; // увеличение размера контейнера
    }
    // Оператор доступа по индексу к элементу контейнера
    const T& operator[](size_t index) const {
        if (index >= size) { // находится ли индекс в пределах размера контейнера
            throw std::out_of_range("Неверный индекс!"); // если нет, вырбрасывается исключение
        }
        return data[index]; // иначе, возвращается ссылка на элемент по заданному индексу
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

// Двунаправленный список
template <typename T, typename Allocator = CustomAllocator<T>> // T — тип данных, Allocator — тип аллокатора для выделения памяти (по умолчанию CustomAllocator<T>).
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
    Allocator allocator; // экземпляр аллокатора
public:
    BidirectionalList() : head(nullptr), tail(nullptr), numberOfElements(0), allocator() {}
    BidirectionalList(BidirectionalList&& other) noexcept
        : head(other.head), tail(other.tail), numberOfElements(other.numberOfElements), allocator(std::move(other.allocator)) {
        other.head = nullptr; 
        other.tail = nullptr;
        other.numberOfElements = 0;
    }
    ~BidirectionalList() { 
        Node* current = head;
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
    }
    void push_back(const T& value) {
        typename Allocator::template rebind<Node>::other nodeAlloc; // создание специализированного аллокатора для узлов Node (использует rebind для создания нового аллокатора nodeAlloc, который будет выделять память для объектов типа Node, а не для T)
        Node* newNode = nodeAlloc.allocate(1); // выделение памяти для одного нового элемента типа Node
        new(newNode) Node(value); // использование placement new для создания нового объекта типа Node в выделенной памяти (позволяет избежать лишнего вызова new, который бы выделил новую память, а не использовал уже выделенную)
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
    // создание экземпляра std::map<int, int> со своим аллокатором
    std::map<int, int, std::less<int>, CustomAllocator<std::pair<const int, int>, 10>> allocatorMap;
    // заполнение 10 элементами, где ключ – это число от 0 до 9, а значение – факториал ключа
    for (int i = 0; i < 10; ++i) {
        allocatorMap[i] = factorial(i);
    }
    // вывод на экран всех значений хранящихся в контейнере
    std::cout << "Словарь со своим аллокатором: " << std::endl;
    for (const auto& pair : allocatorMap) {
        std::cout << pair.first << " " << pair.second << std::endl;
    }
    // создание экземпляра своего контейнера для хранения значений типа int
    BidirectionalList<int> list;
    // заполнение 10 элементами от 0 до 9
    for (int i = 0; i <= 9; i++) {
        list.push_back(i);
    }
    // создание экземпляра своего контейнера для хранения значений типа int со своим аллокатором
    BidirectionalList<int, CustomAllocator<int> > allocatorList;
    // заполнение 10 элементами от 0 до 9
    for (int i = 0; i <= 9; i++) {
        allocatorList.push_back(i);
    }
    // вывод на экран всех значений, хранящихся в контейнере
    std::cout << "Свой контейнер со своим аллокатором: ";
    allocatorList.print();
    std::cout << std::endl;
    // создание экземпляра своего контейнера с фиксированным размеом для хранения значений типа int со своим аллокатором
    LimitedContainer<std::pair<const int, int>, 10, CustomAllocator<std::pair<const int, int>, 10>> limitedMap;
    // заполнение 10 элементами, где ключ – это число от 0 до 9, а значение – факториал ключа
    for (int i = 0; i < 10; ++i) {
        limitedMap.push_back(std::make_pair(i, factorial(i)));
    }
    // вывод на экран всех значений, хранящихся в контейнере
    std::cout << "Свой контейнер с фиксированным размеом со своим аллокатором: " << std::endl;
    for (size_t i = 0; i < limitedMap.getSize(); ++i) {
        std::cout << limitedMap[i].first << " " << limitedMap[i].second << std::endl;
    }
    return 0;
}