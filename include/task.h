#pragma once

// Colaborative multitasking support
// Piority or resource allocation can be done though placing the task several times in the list

enum class task_state_t {
    kYeilding,  // Task is not done, but it is not ready to run
    kWaiting,   // Task is waiting for a resource
    kDone       // Task is done
};

typedef task_state_t (*exec_unit_t)(void);
struct node_t {
    node_t *next;
    exec_unit_t run;
    node_t(node_t *next = nullptr, exec_unit_t run = nullptr) : next(next), run(run) {};
};

void addTask(exec_unit_t task);
void removeTask(exec_unit_t task);
task_state_t runTasks();
