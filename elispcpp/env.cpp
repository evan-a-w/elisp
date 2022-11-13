#include <memory>
#include <stdlib.h>
#include <list>

#include "env.h"

template<typename T, typename V>
EnvNode<T, V>::EnvNode(T k, V v) {
    key = k;
    l = {v};
    left = nullptr;
    right = nullptr;
}

template<typename T, typename V>
EnvNode<T, V>::EnvNode(EnvNode const &n) {
    key = n.key;
    prio = n.prio;
    l = n.l;
    left = n.left;
    right = n.right;
}

template<typename T, typename V>
EnvNode<T, V> EnvNode<T, V>::clone() const {
    EnvNode<T, V> n(*this);
    n.left = left;
    n.right = right;
    return std::move(n);
}

template<typename T, typename V>
EnvNode<T, V> EnvNode<T, V>::rotate_left() const {
    EnvNode<T, V> n = right->clone();
    auto curr = std::make_shared<EnvNode<T, V>>(clone());
    curr->right = n.left;
    n.left = curr;
    return std::move(n);
}

template<typename T, typename V>
EnvNode<T, V> EnvNode<T, V>::rotate_right() const {
    EnvNode<T, V> n = left->clone();
    auto curr = std::make_shared<EnvNode<T, V>>(clone());
    curr->left = n.right;
    n.right = curr;
    return std::move(n);
}

template<typename T, typename V>
EnvNode<T, V> EnvNode<T, V>::push(T k, V v) const {
    auto res = clone();
    auto cmp = k.compare(key);
    if (cmp < 0) {
        if (left == nullptr) res.left = std::make_shared<EnvNode<T, V>>(k, v);
        else res.left = left->push(k, v);
        if (res.left->prio > res.prio) res = res.rotate_right();
    } else if (cmp > 0) {
        if (right == nullptr) res.right = std::make_shared<EnvNode<T, V>>(k, v);
        else res.right = right->push(k, v);
        if (res.right->prio > res.prio) res = res.rotate_left();
    } else {
        res.l = std::make_shared<EnvList<T, V>>(v, l);
    }
    return res;
}

template<typename T, typename V>
EnvNode<T, V> EnvNode<T, V>::pop(T k) const {
    auto res = clone();
    auto cmp = k.compare(key);
    if (cmp < 0) {
        if (left == nullptr) return res;
        res.left = left->pop(k);
    } else if (cmp > 0) {
        if (right == nullptr) return res;
        res.right = right->pop(k);
    } else {
        if (res.l != nullptr) res.l = res.l->next;
    }
    return res;
}

template<typename T, typename V>
V const *EnvNode<T, V>::lookup(T k) const {
    auto cmp = k.compare(key);
    if (cmp < 0) {
        if (left == nullptr) return nullptr;
        return left->lookup(k);
    } else if (cmp > 0) {
        if (right == nullptr) return nullptr;
        return right->lookup(k);
    } else {
        if (l == nullptr) return nullptr;
        return &l->val;
    }
}

template<typename T, typename V>
Env<T, V> Env<T, V>::push(T k, V v) const {
    if (root == nullptr) return Env<T, V>(std::make_shared<EnvNode<T, V>>(k, v));
    return {root->push(k, v)};
}

template<typename T, typename V>
Env<T, V> Env<T, V>::pop(T k) const {
    if (root == nullptr) return *this;
    return {root->pop(k)};
}

template<typename T, typename V>
bool Env<T, V>::search(T k, V &v) const {
    auto res = (root == nullptr) ? nullptr : root->lookup(k);
    if (res == nullptr) return false;
    v = *res;
    return true;
}

template<typename T, typename V>
V &Env<T, V>::get(T k) const {
    auto res = (root == nullptr) ? nullptr : root->lookup(k);
    if (res == nullptr) throw std::runtime_error("Key not found");
    return *res;
}

template<typename T, typename V>
void Env<T, V>::insert(T k, V v) const {
    if (root == nullptr) root = std::make_shared<EnvNode<T, V>>(k, v);
    else root = root->push(k, v);
}

template<typename T, typename V>
void Env<T, V>::erase(T k) const {
    if (root == nullptr) return;
    root = root->pop(k);
}

template<typename T, typename V>
template<typename F>
void EnvNode<T, V>::for_each_value(F f) const {
    EnvList<T, V> *curr = l;
    while (curr != nullptr) {
        f(curr->val);
        curr = curr->next;
    }
    if (left != nullptr) left->for_each_value(f);
    if (right != nullptr) right->for_each_value(f);
}

template<typename T, typename V>
template<typename F>
void Env<T, V>::for_each_value(F f) const {
    if (root == nullptr) return;
    root->for_each_value(f);
}
