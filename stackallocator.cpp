//
// Created by serge on 25.04.2022.
//

#ifndef LIST_LIST_H
#define LIST_LIST_H

#include <cstddef>
#include <memory>

//STACKSTORAGE//

template <size_t N>
class StackStorage {
public:
    StackStorage(const StackStorage&) = delete;
    StackStorage& operator=(const StackStorage&) = delete;

    StackStorage() {
        ptr_ = memory_;
    }

    int8_t* allocate(size_t num) {
        int8_t* ret_ptr = ptr_;
        ptr_ += num + (alignof(std::max_align_t) - num) % alignof(std::max_align_t);
        return ret_ptr;
    }

    void deallocate(int8_t*, size_t) {}

private:
    int8_t memory_[N];
    int8_t* ptr_;
};


template <typename T, size_t N>
class StackAllocator {
public:
    using value_type = T;

    template <typename U>
    struct rebind {
        typedef StackAllocator<U, N> other;
    };

    StackAllocator(StackStorage<N>& storage) : storage_(&storage) {}

    ~StackAllocator() = default;

    StackAllocator(const StackAllocator<T, N>& other) : storage_(other.storage_) {}

    template <typename U>
    StackAllocator(const StackAllocator<U, N>& other) : storage_(other.storage_) {}

    StackAllocator& operator=(const StackAllocator<T, N>& other) {
        storage_ = other.storage_;
        return *this;
    }

    template <typename U>
    StackAllocator& operator=(const StackAllocator<U, N>& other) {
        storage_ = other.storage_;
        return *this;
    }

    T* allocate(size_t num) {
        return reinterpret_cast<T*>(storage_->allocate(sizeof(T) * num));
    }

    void deallocate(T* ptr, size_t num) {
        storage_->deallocate(reinterpret_cast<int8_t*>(ptr), sizeof(T) * num);
    }

    template <typename U>
    bool operator==(const StackAllocator<U, N>& other) const {
        return storage_ == other.storage_;
    }

    template <typename U>
    bool operator!=(const StackAllocator<U, N>& other) const {
        return storage_ != other.storage_;
    }

    template <typename U, size_t M>
    friend class StackAllocator;

private:
    StackStorage<N>* storage_;
};

template <typename T, typename Alloc = std::allocator<T>>
class List {
private:
    struct BaseNode {
        BaseNode* prev;
        BaseNode* next;
    };
    struct Node : BaseNode {
        Node(const T& value) : value(value) {}
        Node() {}
        T value;
    };

    template <bool isConst>
    class CommonIterator;

    using AllocTraits = std::allocator_traits<Alloc>;
    using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
    using NodeAllocTraits = std::allocator_traits<NodeAlloc>;

    size_t size_ = 0;
    NodeAlloc alloc_;
    BaseNode fakeNode_;
    CommonIterator<false> fakeIter_{&fakeNode_};

public:
    using iterator = CommonIterator<false>;
    using const_iterator = CommonIterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    List();
    List(size_t size);
    List(size_t size, const T& value);
    List(const Alloc& alloc);
    List(size_t size, const Alloc& alloc);
    List(size_t size, const T& value, const Alloc& alloc);
    List(const List<T, Alloc>& other);

    ~List();

    template <typename OtherAlloc>
    List& operator=(const List<T, OtherAlloc>& other);

    List& operator=(const List<T, Alloc>& other);

    Alloc get_allocator() const;

    size_t size() const;

    void push_back(const T& value);
    void push_front(const T& value);
    void pop_back();
    void pop_front();

    template <bool isConst>
    void insert(CommonIterator<isConst> it, const T& value);

    template <bool isConst>
    void erase(CommonIterator<isConst> it);

    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;
    const_iterator cbegin() const;
    const_iterator cend() const;
    reverse_iterator rbegin();
    const_reverse_iterator rbegin() const;
    reverse_iterator rend();
    const_reverse_iterator rend() const;
    const_reverse_iterator crbegin() const;
    const_reverse_iterator crend() const;
};

template <typename T, typename Alloc>
List<T, Alloc>::List() : size_(0) {
    fakeNode_.next = &fakeNode_;
    fakeNode_.prev = &fakeNode_;
}

template <typename T, typename Alloc>
List<T, Alloc>::List(size_t size) {
    BaseNode* prev_node = &fakeNode_;
    try {
        while (size_ < size) {
            Node* new_node = NodeAllocTraits::allocate(alloc_, 1);
            NodeAllocTraits::construct(alloc_, new_node);
            prev_node->next = new_node;
            new_node->prev = prev_node;
            prev_node = new_node;
            new_node->next = &fakeNode_;
            fakeNode_.prev = new_node;
            ++size_;
        }
    } catch(...) {
        while (size_ > 0) {
            pop_front();
        }
        throw;
    }
}

