#include "eval.h"
#include "garb.h"
#include "roots.h"
#include "stdlib.h"

struct eval_state_t {
    handle_t env;
    std::vector<handle_t> stack;
    std::vector<handle_t> arg_stack;
    std::size_t to_eval;

    handle_t push(handle_t h) {
        stack.push_back(h);
        return h;
    }

    handle_t pop(void) {
        handle_t h = stack.back();
        stack.pop_back();
        return h;
    }

    handle_t peek(void) {
        return stack.back();
    }

    handle_t push_arg(handle_t h) {
        arg_stack.push_back(h);
        return h;
    }

    handle_t pop_arg(void) {
        handle_t h = arg_stack.back();
        arg_stack.pop_back();
        return h;
    }

    handle_t peek_arg(void) {
        return arg_stack.back();
    }

    handle_t push_dep(handle_t h) {
        if (to_eval) {
            return push_arg(h);
        }
        return push(h);
    }

    eval_state_t() {
        gc_init();
        init_handlers();
        init_roots();
        env = env_new();
        to_eval = 0;
    }

    ~eval_state_t() {
        gc_destroy();
        destroy_roots();
    }
};

static eval_state_t st;

void extra_roots(void (*f)(handle_t)) {
    for (auto h : st.stack)
        f(h);
    for (auto h : st.arg_stack)
        f(h);
    d<env_t>(st.env).for_each_value(f);
}

handle_t ast_to_val(ast_t *ast);
handle_t eval_ast(ast_t *);
handle_t eval(void);
void print_value(handle_t h);
