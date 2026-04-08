#ifndef SRC_HPP
#define SRC_HPP

#include <stdexcept>
#include <initializer_list>
#include <typeinfo>
#include <utility>

namespace sjtu {

class any_ptr {
 private:
  struct ControlBlockBase {
    int ref_count;
    ControlBlockBase() : ref_count(1) {}
    virtual ~ControlBlockBase() = default;
    virtual const std::type_info& type() const = 0;
    virtual void* get_ptr() = 0;
  };

  template <typename T>
  struct ControlBlock : ControlBlockBase {
    T* ptr;
    ControlBlock(T* p) : ptr(p) {}
    ~ControlBlock() override {
        delete ptr;
    }
    const std::type_info& type() const override {
        return typeid(T);
    }
    void* get_ptr() override {
        return const_cast<void*>(static_cast<const void*>(ptr));
    }
  };

  ControlBlockBase* cb;

  void release() {
    if (cb) {
      if (--cb->ref_count == 0) {
        delete cb;
      }
      cb = nullptr;
    }
  }

 public:
  any_ptr() : cb(nullptr) {}

  any_ptr(const any_ptr &other) : cb(other.cb) {
    if (cb) {
      cb->ref_count++;
    }
  }
  
  template <class T> 
  any_ptr(T *ptr) : cb(ptr ? new ControlBlock<T>(ptr) : nullptr) {}

  ~any_ptr() {
    release();
  }

  any_ptr &operator=(const any_ptr &other) {
    if (this != &other) {
      release();
      cb = other.cb;
      if (cb) {
        cb->ref_count++;
      }
    }
    return *this;
  }
  
  template <class T> 
  any_ptr &operator=(T *ptr) {
    if (cb && cb->get_ptr() == ptr) {
      return *this;
    }
    release();
    if (ptr) {
      cb = new ControlBlock<T>(ptr);
    }
    return *this;
  }

  template <class T> 
  T &unwrap() {
    if (!cb) throw std::bad_cast();
    if (cb->type() != typeid(T)) throw std::bad_cast();
    return *static_cast<T*>(cb->get_ptr());
  }
  
  template <class T> 
  const T &unwrap() const {
    if (!cb) throw std::bad_cast();
    if (cb->type() != typeid(T)) throw std::bad_cast();
    return *static_cast<T*>(cb->get_ptr());
  }
};

template <class T> 
any_ptr make_any_ptr(const T &t) { 
  return any_ptr(new T(t)); 
}

template <class T, class... Args>
any_ptr make_any_ptr(Args&&... args) {
  return any_ptr(new T{std::forward<Args>(args)...});
}

template <class T, class U, class... Args>
any_ptr make_any_ptr(std::initializer_list<U> il, Args&&... args) {
  return any_ptr(new T{il, std::forward<Args>(args)...});
}

}  // namespace sjtu

#endif