template <typename T, typename Alloc>
List<T, Alloc>::List(size_t size, const T& value) {
    fakeNode_.next = &fakeNode_;
    fakeNode_.prev = &fakeNode_;
    try {
        while (size_ < size) {
            push_front(value);
        }
    } catch(...) {
        while (size_ > 0) {
            pop_front();
        }
        throw;
    }
}

template <typename T, typename Alloc>
List<T, Alloc>::List(const Alloc& alloc) : size_(0), alloc_(alloc) {
    fakeNode_.next = &fakeNode_;
    fakeNode_.prev = &fakeNode_;
}

template <typename T, typename Alloc>
List<T, Alloc>::List(size_t size, const Alloc& alloc) : size_(0), alloc_(alloc) {
    BaseNode* prev_node = &fakeNode_;
    try {
        while (size_ < size) {
            Node* new_node = NodeAllocTraits::allocate(alloc_, 1);
            NodeAllocTraits::construct(alloc_, new_node);
            prev_node->next = new_node;
            new_node->prev = prev_node;
            prev_node = new_node;
            new_node->next = &fakeNode_;
            fakeNode_.prev = new_node;
            ++size_;
        }
    } catch(...) {
        while (size_ > 0) {
            pop_front();
        }
        throw;
    }
}

template <typename T, typename Alloc>
List<T, Alloc>::List(size_t size, const T& value, const Alloc& alloc) : size_(0), alloc_(alloc) {
    fakeNode_.next = &fakeNode_;
    fakeNode_.prev = &fakeNode_;
    try {
        while (size_ < size) {
            push_front(value);
        }
    } catch(...) {
        while (size_ > 0) {
            pop_front();
        }
        throw;
    }
}

template <typename T, typename Alloc>
List<T, Alloc>::List(const List<T, Alloc>& other) : size_(0),
                                                    alloc_(NodeAllocTraits::select_on_container_copy_construction(other.alloc_)) {
    fakeNode_.next = &fakeNode_;
    fakeNode_.prev = &fakeNode_;
    try {
        auto it = other.begin();
        while (size_ < other.size_) {
            push_back(*it);
            ++it;
        }
    } catch(...) {
        while (size_ > 0) {
            pop_front();
        }
        throw;
    }
}

template <typename T, typename Alloc>
template <typename OtherAlloc>
List<T, Alloc>& List<T, Alloc>::operator=(const List<T, OtherAlloc>& other) {
    NodeAlloc prev_alloc = alloc_;
    size_t prev_size = size_;
    if (alloc_ != other.alloc_ && NodeAllocTraits::propagate_on_container_copy_assignment::value) {
        alloc_ = other.alloc_;
    }
    try {
        auto it = other.begin();
        while (size_ < other.size_ + prev_size) {
            push_back(*it);
            ++it;
        }
    } catch(...) {
        while (size_ > prev_size) {
            pop_back();
        }
        alloc_ = prev_alloc;
        throw;
    }
    while (size_ > other.size_) {
        pop_front();
    }

    return *this;
}

template <typename T, typename Alloc>
List<T, Alloc>& List<T, Alloc>::operator=(const List<T, Alloc>& other) {
    NodeAlloc prev_alloc = alloc_;
    size_t prev_size = size_;
    if (alloc_ != other.alloc_ && NodeAllocTraits::propagate_on_container_copy_assignment::value) {
        alloc_ = other.alloc_;
    }
    try {
        auto it = other.begin();
        while (size_ < other.size_ + prev_size) {
            push_back(*it);
            ++it;
        }
    } catch(...) {
        while (size_ > prev_size) {
            pop_back();
        }
        alloc_ = prev_alloc;
        throw;
    }
    while (size_ > other.size_) {
        pop_front();
    }

    return *this;
}

template <typename T, typename Alloc>
List<T, Alloc>::~List() {
    while (size_ > 0) {
        pop_front();
    }
}

template <typename T, typename Alloc>
Alloc List<T, Alloc>::get_allocator() const {
    return alloc_;
}

template <typename T, typename Alloc>
size_t List<T, Alloc>::size() const {
    return size_;
}

template <typename T, typename Alloc>
void List<T, Alloc>::push_front(const T& value) {
    insert(begin(), value);
}

template <typename T, typename Alloc>
void List<T, Alloc>::push_back(const T& value) {
    insert(end(), value);
}

template <typename T, typename Alloc>
void List<T, Alloc>::pop_front() {
    erase(begin());
}

template <typename T, typename Alloc>
void List<T, Alloc>::pop_back() {
    auto it = end();
    --it;
    erase(it);
}

