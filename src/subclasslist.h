#ifndef T3_WIDGET_SUBCLASSLIST_H
#define T3_WIDGET_SUBCLASSLIST_H

#include <stdexcept>
#include <type_traits>
#include <utility>

// FIXME: currently this has received too little testing to allow use outside of the library.
#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

namespace t3_widget {

/** A list where items are potentially instances of subclasses of T, rather than instances of T
   itself. Due to the way the list is built, there can be no object slicing. Type T must provide the
   following:

   T *&next();
   T *&prev();

   Typically these should not be accessible generally, so it is advisable to make these functions
   private and add:

   template <typename T> friend class subclass_list_t;
*/
template <typename T>
class subclass_list_t {
 public:
  subclass_list_t() : head_(nullptr), tail_(nullptr) {}
  subclass_list_t(const subclass_list_t &other) = delete;
  subclass_list_t(subclass_list_t &&other) : head_(other.head_), tail_(other.tail_) {
    other.head_ = nullptr;
    other.tail_ = nullptr;
  }

  ~subclass_list_t() {
    while (head_) {
      T *item = head_;
      head_ = item->next();
      delete item;
    }
  }

  subclass_list_t &operator=(const subclass_list_t &other) = delete;
  subclass_list_t &operator=(subclass_list_t &&other) {
    head_ = other.head_;
    tail_ = other.tail_;
    other.head_ = nullptr;
    other.tail_ = nullptr;
  }

  T &back() {
    if (!tail_) {
      throw std::logic_error("can't get element from empty list");
    }
    return *tail_;
  }

  T &front() {
    if (!head_) {
      throw std::logic_error("can't get element from empty list");
    }
    return *head_;
  }

  void pop_back() {
    if (!tail_) {
      throw std::logic_error("can't pop from empty list");
    }
    T *tmp = tail_;
    tail_ = tmp->prev();
    if (tail_) {
      tail_->next() = nullptr;
    } else {
      head_ = nullptr;
    }
    delete tmp;
    --size_;
  }

  void pop_front() {
    if (!head_) {
      throw std::logic_error("can't pop from empty list");
    }
    T *tmp = head_;
    head_ = tmp->next();
    if (head_) {
      head_->prev() = nullptr;
    } else {
      tail_ = nullptr;
    }
    delete tmp;
    --size_;
  }

  template <typename S>
  typename std::enable_if<std::is_base_of<T, S>::value>::type push_back(const S &s) {
    link_back(new S(s));
  }

  template <typename S>
  typename std::enable_if<std::is_base_of<T, S>::value>::type push_back(S &&s) {
    link_back(new S(std::forward<S>(s)));
  }

  template <typename S = T, typename... Args>
  typename std::enable_if<std::is_base_of<T, S>::value>::type emplace_back(Args &&... args) {
    link_back(new S(std::forward<Args>(args)...));
  }

  template <typename S>
  typename std::enable_if<std::is_base_of<T, S>::value>::type push_front(const S &s) {
    link_front(new S(s));
  }

  template <typename S>
  typename std::enable_if<std::is_base_of<T, S>::value>::type push_front(S &&s) {
    if (s.next() || s.prev()) {
      throw std::logic_error("attempting to move item between lists");
    }
    link_front(new S(std::forward<S>(s)));
  }

  template <typename S = T, typename... Args>
  typename std::enable_if<std::is_base_of<T, S>::value>::type emplace_front(Args &&... args) {
    link_front(new S(std::forward<Args>(args)...));
  }

  void clear() {
    while (head_) {
      T *t = head_;
      head_ = t->next();
      delete t;
    }
    tail_ = nullptr;
    size_ = 0;
  }

  bool empty() const { return head_ == nullptr; }

  size_t size() const { return size_; }

 private:
  template <bool is_const>
  class iterator_base {
   public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = typename std::conditional<is_const, const T *, T *>::type;
    using reference = typename std::conditional<is_const, const T &, T &>::type;
    using iterator_category = std::bidirectional_iterator_tag;

    iterator_base() : item_(nullptr), list_(nullptr) {}
    iterator_base(const iterator_base<false> &other) : item_(other.item_), list_(other.list_) {}

    reference operator*() const { return *item_; }
    pointer operator->() const { return item_; }

    iterator_base &operator++() {
      this->item_ = this->item_->next();
      return *this;
    }

    iterator_base operator++(int) {
      iterator result = *this;
      this->item_ = this->item_->next();
      return *this;
    }

    iterator_base &operator--() {
      if (this->item_) {
        this->item_ = this->item_->prev();
      } else {
        this->item_ = this->list_->tail_;
      }
      return *this;
    }

    iterator_base operator--(int) {
      iterator_base result = *this;
      --(*this);
      return result;
    }

    bool operator==(const iterator_base &other) const { return this->item_ == other.item_; }
    bool operator!=(const iterator_base &other) const { return this->item_ != other.item_; }

