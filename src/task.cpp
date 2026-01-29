#include "task.h"

static node_t*& getTaskList() {
    static node_t* tasks = nullptr;
    return tasks;
}

void addTask(exec_unit_t task) {
    node_t *new_task = new node_t{nullptr, task};
    node_t*& tasks = getTaskList();
    new_task -> next = tasks;
    tasks = new_task;
}

void removeTask(exec_unit_t task) {
    node_t*& tasks = getTaskList();
    node_t *n = tasks;
    node_t *prev = nullptr;
    while (n) {
        if (n->run == task) {
            if (prev) {
                prev->next = n->next;
            } else {
                tasks = n->next;
            }
            delete n;
            return;
        }
        prev = n;
        n = n->next;
    }
}

task_state_t runTasks() {
    node_t **tasks = &getTaskList();
    node_t *task = *tasks;
    task_state_t rstate = task_state_t::kDone;
    while (task) {
        node_t *next = task->next; // Store the next node before removing the current task
        task_state_t state = task->run();
        if (state == task_state_t::kDone) {
            removeTask(task->run);
        } else {
            rstate = state;
        }
        task = next; // Move to the next node
    }
    return rstate;
}