template <typename T, typename Alloc>
template <bool isConst>
void List<T, Alloc>::insert(List::CommonIterator<isConst> it, const T& value) {
    Node* new_node = NodeAllocTraits::allocate(alloc_, 1);
    try {
        NodeAllocTraits::construct(alloc_, new_node, value);
    } catch(...) {
        NodeAllocTraits::deallocate(alloc_, new_node, 1);
        throw;
    }

    BaseNode* next_node = it.node_;
    BaseNode* prev_node = next_node->prev;
    prev_node->next = new_node;
    next_node->prev = new_node;
    new_node->prev = prev_node;
    new_node->next = next_node;
    ++size_;
}

template <typename T, typename Alloc>
template <bool isConst>
void List<T, Alloc>::erase(List::CommonIterator<isConst> it) {
    BaseNode* cur_node = it.node_;
    BaseNode* prev_node = cur_node->prev;
    BaseNode* next_node = cur_node->next;
    prev_node->next = next_node;
    next_node->prev = prev_node;
    NodeAllocTraits::destroy(alloc_, static_cast<Node*>(cur_node));
    NodeAllocTraits::deallocate(alloc_, static_cast<Node*>(cur_node), 1);
    --size_;
}

template <typename T, typename Alloc>
typename List<T, Alloc>::iterator List<T, Alloc>::begin() {
    iterator return_iter = fakeIter_;
    ++return_iter;
    return return_iter;
}

template <typename T, typename Alloc>
typename List<T, Alloc>::const_iterator List<T, Alloc>::begin() const {
    const_iterator return_iter = fakeIter_;
    ++return_iter;
    return return_iter;
}

template <typename T, typename Alloc>
typename List<T, Alloc>::iterator List<T, Alloc>::end() {
    return fakeIter_;
}

template <typename T, typename Alloc>
typename List<T, Alloc>::const_iterator List<T, Alloc>::end() const {
    return const_iterator(fakeIter_);
}

template <typename T, typename Alloc>
typename List<T, Alloc>::const_iterator List<T, Alloc>::cbegin() const {
    const_iterator return_iter = fakeIter_;
    ++return_iter;
    return return_iter;
}

template <typename T, typename Alloc>
typename List<T, Alloc>::const_iterator List<T, Alloc>::cend() const {
    return const_iterator(fakeIter_);
}

template <typename T, typename Alloc>
typename List<T, Alloc>::reverse_iterator List<T, Alloc>::rbegin() {
    return reverse_iterator(end());
}

template <typename T, typename Alloc>
typename List<T, Alloc>::const_reverse_iterator List<T, Alloc>::rbegin() const {
    return const_reverse_iterator(end());
}

template <typename T, typename Alloc>
typename List<T, Alloc>::reverse_iterator List<T, Alloc>::rend() {
    return reverse_iterator(begin());
}

template <typename T, typename Alloc>
typename List<T, Alloc>::const_reverse_iterator List<T, Alloc>::rend() const {
    return const_reverse_iterator(begin());
}

template <typename T, typename Alloc>
typename List<T, Alloc>::const_reverse_iterator List<T, Alloc>::crbegin() const {
    return const_reverse_iterator(end());
}

template <typename T, typename Alloc>
typename List<T, Alloc>::const_reverse_iterator List<T, Alloc>::crend() const {
    return const_reverse_iterator(begin());
}


template <typename T, typename Alloc>
template <bool isConst>
class List<T, Alloc>::CommonIterator {
public:
    using value_type = T;
    using pointer = typename std::conditional_t<isConst, const T*, T*>;
    using reference = typename std::conditional_t<isConst, const T&, T&>;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;

    CommonIterator() = default;
    CommonIterator(const CommonIterator<isConst>& other) : node_(other.node_) {}
    CommonIterator& operator=(const CommonIterator& other) {
        node_ = other.node_;
        return *this;
    }
    ~CommonIterator() = default;
    CommonIterator(BaseNode* node) : node_(node) {}

    CommonIterator& operator++() {
        node_ = node_->next;
        return *this;
    }

    CommonIterator operator++(int) {
        CommonIterator copy = *this;
        ++(*this);
        return copy;
    }

    CommonIterator operator--() {
        node_ = node_->prev;
        return *this;
    }

    CommonIterator operator--(int) {
        CommonIterator copy = *this;
        --(*this);
        return copy;
    }

    pointer operator->() const {
        return &(static_cast<Node*>(node_)->value);
    }
    pointer operator->() {
        return &(static_cast<Node*>(node_)->value);
    }

    reference operator*() const {
        return static_cast<Node*>(node_)->value;
    }

    reference operator*() {
        return static_cast<Node*>(node_)->value;
    }

    bool operator==(const CommonIterator<true>& other) const {
        return node_ == other.node_;
    }
    bool operator!=(const CommonIterator<true>& other) const {
        return node_ != other.node_;
    }

    operator CommonIterator<true>() const {
        return CommonIterator<true>(node_);
    }
    friend class CommonIterator<!isConst>;
    friend class List<T, Alloc>;

private:
    BaseNode* node_;
};

#endif //LIST_LIST_H
