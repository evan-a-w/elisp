#ifndef ENV_H
#define ENV_H

#include <list>
#include <memory>

template<typename T, typename V>
struct EnvList {
    V val;
    std::shared_ptr<EnvList<T, V>> next;

    EnvList(V v, std::shared_ptr<EnvList<T, V>> n) {
        val = v;
        next = n;
    }

    EnvList(V v) {
        val = v;
        next = nullptr;
    }
};

template<typename T, typename V>
struct EnvNode {
    T key;
    std::size_t prio;
    std::shared_ptr<EnvList<T, V>> l;
    std::shared_ptr<EnvNode<T, V>> left, right;

    EnvNode(T k, V v);
    EnvNode(EnvNode const &n);
    EnvNode clone() const;
    EnvNode push(T k, V v) const;
    EnvNode pop(T k) const;
    V const *lookup(T k) const;
    template<typename F>
    void for_each_value(F f) const;
private:
    EnvNode rotate_left() const;
    EnvNode rotate_right() const;
};

template<typename T, typename V>
struct Env {
    std::shared_ptr<EnvNode<T, V>> root;

    Env() : root(nullptr) {}
    Env push(T k, V v) const;
    Env pop(T k) const;
    bool search(T k, V &v) const;
    V &get(T k) const;
    void insert(T k, V v) const;
    void erase(T k) const;
    template<typename F>
    void for_each_value(F f) const;
private: 
    Env(std::shared_ptr<EnvNode<T, V>> r) : root(r) {}
};

#endif