   private:
    iterator_base(T *item, const subclass_list_t *list) : item_(item), list_(list) {}

    T *item_;
    const subclass_list_t *list_;
    friend class iterator_base<true>;
    template <typename X>
    friend class subclass_list_t;
  };

 public:
  using iterator = iterator_base<false>;
  using const_iterator = iterator_base<true>;

  iterator begin() { return iterator(head_, this); }
  iterator end() { return iterator(nullptr, this); }

  const_iterator begin() const { return cbegin(); }
  const_iterator end() const { return cend(); }

  const_iterator cbegin() const { return const_iterator(head_, this); }
  const_iterator cend() const { return const_iterator(nullptr, this); }

  using reverse_iterator = std::reverse_iterator<iterator_base<false>>;
  using const_reverse_iterator = std::reverse_iterator<iterator_base<true>>;

  reverse_iterator rbegin() { return reverse_iterator(tail_, this); }
  reverse_iterator rend() { return reverse_iterator(nullptr, this); }

  const_reverse_iterator rbegin() const { return crbegin(); }
  const_reverse_iterator rend() const { return crend(); }

  const_reverse_iterator crbegin() const { return const_reverse_iterator(tail_, this); }
  const_reverse_iterator crend() const { return const_reverse_iterator(nullptr, this); }

  template <typename S>
  typename std::enable_if<std::is_base_of<T, S>::value, iterator>::type insert(const_iterator pos,
                                                                               const S &s) {
    T *new_t = new S(s);
    link_insert(pos, new_t);
    return iterator(new_t, this);
  }

  template <typename S>
  typename std::enable_if<std::is_base_of<T, S>::value, iterator>::type insert(const_iterator pos,
                                                                               S &&s) {
    T *new_t = new S(std::forward<S>(s));
    link_insert(pos, new_t);
    return iterator(new_t, this);
  }

  template <typename S = T, typename... Args>
  typename std::enable_if<std::is_base_of<T, S>::value>::type emplace(const_iterator pos,
                                                                      Args &&... args) {
    link_insert(pos, new S(std::forward<Args>(args)...));
  }

  iterator erase(const_iterator pos) {
    auto result = unlink(pos);
    delete result.first;
    return result.second;
  }

  iterator erase(const_iterator start, const_iterator end) {
    if (start == end) {
      return iterator(end.item_, end.list_);
    }
    unlink(start, end);
    return iterator(end.item_, end.list_);
  }

  //================================== Extra methods ===============================================
  void link_back(T *t) {
    t->next() = nullptr;
    if (tail_) {
      tail_->next() = t;
      t->prev() = tail_;
      tail_ = t;
    } else {
      t->prev() = nullptr;
      head_ = tail_ = t;
    }
    ++size_;
  }

  void link_front(T *t) {
    t->prev() = nullptr;
    if (head_) {
      head_->prev() = t;
      t->next() = head_;
      head_ = t;
    } else {
      t->next() = nullptr;
      head_ = tail_ = t;
    }
    ++size_;
  }

  iterator link_insert(const_iterator pos, T *t) {
    if (!pos.item_) {
      link_back(t);
      return iterator(t, this);
    }
    t->next() = pos.item_;
    t->prev() = pos.item_->prev();
    t->next()->prev() = t;
    if (t->prev()) {
      t->prev()->next() = t;
    } else {
      head_ = t;
    }
    return iterator(t, this);
  }

  std::pair<T *, iterator> unlink(const_iterator pos) {
    T *t = &*pos;
    ++pos;
    if (t->next()) {
      t->next()->prev() = t->prev();
    } else {
      tail_ = t->prev();
    }
    if (t->prev()) {
      t->prev()->next() = t->next();
    } else {
      head_ = t->next();
    }
    t->next() = nullptr;
    t->prev() = nullptr;
    return {t, iterator(pos.item_, pos.list_)};
  }

  subclass_list_t unlink(const_iterator start, const_iterator end) {
    if (start == end) {
      return subclass_list_t();
    }
    const_iterator last = end;
    --last;
    if (start.item_->prev()) {
      start.item_->prev()->next() = end.item_;
    } else {
      head_ = end.item_;
    }
    if (end.item_) {
      end.item_->prev() = start.item_->prev();
    } else {
      tail_ = start.item_->prev();
    }
    return subclass_list_t(start.item_, last.item_);
  }

 private:
  /* Builds a new list from an existing chain. This does not require the ends of the chains to be
     pointing to null, as it would for normal linking operations. */
  subclass_list_t(T *first, T *last) : head_(first), tail_(last) {
    head_->prev() = nullptr;
    tail_->next() = nullptr;
  }

  T *head_, *tail_;
  size_t size_;
};

}  // namespace t3_widget

#endif  // SUBCLASSLIST_H
